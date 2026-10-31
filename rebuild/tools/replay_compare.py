#!/usr/bin/env python3
"""Replay one input timeline to two X11 windows and compare named checkpoints.

Input is a JSON list containing actions such as ``{"key":"Down"}``,
``{"text":"Paris"}``, ``{"wait_ms":500}``, and
``{"capture":"after-menu"}``. Each key/text action is sent to both windows
in sequence, focusing each one first, unless ``"target":"original"`` or
``"target":"candidate"`` selects one side. PNG pairs and pixel-difference
reports are written under the chosen output directory. The windows may share
DISPLAY or run on separate displays selected with the command-line options.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import time
from pathlib import Path

from PIL import Image

from compare_frames import compare_pair


def display_env(display: str | None) -> dict[str, str] | None:
    """Return an environment selecting one X display, if explicitly requested."""
    if display is None:
        return None
    return {**os.environ, "DISPLAY": display}


def run_xdotool(*args: str, display: str | None = None) -> None:
    """Send a single event to one window using xdotool."""
    subprocess.run(["xdotool", *args], check=True, env=display_env(display))


def top_container(wid: str, display: str | None = None) -> str:
    """Return the top-level frame that contains a game surface."""
    # Query the window directly. Parsing the root tree by indentation is fragile:
    # xwininfo changes that indentation depending on the window manager, and can
    # mistakenly return the game child itself (which Wine may destroy on focus).
    info = subprocess.run(
        ["xwininfo", "-id", wid], check=True, text=True, capture_output=True,
        env=display_env(display),
    ).stdout
    match = re.search(r"Parent window id:\s*(0x[0-9a-fA-F]+)", info)
    return match.group(1) if match else wid


def focus_window(wid: str, display: str | None = None) -> None:
    """Raise and focus a game so the original's foreground-gated input works."""
    run_xdotool("windowraise", top_container(wid, display), display=display)
    run_xdotool("windowfocus", "--sync", wid, display=display)


def type_polled_text(value: str, display: str | None, hold_ms: int,
                     delay_ms: int) -> None:
    """Type ASCII with short key holds for games that poll DirectInput."""
    shifted_symbols = {
        "!": "1", "@": "2", "#": "3", "$": "4", "%": "5",
        "^": "6", "&": "7", "*": "8", "(": "9", ")": "0",
        "_": "minus", "+": "equal", "{": "bracketleft",
        "}": "bracketright", "|": "backslash", ":": "semicolon",
        '"': "apostrophe", "<": "comma", ">": "period",
        "?": "slash", "~": "grave",
    }
    symbol_keys = {
        "'": "apostrophe", ",": "comma", "-": "minus", ".": "period",
        "/": "slash", ";": "semicolon", "=": "equal", "[": "bracketleft",
        "]": "bracketright", "\\": "backslash", "`": "grave",
    }
    for char in value:
        shift = char.isupper() or char in shifted_symbols
        key = shifted_symbols.get(char, symbol_keys.get(
            char, char.lower() if char.isupper() else char))
        if char == " ":
            key = "space"
        if shift:
            run_xdotool("keydown", "Shift_L", display=display)
        run_xdotool("keydown", key, display=display)
        time.sleep(max(0, hold_ms) / 1000)
        run_xdotool("keyup", key, display=display)
        if shift:
            run_xdotool("keyup", "Shift_L", display=display)
        time.sleep(max(0, delay_ms) / 1000)


def window_id(pattern: str, display: str | None = None) -> str:
    """Find a visible window matching a regular expression, preferring exact titles."""
    result = subprocess.run(
        ["xdotool", "search", "--onlyvisible", "--name", pattern],
        check=True, text=True, capture_output=True, env=display_env(display),
    )
    ids = result.stdout.split()
    if not ids:
        raise RuntimeError(f"no visible X11 window matches {pattern!r}")
    # The game surface is generally the largest matching top-level window.
    candidates: list[tuple[int, str]] = []
    for wid in ids:
        info = subprocess.run(
            ["xwininfo", "-id", wid], check=True, text=True, capture_output=True,
            env=display_env(display),
        ).stdout
        match = re.search(r"-geometry\s+(\d+)x(\d+)", info)
        area = int(match.group(1)) * int(match.group(2)) if match else 0
        name = subprocess.run(
            ["xdotool", "getwindowname", wid], check=True, text=True,
            capture_output=True, env=display_env(display),
        ).stdout.strip()
        exact = name == pattern.strip("^$")
        candidates.append((area + (1 << 40 if exact else 0), wid))
    return max(candidates)[1]


