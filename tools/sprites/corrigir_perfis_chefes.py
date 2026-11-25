"""Corrige somente os perfis invertidos de Rene e IARA."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
SPRITES = ROOT / "code" / "assets" / "sprites" / "sobrevivencia"


def corrigir(nome: str, quadro: int) -> None:
    caminho = SPRITES / nome
    with Image.open(caminho) as source:
        tira = source.convert("RGBA")
    for indice in (*range(4, 8), *range(16, 20)):
        x = indice * quadro
        frame = tira.crop((x, 0, x + quadro, tira.height))
        tira.paste(frame.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (x, 0))
    tira.save(caminho)


if __name__ == "__main__":
    corrigir("bo_rene.png", 80)
    corrigir("bo_iara.png", 96)
