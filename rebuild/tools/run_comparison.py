#!/usr/bin/env python3
"""Launch Wine and the C port on separate Xvfb displays, then replay a timeline.

This wraps ``replay_compare.py`` so a comparison can be started with one command
instead of manually preparing two X servers, two game windows, and their cleanup.
"""

from __future__ import annotations

import argparse
import os
import selectors
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path


REBUILD = Path(__file__).resolve().parents[1]
ROOT = REBUILD.parent


def start_xvfb(executable: str, geometry: str) -> tuple[subprocess.Popen[str], str]:
    """Start Xvfb with an automatically allocated display number."""
    process = subprocess.Popen(
        [executable, "-displayfd", "1", "-screen", "0", geometry,
         "-nolisten", "tcp", "-ac"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    assert process.stdout is not None
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ)
    try:
        if not selector.select(timeout=10):
            raise RuntimeError(f"Xvfb non ha allocato un display ({executable})")
        display_number = process.stdout.readline().strip()
    finally:
        selector.close()
        process.stdout.close()
    if not display_number.isdigit():
        raise RuntimeError(f"numero display Xvfb non valido: {display_number!r}")
    return process, f":{display_number}"


def wait_for_window(pattern: str, display: str, process: subprocess.Popen[bytes], timeout: float) -> None:
    """Wait for a visible game window, failing early if its process exits."""
    deadline = time.monotonic() + timeout
    env = {**os.environ, "DISPLAY": display}
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"il gioco è terminato prima di aprire la finestra: {pattern}")
        found = subprocess.run(
            ["xdotool", "search", "--onlyvisible", "--name", pattern],
            env=env, text=True, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, check=False,
        )
        if found.returncode == 0:
            return
        time.sleep(0.2)
    raise RuntimeError(f"timeout in attesa della finestra {pattern!r} su {display}")


