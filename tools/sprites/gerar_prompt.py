"""
Desenha os sprites do Prompt da IARA, o poder inicial do Prof. Lucas no Modo Sobrevivência:

    ic_prompt    24x24, 4 quadros: ícone do poder (terminal com ">_" e o cursor piscando)
    fx_terminal  120x36, 2 quadros: moldura da janela que digita o comando (0 normal, 1 no Enter, mais clara)

O comando digitado e a chuva de letras continuam sendo desenhados pelo jogo (sv_render.c), por cima da moldura.

Uso, a partir da raiz do repositório:
    python tools/sprites/gerar_prompt.py

Precisa de Python 3 com Pillow e NumPy.
"""

from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

RAIZ = Path(__file__).resolve().parents[2]
SAIDA = RAIZ / "code/assets/sprites/sobrevivencia"

CONTORNO = (8, 12, 18, 255)
TELA = [(6, 18, 26, 255), (10, 30, 40, 255)]
MOLDURA = [(14, 74, 82, 255), (24, 124, 128, 255), (56, 196, 192, 255), (150, 250, 240, 255)]
TEXTO = (120, 255, 240, 255)
BRANCO = (224, 255, 248, 255)
VERMELHO = (232, 36, 44, 255)
AMARELO = (232, 194, 74, 255)

# Fonte pixel 3x5 só com as letras que aparecem
FONTE = {
    "I": ["###", ".#.", ".#.", ".#.", "###"],
    "A": [".#.", "#.#", "###", "#.#", "#.#"],
    "R": ["##.", "#.#", "##.", "#.#", "#.#"],
    ">": ["#..", ".#.", "..#", ".#.", "#.."],
    "_": ["...", "...", "...", "...", "###"],
}


def escrever(img, x, y, texto, cor):
    px = img.load()
    for c in texto:
        for dy, linha in enumerate(FONTE[c]):
            for dx, v in enumerate(linha):
                if v == "#":
                    px[x + dx, y + dy] = cor
        x += 4


def ic_prompt():
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        d.rectangle([1, 3, 22, 21], fill=CONTORNO)
        d.rectangle([2, 4, 21, 20], fill=MOLDURA[1])
        d.rectangle([2, 4, 21, 7], fill=MOLDURA[2])                 # Barra de título
        d.line([(2, 4), (21, 4)], fill=MOLDURA[3])
        d.point((4, 5), fill=VERMELHO)
        d.point((6, 5), fill=AMARELO)
        d.rectangle([3, 8, 20, 19], fill=TELA[0])                   # Tela
        d.line([(3, 8), (20, 8)], fill=TELA[1])
        escrever(im, 5, 11, ">", TEXTO)
        if f in (0, 1):                                             # Cursor piscando
            d.rectangle([10, 15, 12, 15], fill=BRANCO)
        else:
            d.rectangle([10, 15, 12, 15], fill=TEXTO[:3] + (90,))
        # Linhas de "resposta" aparecendo uma a uma
        for k in range(min(f, 2)):
            d.line([(14, 11 + k * 3), (18 - k, 11 + k * 3)], fill=MOLDURA[2])
        quadros.append(im)
    out = Image.new("RGBA", (24 * 4, 24), (0, 0, 0, 0))
    for i, q in enumerate(quadros):
        out.alpha_composite(q, (i * 24, 0))
    return out


def fx_terminal():
    quadros = []
    for f in range(2):
        claro = f == 1
        im = Image.new("RGBA", (120, 36), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        d.rectangle([0, 0, 119, 35], fill=CONTORNO)
        borda = MOLDURA[3] if claro else MOLDURA[2]
        d.rectangle([1, 1, 118, 34], fill=MOLDURA[1])
        d.line([(1, 1), (118, 1)], fill=borda)                      # Luz em cima e à esquerda (moldura com relevo)
        d.line([(1, 1), (1, 34)], fill=borda)
        d.line([(2, 34), (118, 34)], fill=MOLDURA[0])
        d.line([(118, 2), (118, 34)], fill=MOLDURA[0])
        # Barra de título com os três botões e o nome
        d.rectangle([2, 2, 117, 9], fill=MOLDURA[2] if not claro else MOLDURA[3])
        for k, cor in enumerate((VERMELHO, AMARELO, AMARELO)):
            d.rectangle([5 + k * 6, 4, 7 + k * 6, 6], fill=cor)
        escrever(im, 102, 3, "IARA", BRANCO if not claro else CONTORNO)
        # Tela com moldura interna escura
        d.rectangle([3, 11, 116, 32], fill=CONTORNO)
        d.rectangle([4, 12, 115, 31], fill=TELA[1] if claro else TELA[0])
        # Linhas de varredura bem fracas, como monitor antigo
        for y in range(13, 31, 2):
            d.line([(4, y), (115, y)], fill=(16, 40, 50, 255) if claro else (8, 24, 32, 255))
        quadros.append(im)
    out = Image.new("RGBA", (240, 36), (0, 0, 0, 0))
    for i, q in enumerate(quadros):
        out.alpha_composite(q, (i * 120, 0))
    return out


if __name__ == "__main__":
    ic_prompt().save(SAIDA / "ic_prompt.png")
    fx_terminal().save(SAIDA / "fx_terminal.png")
    print("ic_prompt.png e fx_terminal.png gerados em", SAIDA.relative_to(RAIZ))
