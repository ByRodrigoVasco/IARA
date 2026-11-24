"""
Sprites parados do Prof. Danilo, redesenhados a partir dos do Prof. Lucas (mesmo corpo e mesma barba),
para ficarem no mesmo estilo dos outros personagens. Trocamos, pixel a pixel:
cabelo e barba castanho-escuros, óculos, sorriso, camisa verde-oliva com gola, bolsa carteiro preta
com a alça atravessando o peito, jeans azul-médio e tênis preto de biqueira branca.

    python tools/sprites/redesenhar_danilo.py [--previa]
    Depois: python tools/sprites/animar_caminhada.py Danilo

Precisa de Python 3 com Pillow e NumPy.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

RAIZ = Path(__file__).resolve().parents[2]
ORIGEM = RAIZ / "code/assets/sprites/PERSONAGENS/Lucas/rotations"
PASTA = RAIZ / "code/assets/sprites/PERSONAGENS/Danilo"

CONTORNO = (0, 0, 2)
OCULOS = (16, 12, 18)
BRILHO = (214, 232, 240)
DENTES = (246, 246, 240)
OLIVA, OLIVA_SOMBRA, OLIVA_LUZ, OLIVA_XADREZ = (118, 122, 62), (78, 82, 40), (156, 160, 92), (96, 100, 50)
BOLSA, BOLSA_LUZ = (30, 28, 34), (64, 62, 70)

# Cabelo e barba: do castanho-avermelhado do Lucas para castanho-escuro
CABELO = {
    (152, 88, 50): (88, 58, 40), (164, 87, 52): (98, 66, 46), (108, 58, 31): (70, 46, 32),
    (94, 47, 26): (60, 39, 28), (84, 41, 25): (52, 34, 25), (60, 29, 23): (44, 29, 22),
    (51, 24, 21): (38, 25, 20), (40, 19, 18): (30, 20, 17),
}
# Camiseta preta e o crachá do Lucas viram a camisa verde-oliva (só no tronco)
CAMISA = {
    (37, 37, 37): OLIVA, (21, 20, 25): OLIVA_SOMBRA, (30, 23, 29): OLIVA_SOMBRA,
    (89, 131, 195): OLIVA, (63, 85, 143): OLIVA_LUZ, (70, 95, 158): OLIVA, (235, 166, 17): OLIVA,
    (242, 248, 247): OLIVA, (128, 123, 118): OLIVA,
}
# Jeans escuro do Lucas para azul-médio
JEANS = {
    (50, 67, 80): (62, 98, 150), (83, 108, 121): (96, 134, 182), (41, 54, 69): (48, 76, 120),
    (38, 46, 62): (42, 64, 102), (60, 80, 93): (70, 108, 160),
}
# Sapato marrom para tênis preto com biqueira branca
TENIS = {(23, 14, 14): (26, 24, 30), (64, 47, 42): (236, 236, 232), (40, 19, 18): (26, 24, 30)}

PELE = {(247, 180, 126), (240, 168, 112), (233, 152, 102), (231, 144, 98)}


def recolorir(a, tabela, linhas):
    for y in linhas:
        for x in range(32):
            if a[y, x, 3] and tuple(a[y, x, :3]) in tabela:
                a[y, x, :3] = tabela[tuple(a[y, x, :3])]


def tronco(a):
    """Caixa da camisa: pixels verde-oliva entre as linhas 15 e 24."""
    ys, xs = np.nonzero(np.isin(a[15:25, :, 0], [OLIVA[0], OLIVA_SOMBRA[0], OLIVA_LUZ[0]]) & (a[15:25, :, 3] > 0))
    return xs.min(), 15 + ys.min(), xs.max(), 15 + ys.max()


def linha(a, x0, y0, x1, y1, cor):
    n = max(abs(x1 - x0), abs(y1 - y0))
    for i in range(n + 1):
        x, y = round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n)
        if a[y, x, 3]:
            a[y, x, :3] = cor


def bolsa(a, x, y):
    for yy in range(y, y + 5):
        for xx in range(x, x + 5):
            borda = yy in (y, y + 4) or xx in (x, x + 4)
            a[yy, xx] = (*(CONTORNO if borda else BOLSA_LUZ if yy == y + 1 else BOLSA), 255)


def xadrez(a, caixa):
    """Quadradinhos escuros espaçados, sem poluir: lembra o xadrez miúdo da camisa."""
    x0, y0, x1, y1 = caixa
    for y in range(y0 + 1, y1 + 1, 2):
        for x in range(x0, x1 + 1):
            if (x + y) % 4 == 0 and tuple(a[y, x, :3]) == OLIVA:
                a[y, x, :3] = OLIVA_XADREZ


def oculos(a, olho_y, lentes, haste=None):
    """Óculos legíveis em 32 px: aro de cima escuro (linha olho_y-1), ponte na linha do olho e um
    pixel de brilho claro em cada lente, ao lado do olho. lentes = [(x_esq, x_dir)];
    haste = (x0, x1) liga o aro até a orelha (vista de lado)."""
    for xa, xb in lentes:
        for x in range(xa, xb + 1):
            if a[olho_y - 1, x, 3]:
                a[olho_y - 1, x, :3] = OCULOS
        a[olho_y, xb, :3] = BRILHO
    # Sobrancelhas pretas do Lucas ficam castanhas, para não somar com o aro e virar uma faixa
    for x in range(lentes[0][0], lentes[-1][1] + 1):
        if a[olho_y - 2, x, 3] and int(a[olho_y - 2, x, :3].sum()) < 60:
            a[olho_y - 2, x, :3] = (88, 58, 40)
    if len(lentes) == 2:
        for x in range(lentes[0][1] + 1, lentes[1][0]):
            a[olho_y, x, :3] = OCULOS
    if haste:
        for x in range(haste[0], haste[1] + 1):
            a[olho_y - 1, x, :3] = OCULOS


def sorriso(a, y, xs):
    for x in xs:
        a[y, x, :3] = DENTES


def vista(nome):
    a = np.asarray(Image.open(ORIGEM / f"{nome}.png").convert("RGBA")).copy()
    recolorir(a, CABELO, range(0, 15))
    recolorir(a, CAMISA, range(15, 25))
    recolorir(a, JEANS, range(23, 31))
    recolorir(a, TENIS, range(26, 31))
    for y in range(15, 23):
        for x in range(32):
            r, g, b = (int(v) for v in a[y, x, :3])
            if a[y, x, 3] and b > r + 30 and b > 90:
                a[y, x, :3] = OLIVA
    caixa = tronco(a)
    xadrez(a, caixa)
    x0, y0, x1, y1 = caixa

    if nome == "south":
        oculos(a, 10, [(12, 14), (16, 18)], haste=(11, 11))
        sorriso(a, 13, range(15, 18))
        linha(a, x0 + 2, y0 + 1, x1 - 2, y1 - 1, CONTORNO)      # Alça do ombro direito ao quadril esquerdo
        bolsa(a, x1 - 3, y1 - 2)
        a[y0 + 1, 13:18, :3] = OLIVA_LUZ                        # Gola
    elif nome == "south-east":
        oculos(a, 10, [(14, 17), (19, 21)])
        sorriso(a, 13, range(16, 19))
        linha(a, x0 + 2, y0 + 1, x1 - 1, y1 - 1, CONTORNO)
        bolsa(a, x1 - 2, y1 - 2)
        a[y0 + 1, 14:19, :3] = OLIVA_LUZ
    elif nome == "east":
        oculos(a, 10, [(17, 19)], haste=(13, 16))
        sorriso(a, 12, range(19, 21))
        linha(a, x0 + 3, y0 + 1, x0 + 4, y1 - 1, CONTORNO)
        bolsa(a, x0 + 3, y1 - 2)
    elif nome == "north-east":
        linha(a, x1 - 2, y0 + 1, x0 + 2, y1 - 1, CONTORNO)
        bolsa(a, x0 - 1, y1 - 2)
        a[10, x1 - 1, :3] = OCULOS                              # Haste dos óculos aparecendo na lateral
    elif nome == "north":
        linha(a, x1 - 2, y0 + 1, x0 + 2, y1 - 1, CONTORNO)
        bolsa(a, x0 - 1, y1 - 2)
    return Image.fromarray(a)


def main():
    (PASTA / "rotations").mkdir(parents=True, exist_ok=True)
    (PASTA / "walk").mkdir(parents=True, exist_ok=True)
    vistas = {n: vista(n) for n in ("south", "south-east", "east", "north-east", "north")}
    vistas["south-west"] = vistas["south-east"].transpose(Image.FLIP_LEFT_RIGHT)
    vistas["north-west"] = vistas["north-east"].transpose(Image.FLIP_LEFT_RIGHT)
    vistas["west"] = vistas["east"].transpose(Image.FLIP_LEFT_RIGHT)
    for n, im in vistas.items():
        im.save(PASTA / "rotations" / f"{n}.png")
    if "--previa" in sys.argv:
        ordem = ["south", "south-east", "east", "north-east", "north"]
        out = Image.new("RGBA", (5 * 200 + 10, 210), (70, 66, 80, 255))
        for i, n in enumerate(ordem):
            out.alpha_composite(vistas[n].resize((192, 192), Image.NEAREST), (10 + i * 200, 9))
        out.save("/tmp/danilo_previa.png")
    print("rotations do Danilo geradas")


if __name__ == "__main__":
    main()
