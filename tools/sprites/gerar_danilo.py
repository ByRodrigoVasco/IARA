"""
Sprites do Prof. Danilo (PIF, Programação Imperativa e Funcional) no Modo Sobrevivência.

    python tools/sprites/gerar_danilo.py icone
        ic_recursao (24x24, 4 quadros): ícone da Recursão, uma árvore de chamadas f() acendendo nível a nível.
        fx_recursao (20x12, 1 quadro): o f() que voa (o jogo diminui as cópias a cada divisão).

    O personagem vem do redesenhar_danilo.py (feito a partir do Prof. Lucas, no mesmo estilo dos outros).

Precisa de Python 3 com Pillow e NumPy.
"""

import sys
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gerar_guilherme  # noqa: E402  (contorno)

RAIZ = Path(__file__).resolve().parents[2]
SOBREVIVENCIA = RAIZ / "code/assets/sprites/sobrevivencia"
VERDES = [(24, 90, 56, 255), (60, 190, 110, 255), (150, 245, 180, 255), (235, 255, 240, 255)]

# Letras de 8 px de altura: f, ( e ), como num terminal
LETRA_F = ["..##", ".#..", ".#..", "###.", ".#..", ".#..", ".#..", ".#.."]
ABRE = [".#", "#.", "#.", "#.", "#.", "#.", "#.", ".#"]
FECHA = ["#.", ".#", ".#", ".#", ".#", ".#", ".#", "#."]


def escrever(d, x, y, desenho, cor):
    for j, linha in enumerate(desenho):
        for i, c in enumerate(linha):
            if c == "#":
                d.point((x + i, y + j), fill=cor)


def chamada(d, x, y, cor, escala=1):
    """Escreve f() a partir de (x, y). Devolve a largura usada."""
    for dx, letra in ((0, LETRA_F), (5, ABRE), (8, FECHA)):
        for j, linha in enumerate(letra):
            for i, c in enumerate(linha):
                if c == "#":
                    px, py = x + (dx + i) * escala, y + j * escala
                    d.rectangle([px, py, px + escala - 1, py + escala - 1], fill=cor)
    return 10 * escala


def fx_recursao():
    im = Image.new("RGBA", (20, 12), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([1, 1, 18, 10], radius=2, fill=VERDES[0])
    d.line([(3, 1), (16, 1)], fill=VERDES[1])
    chamada(d, 5, 2, VERDES[2])
    gerar_guilherme.contornar(im).save(SOBREVIVENCIA / "fx_recursao.png")


def ic_recursao():
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        d.rectangle([1, 1, 22, 22], fill=gerar_guilherme.CONTORNO)
        d.rectangle([2, 2, 21, 21], fill=(14, 40, 28, 255))
        d.line([(2, 2), (21, 2)], fill=VERDES[0])
        # Árvore de chamadas: a raiz em cima, dois filhos, quatro netos. Cada quadro acende um nível.
        acesa = lambda nivel: VERDES[3] if f == nivel else VERDES[2] if f > nivel else VERDES[1]
        d.rectangle([9, 4, 14, 7], fill=acesa(0))
        d.line([(11, 8), (6, 11)], fill=VERDES[0])
        d.line([(12, 8), (17, 11)], fill=VERDES[0])
        for x in (4, 16):
            d.rectangle([x, 11, x + 3, 13], fill=acesa(1))
        for x in (2, 7, 14, 19):
            d.line([(x + 1, 14), (x + 1, 16)], fill=VERDES[0])
            d.rectangle([x, 17, x + 2, 18], fill=acesa(2))
        quadros.append(im)
    out = Image.new("RGBA", (96, 24), (0, 0, 0, 0))
    for i, q in enumerate(quadros):
        out.alpha_composite(q, (i * 24, 0))
    out.save(SOBREVIVENCIA / "ic_recursao.png")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "icone":
        fx_recursao()
        ic_recursao()
        print("ic_recursao.png e fx_recursao.png gerados")
    else:
        print(__doc__)