def stop_process(process: subprocess.Popen[bytes] | subprocess.Popen[str] | None) -> None:
    if process is None or process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=3)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("timeline", type=Path, help="JSON timeline per replay_compare.py")
    parser.add_argument("output_dir", type=Path, help="cartella per catture, diff e CSV")
    parser.add_argument("--original", type=Path, default=ROOT / "Wargames.exe")
    parser.add_argument("--candidate", type=Path, default=REBUILD / "wargame-c")
    parser.add_argument("--wine", default="wine")
    parser.add_argument("--wineprefix", type=Path,
                        default=Path(os.environ.get("WINEPREFIX", "~/.wine-wargame")).expanduser())
    parser.add_argument("--wine-audio-driver", choices=("pulse", "alsa", "oss"),
                        default="pulse", help="driver audio Wine del prefix QA")
    parser.add_argument("--sdl-audio-driver", default="pulse",
                        help="driver audio SDL quando si abilita l'audio")
    parser.add_argument("--audio", action="store_true",
                        help="abilita l'audio reale; per default i test sono silenziosi")
    parser.add_argument("--xvfb", default="Xvfb")
    parser.add_argument("--geometry", default="800x600x24")
    parser.add_argument("--startup-timeout", type=float, default=30)
    parser.add_argument("--candidate-start", help="valore WARGAME_COMPARE_START del candidato")
    parser.add_argument("--record-original", type=Path,
                        help="registra il display Wine in FFV1 per confronti frame indipendenti dal timing")
    parser.add_argument("--record-fps", type=int, default=30)
    parser.add_argument("--original-window", default="^Wargames$")
    parser.add_argument("--candidate-window", default="Wargames - C reconstruction")
    parser.add_argument("--tolerance", type=int, default=0)
    args = parser.parse_args()

    timeline = args.timeline.resolve()
    original = args.original.resolve()
    candidate = args.candidate.resolve()
    output_dir = args.output_dir.resolve()
    replay = REBUILD / "tools" / "replay_compare.py"
    for path in (timeline, original, candidate, replay):
        if not path.is_file():
            parser.error(f"file non trovato: {path}")
    xvfb_path = shutil.which(args.xvfb)
    wine_path = shutil.which(args.wine)
    if not xvfb_path:
        parser.error(f"Xvfb non trovato: {args.xvfb}")
    if not wine_path:
        parser.error(f"Wine non trovato: {args.wine}")

    original_xvfb = candidate_xvfb = None
    original_process = candidate_process = None
    recording_process = None
    mute_module_id = None
    wine_env = {**os.environ, "WINEPREFIX": str(args.wineprefix), "WINEDEBUG": "-all"}
    try:
        if not args.audio:
            pactl = shutil.which("pactl")
            if not pactl:
                raise RuntimeError("pactl non trovato; serve per silenziare il solo audio Wine del test")
            sink_name = f"wargame_qa_mute_{os.getpid()}"
            mute_module = subprocess.run(
                [pactl, "load-module", "module-null-sink", f"sink_name={sink_name}",
                 "sink_properties=device.description=Wargame_QA_Mute"],
                text=True, capture_output=True, check=False,
            )
            if mute_module.returncode != 0:
                raise RuntimeError("PipeWire/PulseAudio non ha creato il sink temporaneo muto")
            mute_module_id = mute_module.stdout.strip()
            wine_env["PULSE_SINK"] = sink_name

        audio_setup = subprocess.run(
            [wine_path, "reg", "add", r"HKCU\Software\Wine\Drivers",
             "/v", "Audio", "/t", "REG_SZ", "/d", args.wine_audio_driver, "/f"],
            cwd=ROOT, env=wine_env, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, check=False,
        )
        if audio_setup.returncode != 0:
            raise RuntimeError("impossibile impostare il driver audio muto nel prefix Wine QA")

        original_xvfb, original_display = start_xvfb(xvfb_path, args.geometry)
        candidate_xvfb, candidate_display = start_xvfb(xvfb_path, args.geometry)

        original_env = {**wine_env, "DISPLAY": original_display}
        candidate_env = {
            **os.environ,
            "DISPLAY": candidate_display,
            "SDL_VIDEODRIVER": "x11",
            "SDL_AUDIODRIVER": args.sdl_audio_driver if args.audio else "dummy",
        }
        if mute_module_id:
            candidate_env["PULSE_SINK"] = wine_env["PULSE_SINK"]
        if args.candidate_start:
            candidate_env["WARGAME_COMPARE_START"] = args.candidate_start
        else:
            candidate_env.pop("WARGAME_COMPARE_START", None)

        original_process = subprocess.Popen([wine_path, str(original)], cwd=ROOT, env=original_env)
        candidate_process = subprocess.Popen([str(candidate)], cwd=REBUILD, env=candidate_env)
        wait_for_window(args.original_window, original_display, original_process, args.startup_timeout)
        wait_for_window(args.candidate_window, candidate_display, candidate_process, args.startup_timeout)
        print(f"Originale su {original_display}; C su {candidate_display}", flush=True)

        if args.record_original:
            ffmpeg = shutil.which("ffmpeg")
            if not ffmpeg:
                parser.error("ffmpeg non trovato per --record-original")
            width, height = args.geometry.split("x")[:2]
            recording_path = args.record_original.expanduser().resolve()
            recording_path.parent.mkdir(parents=True, exist_ok=True)
            recording_process = subprocess.Popen(
                [ffmpeg, "-nostdin", "-loglevel", "error", "-y", "-f", "x11grab",
                 "-draw_mouse", "0", "-framerate", str(args.record_fps),
                 "-video_size", f"{width}x{height}",
                 "-i", original_display, "-an", "-c:v", "ffv1", "-level", "3",
                 str(recording_path)],
                cwd=REBUILD, env=original_env,
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            )
            time.sleep(0.3)
            if recording_process.poll() is not None:
                raise RuntimeError("ffmpeg non è riuscito ad avviare la registrazione originale")
            print(f"Registrazione Wine: {recording_path}", flush=True)

        command = [
            sys.executable, str(replay), str(timeline), str(output_dir),
            "--original-window", args.original_window,
            "--candidate-window", args.candidate_window,
            "--original-display", original_display,
            "--candidate-display", candidate_display,
            "--tolerance", str(args.tolerance),
        ]
        return subprocess.run(command, cwd=REBUILD, check=False).returncode
    except KeyboardInterrupt:
        print("Confronto interrotto", file=sys.stderr)
        return 130
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"run_comparison: {error}", file=sys.stderr)
        return 2
    finally:
        if recording_process is not None and recording_process.poll() is None:
            recording_process.send_signal(signal.SIGINT)
            try:
                recording_process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                recording_process.kill()
                recording_process.wait(timeout=3)
        stop_process(candidate_process)
        stop_process(original_process)
        subprocess.run(
            ["wineserver", "-k"], cwd=ROOT, env=wine_env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False,
        )
        if mute_module_id:
            pactl = shutil.which("pactl")
            if pactl:
                subprocess.run([pactl, "unload-module", mute_module_id],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                               check=False)
        stop_process(candidate_xvfb)
        stop_process(original_xvfb)


if __name__ == "__main__":
    raise SystemExit(main())