def capture_window(wid: str, destination: Path, display: str | None = None) -> None:
    """Raise a window, capture the visible X root, and crop its screen bounds."""
    run_xdotool("windowraise", top_container(wid, display), display=display)
    time.sleep(0.6)  # Let Wine/SDL repaint after the covering window is raised.
    xwd_path = destination.with_suffix(".root.xwd")
    root_png = destination.with_suffix(".root.png")
    with xwd_path.open("wb") as output:
        subprocess.run(["xwd", "-silent", "-root"], env=display_env(display),
                       check=True, stdout=output)
    with xwd_path.open("rb") as source, root_png.open("wb") as output:
        xwdtopnm = subprocess.Popen(["xwdtopnm"], stdin=source, stdout=subprocess.PIPE)
        subprocess.run(["pnmtopng"], stdin=xwdtopnm.stdout, stdout=output, check=True)
        assert xwdtopnm.stdout is not None
        xwdtopnm.stdout.close()
        if xwdtopnm.wait() != 0:
            raise RuntimeError("xwdtopnm failed")
    xwd_path.unlink()
    info = subprocess.run(
        ["xwininfo", "-id", wid], check=True, text=True, capture_output=True,
        env=display_env(display),
    ).stdout
    x = int(re.search(r"Absolute upper-left X:\s*(-?\d+)", info).group(1))
    y = int(re.search(r"Absolute upper-left Y:\s*(-?\d+)", info).group(1))
    width = int(re.search(r"Width:\s*(\d+)", info).group(1))
    height = int(re.search(r"Height:\s*(\d+)", info).group(1))
    with Image.open(root_png) as root_image:
        root_image.crop((x, y, x + width, y + height)).save(destination)
    root_png.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("timeline", type=Path, help="JSON file with actions and capture checkpoints")
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--original-window", default="^Wargames$")
    parser.add_argument("--candidate-window", default="Wargames - C reconstruction")
    parser.add_argument("--original-display", help="X11 display for Wine; defaults to DISPLAY")
    parser.add_argument("--candidate-display", help="X11 display for C; defaults to DISPLAY")
    parser.add_argument("--tolerance", type=int, default=0)
    args = parser.parse_args()
    actions = json.loads(args.timeline.read_text(encoding="utf-8"))
    if not isinstance(actions, list):
        parser.error("timeline must contain a JSON array")
    original_display = args.original_display
    candidate_display = args.candidate_display
    original_id = window_id(args.original_window, original_display)
    candidate_id = window_id(args.candidate_window, candidate_display)
    print(f"Original window {original_id}; C window {candidate_id}")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    captures: list[tuple[str, Path, Path]] = []
    for index, action in enumerate(actions):
        if not isinstance(action, dict):
            parser.error(f"action {index} must be an object")
        print(f"Action {index + 1}/{len(actions)}: {action}", flush=True)
        target = action.get("target", "both")
        windows = {
            "original": ((original_id, original_display),),
            "candidate": ((candidate_id, candidate_display),),
            "both": ((original_id, original_display), (candidate_id, candidate_display)),
        }.get(target)
        if windows is None:
            parser.error(f"action {index} target must be original, candidate, or both")
        if "wait_ms" in action:
            time.sleep(max(0, int(action["wait_ms"])) / 1000)
        elif "key" in action:
            key = str(action["key"])
            repeat = max(1, int(action.get("repeat", 1)))
            interval = max(0, int(action.get("interval_ms", 0))) / 1000
            hold = max(1, int(action.get("hold_ms", 140))) / 1000
            for _ in range(repeat):
                for wid, display in windows:
                    # On one X server, focus switches between the two games.
                    # On separate servers, focus each game's window on its own
                    # server so XTest does not send keys to a desktop window.
                    focus_window(wid, display)
                    # Hold keydown long enough for both SDL and DirectInput polling
                    # to observe it, then release it explicitly.
                    run_xdotool("keydown", key, display=display)
                    time.sleep(hold)
                    run_xdotool("keyup", key, display=display)
                if interval:
                    time.sleep(interval)
        elif "text" in action:
            value = str(action["text"])
            hold_ms = max(0, int(action.get("char_hold_ms", 120)))
            delay_ms = max(0, int(action.get("char_delay_ms", 30)))
            for wid, display in windows:
                focus_window(wid, display)
                type_polled_text(value, display, hold_ms, delay_ms)
        elif "capture" in action:
            name = re.sub(r"[^A-Za-z0-9_.-]+", "_", str(action["capture"]))
            reference = args.output_dir / f"original-{name}.png"
            candidate = args.output_dir / f"c-{name}.png"
            capture_window(original_id, reference, original_display)
            capture_window(candidate_id, candidate, candidate_display)
            captures.append((name, reference, candidate))
            print(f"Captured {name}", flush=True)
        else:
            parser.error(f"action {index} needs key, text, wait_ms, or capture")
        if "settle_ms" in action:
            time.sleep(max(0, int(action["settle_ms"])) / 1000)
    diff_dir = args.output_dir / "diffs"
    diff_dir.mkdir(exist_ok=True)
    rows = []
    for name, reference, candidate in captures:
        rows.append(compare_pair(reference, candidate, diff_dir / f"diff-{name}.png",
                                 args.tolerance))
    if rows:
        import csv
        with (args.output_dir / "comparison.csv").open("w", newline="", encoding="utf-8") as report:
            writer = csv.DictWriter(report, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
    mismatches = sum(int(row["different_pixels"]) for row in rows)
    print(f"Compared {len(rows)} checkpoints: {mismatches} total differing pixels")
    return int(mismatches > 0)


if __name__ == "__main__":
    raise SystemExit(main())
