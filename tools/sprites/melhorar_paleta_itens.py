"""Refina itens e passivos sem alterar forma, alpha ou coordenadas."""

from __future__ import annotations

import argparse
import colorsys
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
SPRITES = ROOT / "code" / "assets" / "sprites" / "sobrevivencia"
FILES = (
    "ps_cafe", "ps_manual", "ps_colinha", "ps_ergonomia", "ps_saude",
    "ps_network", "ps_horaextra", "ps_lanterna", "it_xp", "it_moeda",
    "it_marmita", "it_bau", "cm_morte", "cm_explosao", "ic_tarja",
)


def refine(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    red, green, blue = (value / 255.0 for value in rgb)
    hue, saturation, value = colorsys.rgb_to_hsv(red, green, blue)
    saturation = min(1.0, saturation * 1.07)
    value = min(1.0, max(0.0, (value - 0.43) * 1.06 + 0.43))
    if value > 0.74:
        value = min(1.0, value + 0.02)
    elif value < 0.22:
        value = max(0.0, value - 0.01)
    return tuple(round(channel * 255) for channel in colorsys.hsv_to_rgb(hue, saturation, value))


def process(name: str) -> None:
    path = SPRITES / f"{name}.png"
    with Image.open(path) as source:
        image = source.convert("RGBA")
    alpha = image.getchannel("A").tobytes()
    mapping = {}
    output = []
    for pixel in image.getdata():
        if pixel[3] == 0:
            output.append(pixel)
            continue
        mapping.setdefault(pixel, (*refine(pixel[:3]), pixel[3]))
        output.append(mapping[pixel])
    image.putdata(output)
    if image.getchannel("A").tobytes() != alpha:
        raise RuntimeError(f"A máscara alpha mudou em {name}")
    image.save(path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("names", nargs="*", choices=FILES)
    args = parser.parse_args()
    for filename in args.names or FILES:
        process(filename)
