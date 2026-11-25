"""Refina ataques de chefes sem modificar silhueta, alpha ou quadros."""

from __future__ import annotations

import argparse
import colorsys
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
SPRITES = ROOT / "code" / "assets" / "sprites" / "sobrevivencia"

# B7 permanece de fora: os dois arquivos não possuem uso ativo e são "pergunte antes".
FILES = (
    "bp_acesso", "bp_contrato", "bp_corrente", "bp_falsa", "bp_garras",
    "bp_justa", "bp_maos", "bp_martelo", "bp_olhar", "bp_onda",
    "bp_papelada", "bp_porta", "bp_rastro", "bp_sobreajuste", "bp_sombra",
)


def refine(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    """Aumenta discretamente a leitura sem interpolacao espacial."""
    red, green, blue = (channel / 255.0 for channel in rgb)
    hue, saturation, value = colorsys.rgb_to_hsv(red, green, blue)
    saturation = min(1.0, saturation * 1.09)
    value = min(1.0, max(0.0, (value - 0.42) * 1.075 + 0.42))
    if value > 0.70:
        value = min(1.0, value + 0.022)
    elif value < 0.22:
        value = max(0.0, value - 0.010)
    result = colorsys.hsv_to_rgb(hue, saturation, value)
    return tuple(round(channel * 255) for channel in result)


def process(path: Path) -> None:
    with Image.open(path) as source:
        image = source.convert("RGBA")
    original_alpha = image.getchannel("A").tobytes()
    palette: dict[tuple[int, int, int, int], tuple[int, int, int, int]] = {}
    pixels = []
    for pixel in image.getdata():
        if pixel[3] == 0:
            pixels.append(pixel)
            continue
        if pixel not in palette:
            palette[pixel] = (*refine(pixel[:3]), pixel[3])
        pixels.append(palette[pixel])
    image.putdata(pixels)
    if image.getchannel("A").tobytes() != original_alpha:
        raise RuntimeError(f"A mascara alpha mudou em {path.name}")
    image.save(path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("names", nargs="*", choices=FILES)
    args = parser.parse_args()
    for name in args.names or FILES:
        process(SPRITES / f"{name}.png")
