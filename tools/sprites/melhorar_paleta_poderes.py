"""Refina a paleta dos poderes sem alterar forma, alpha ou coordenadas."""

from __future__ import annotations

import argparse
import colorsys
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
SPRITES = ROOT / "code" / "assets" / "sprites" / "sobrevivencia"

FILES = (
    "fx_grampeador", "fx_ponteiro", "fx_caneca", "fx_postits",
    "fx_naoperturbe", "fx_deploy", "fx_modus", "fx_tabela",
    "fx_novaconversa", "fx_fibra", "fx_terminal", "fx_negacao",
    "fx_recursao", "ic_grampeador", "ic_ponteiro", "ic_caneca",
    "ic_postits", "ic_naoperturbe", "ic_deploy", "ic_modus",
    "ic_tabela", "ic_novaconversa", "ic_fibra", "ic_prompt",
    "ic_negacao", "ic_recursao",
    "bp_nota", "fx_colega", "ic_nota", "ic_colega", "ps_cracha",
    "ic_calor",
)


def refine(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    red, green, blue = (channel / 255.0 for channel in rgb)
    hue, saturation, value = colorsys.rgb_to_hsv(red, green, blue)

    # Aumentos discretos, calculados por cor, sem interpolação espacial.
    saturation = min(1.0, saturation * 1.08)
    value = min(1.0, max(0.0, (value - 0.43) * 1.07 + 0.43))
    if value > 0.72:
        value = min(1.0, value + 0.025)
    elif value < 0.24:
        value = max(0.0, value - 0.012)

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
        raise RuntimeError(f"A máscara alpha mudou em {path.name}")
    image.save(path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("names", nargs="*", choices=FILES)
    args = parser.parse_args()
    for name in args.names or FILES:
        process(SPRITES / f"{name}.png")
