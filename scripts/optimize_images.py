#!/usr/bin/env python3
"""Rebuild Pebble 64-color bird bitmaps from original illustrations.

Run at repository root:
    python3 -m pip install Pillow
    python3 scripts/optimize_images.py

Originals are never overwritten. The CSV links each watch resource to its source.
"""
import argparse
import csv
from pathlib import Path
from PIL import Image, ImageEnhance, ImageOps

ROOT = Path(__file__).resolve().parent.parent
CATALOG = ROOT / "bird_catalog.csv"
ORIGINALS = ROOT / "originals"
OUTPUT = ROOT / "resources" / "images"
MAX_SIZE = (188, 106)
# Pebble Time / Emery: six color bits, 2 bits for each RGB channel.
CHANNELS = (0, 85, 170, 255)
PALETTE = [value for red in CHANNELS for green in CHANNELS for blue in CHANNELS for value in (red, green, blue)]
PALETTE_IMAGE = Image.new("P", (1, 1))
PALETTE_IMAGE.putpalette(PALETTE + [0] * (768 - len(PALETTE)))
NO_DITHER = Image.Dither.NONE


def optimize(source: Path, target: Path, *, contrast: float, color: float, margin: int) -> None:
    with Image.open(source) as raw:
        # Composite transparency against white; otherwise image palettization can
        # unexpectedly turn the transparent background black.
        rgba = ImageOps.exif_transpose(raw).convert("RGBA")
        background = Image.new("RGBA", rgba.size, (255, 255, 255, 255))
        background.alpha_composite(rgba)
        image = background.convert("RGB")

    # Find the non-white illustration bounding box. Crop *before* shrinking
    # so original detail is retained for the final high-quality resampling.
    difference = ImageOps.invert(image.convert("L"))
    mask = difference.point(lambda p: 255 if p > 20 else 0)
    bounds = mask.getbbox()
    if bounds:
        x0, y0, x1, y1 = bounds
        width, height = image.size
        bounds = (
            max(0, x0 - margin),
            max(0, y0 - margin),
            min(width, x1 + margin),
            min(height, y1 + margin),
        )
        image = image.crop(bounds)

    image.thumbnail(MAX_SIZE, Image.Resampling.LANCZOS)
    # Subtle, configurable enhancement only. Aggressive sharpening on old
    # lithographs generates ugly halos and colored fringe artifacts.
    image = ImageEnhance.Contrast(image).enhance(contrast)
    image = ImageEnhance.Color(image).enhance(color)

    # Map directly to Pebble's actual RGB222 colors without introducing
    # error-diffusion speckling. Resizing is done *before* this step.
    indexed = image.quantize(palette=PALETTE_IMAGE, dither=NO_DITHER)
    result = Image.new("RGB", MAX_SIZE, (255, 255, 255))
    x = (MAX_SIZE[0] - image.width) // 2
    y = (MAX_SIZE[1] - image.height) // 2
    result.paste(indexed.convert("RGB"), (x, y))
    target.parent.mkdir(parents=True, exist_ok=True)
    result.save(target, optimize=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--contrast", type=float, default=1.06)
    parser.add_argument("--color", type=float, default=1.10)
    parser.add_argument("--margin", type=int, default=12)
    args = parser.parse_args()
    with CATALOG.open(newline="", encoding="utf-8-sig") as fh:
        rows = list(csv.DictReader(fh))
    failed = []
    for row in rows:
        source = ORIGINALS / row["source_file"]
        target = OUTPUT / row["filename"]
        if not source.is_file():
            print(f"MISSING: {source}")
            failed.append(source.name)
            continue
        optimize(source, target, contrast=args.contrast, color=args.color, margin=args.margin)
        print(f"OK: {target.relative_to(ROOT)}")
    if failed:
        raise SystemExit(f"{len(failed)} source image(s) missing. No changes to those resources.")
    print(f"Optimized {len(rows)} illustrations for Pebble RGB222.")


if __name__ == "__main__":
    main()
