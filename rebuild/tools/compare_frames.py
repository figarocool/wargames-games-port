#!/usr/bin/env python3
"""Compare matching PNG screenshots from the original and C reconstruction.

The tool pairs files by suffix, reports exact and thresholded pixel differences,
and writes an amplified difference image for every pair. It is intended for
batch comparison of whole scenes captured at matching input/timing checkpoints.
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

from PIL import Image, ImageChops, ImageEnhance


def compare_pair(reference_path: Path, candidate_path: Path, output_path: Path,
                 tolerance: int) -> dict[str, object]:
    """Compare one pair and save a visual diff, padding mismatched sizes black."""
    reference = Image.open(reference_path).convert("RGB")
    candidate = Image.open(candidate_path).convert("RGB")
    reference_size = reference.size
    candidate_size = candidate.size
    width = max(reference.width, candidate.width)
    height = max(reference.height, candidate.height)

    ref_canvas = Image.new("RGB", (width, height), (0, 0, 0))
    cand_canvas = Image.new("RGB", (width, height), (0, 0, 0))
    ref_canvas.paste(reference, (0, 0))
    cand_canvas.paste(candidate, (0, 0))
    absolute = ImageChops.difference(ref_canvas, cand_canvas)

    exact_pixels = 0
    threshold_pixels = 0
    max_delta = 0
    for red, green, blue in absolute.getdata():
        delta = max(red, green, blue)
        max_delta = max(max_delta, delta)
        if delta:
            exact_pixels += 1
        if delta > tolerance:
            threshold_pixels += 1

    # Red marks changed pixels; the amplified grayscale preserves their size.
    amplified = ImageEnhance.Brightness(absolute).enhance(4)
    diff = Image.new("RGB", (width, height), (0, 0, 0))
    diff_pixels = amplified.load()
    abs_pixels = absolute.load()
    for y in range(height):
        for x in range(width):
            if abs_pixels[x, y] != (0, 0, 0):
                r, g, b = diff_pixels[x, y]
                diff_pixels[x, y] = (max(255, r), g // 3, b // 3)
    diff.save(output_path)

    return {
        "reference": reference_path.name,
        "candidate": candidate_path.name,
        "reference_size": f"{reference_size[0]}x{reference_size[1]}",
        "candidate_size": f"{candidate_size[0]}x{candidate_size[1]}",
        "different_pixels": exact_pixels,
        "pixels_over_tolerance": threshold_pixels,
        "max_channel_delta": max_delta,
        "diff_image": output_path.name,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference_dir", type=Path)
    parser.add_argument("candidate_dir", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--reference-prefix", default="original-")
    parser.add_argument("--candidate-prefix", default="c-")
    parser.add_argument("--tolerance", type=int, default=0,
                        help="Ignore per-pixel channel deltas at or below this value")
    args = parser.parse_args()
    if not 0 <= args.tolerance <= 255:
        parser.error("--tolerance must be between 0 and 255")
    if not args.reference_dir.is_dir() or not args.candidate_dir.is_dir():
        parser.error("both screenshot directories must exist")
    args.output_dir.mkdir(parents=True, exist_ok=True)

    references = {
        path.name[len(args.reference_prefix):]: path
        for path in args.reference_dir.glob(f"{args.reference_prefix}*.png")
    }
    candidates = {
        path.name[len(args.candidate_prefix):]: path
        for path in args.candidate_dir.glob(f"{args.candidate_prefix}*.png")
    }
    common = sorted(references.keys() & candidates.keys())
    missing_reference = sorted(candidates.keys() - references.keys())
    missing_candidate = sorted(references.keys() - candidates.keys())
    if not common:
        parser.error("no pairs found; files must share the suffix after their prefixes")

    rows = []
    for suffix in common:
        safe_suffix = suffix.replace("/", "_")
        rows.append(compare_pair(
            references[suffix], candidates[suffix],
            args.output_dir / f"diff-{safe_suffix}", args.tolerance,
        ))

    report_path = args.output_dir / "comparison.csv"
    with report_path.open("w", newline="", encoding="utf-8") as report:
        writer = csv.DictWriter(report, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    print(f"Compared {len(rows)} pairs; exact pixel mismatches:")
    for row in rows:
        print(f"  {row['reference']} vs {row['candidate']}: "
              f"{row['different_pixels']} pixels "
              f"({row['pixels_over_tolerance']} above tolerance), "
              f"max delta {row['max_channel_delta']}")
    if missing_reference:
        print("No reference image for: " + ", ".join(missing_reference))
    if missing_candidate:
        print("No candidate image for: " + ", ".join(missing_candidate))
    print(f"CSV report: {report_path}")
    return int(any(row["different_pixels"] for row in rows))


if __name__ == "__main__":
    raise SystemExit(main())
