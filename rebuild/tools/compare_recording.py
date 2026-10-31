#!/usr/bin/env python3
"""Find exact checkpoint images inside a constant-frame-rate lossless recording.

Unlike a timed replay, this compares animation states independently of their
arrival time. The first matching video frame and its timestamp are recorded;
the result does not assert that the two programs run at identical speeds.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
from fractions import Fraction
from pathlib import Path

from PIL import Image


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("recording", type=Path)
    parser.add_argument("candidate_dir", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--pattern", default="c-tactical-*-*.png")
    args = parser.parse_args()
    paths = sorted(args.candidate_dir.glob(args.pattern))
    if not paths:
        parser.error("no candidate checkpoints found")
    info = json.loads(subprocess.run([
        "ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
        "stream=width,height,r_frame_rate", "-of", "json", str(args.recording),
    ], check=True, capture_output=True, text=True).stdout)["streams"][0]
    size = (info["width"], info["height"])
    fps = Fraction(info["r_frame_rate"])
    candidates: dict[bytes, list[tuple[Path, bytes]]] = {}
    for path in paths:
        with Image.open(path) as image:
            if image.size != size:
                parser.error(f"{path}: expected {size}, got {image.size}")
            pixels = image.convert("RGB").tobytes()
        candidates.setdefault(hashlib.sha256(pixels).digest(), []).append((path, pixels))
    args.output_dir.mkdir(parents=True, exist_ok=True)
    diffs = args.output_dir / "diffs"
    diffs.mkdir(exist_ok=True)
    matches: dict[Path, int] = {}
    frame_size = size[0] * size[1] * 3
    decoder = subprocess.Popen([
        "ffmpeg", "-nostdin", "-loglevel", "error", "-i", str(args.recording),
        "-map", "0:v:0", "-f", "rawvideo", "-pix_fmt", "rgb24", "-",
    ], stdout=subprocess.PIPE)
    assert decoder.stdout is not None
    try:
        index = 0
        while True:
            frame = decoder.stdout.read(frame_size)
            if not frame:
                break
            if len(frame) != frame_size:
                raise RuntimeError("truncated decoded video frame")
            for path, pixels in candidates.get(hashlib.sha256(frame).digest(), []):
                if path not in matches and frame == pixels:
                    suffix = path.name.removeprefix("c-")
                    Image.frombytes("RGB", size, frame).save(args.output_dir / f"original-{suffix}")
                    Image.frombytes("RGB", size, pixels).save(args.output_dir / f"c-{suffix}")
                    Image.new("RGB", size).save(diffs / f"diff-{suffix}")
                    matches[path] = index
            index += 1
        if decoder.wait() != 0:
            raise RuntimeError("FFmpeg failed to decode the recording")
    finally:
        decoder.stdout.close()
        if decoder.poll() is None:
            decoder.terminate()
            decoder.wait()
    rows = []
    for path in paths:
        frame = matches.get(path)
        rows.append({
            "candidate": path.name,
            "original_frame": frame if frame is not None else "",
            "original_time_s": f"{float(frame / fps):.6f}" if frame is not None else "",
            "size": f"{size[0]}x{size[1]}",
            "different_pixels": 0 if frame is not None else "unmatched",
        })
        print(f"{path.name}: " + (f"0 different pixels, original frame {frame}"
                                  if frame is not None else "no exact match"))
    with (args.output_dir / "recording-comparison.csv").open("w", newline="", encoding="utf-8") as report:
        writer = csv.DictWriter(report, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"Exact matches: {len(matches)}/{len(paths)}")
    return int(len(matches) != len(paths))


if __name__ == "__main__":
    raise SystemExit(main())
