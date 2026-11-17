"""
Gera o chão do Modo Sobrevivência (code/assets/sprites/sobrevivencia/fl_R_C.png).

O mapa tem 8 x 6 blocos de 531 x 438 pixels. Cada bloco pertence a uma área (a mesma tabela
area_grid de code/src/survival/sv_game.c, lida direto do código para os dois nunca
divergirem). Cada área tem o seu piso, desenhado em ladrilhos de 32 x 32, escuro e de pouco
contraste: quem dá o clima é a iluminação do jogo (sv_light.c), e o chão precisa deixar os
inimigos legíveis por cima.

Uso, a partir da raiz do repositório:
    python tools/sprites/gerar_chao.py            gera os 48 blocos
    python tools/sprites/gerar_chao.py --previa   gera também docs/assets/chao-sobrevivencia.png (mapa inteiro reduzido)

Precisa de Python 3 com Pillow e NumPy (pip install pillow numpy).
"""

import argparse
import re
from pathlib import Path

import numpy as np
from PIL import Image

COLS, ROWS = 8, 6
CELL_W, CELL_H = 531, 438
W, H = COLS * CELL_W, ROWS * CELL_H
T = 32  # lado do ladrilho
SEED = 2026

RAIZ = Path(__file__).resolve().parents[2]
SAIDA = RAIZ / "code/assets/sprites/sobrevivencia"


# -----------------------------------------------------------------------------
# Tabela de áreas (lida do código)
# -----------------------------------------------------------------------------

def ler_areas():
    fonte = (RAIZ / "code/src/survival/sv_game.c").read_text(encoding="utf-8")
    nomes = re.search(r"enum \{ (A_OPE[^}]*)\}", fonte).group(1)
    nomes = [n.strip() for n in nomes.split(",") if n.strip()]
    bloco = re.search(r"area_grid\[SV_ROWS\]\[SV_COLS\] = \{(.*?)\};", fonte, re.S).group(1)
    linhas = re.findall(r"\{([^}]*)\}", bloco)
    return [[n.strip()[2:] for n in linha.split(",")] for linha in linhas]  # "A_OPE" -> "OPE"


# -----------------------------------------------------------------------------
# Pisos: cada função devolve uma imagem RGB (H x W x 3, float) do tamanho pedido
# -----------------------------------------------------------------------------

def disco(img, cy, cx, r):
    """Recorte da imagem em volta de (cy, cx) e a máscara do círculo de raio r dentro dele."""
    y0, y1, x0, x1 = max(cy - r, 0), min(cy + r + 1, img.shape[0]), max(cx - r, 0), min(cx + r + 1, img.shape[1])
    if y1 <= y0 or x1 <= x0:
        return None
    yy, xx = np.ogrid[y0:y1, x0:x1]
    return img[y0:y1, x0:x1], (yy - cy) ** 2 + (xx - cx) ** 2 < r * r


def ruido(rng, h, w, forca):
    return rng.normal(0.0, forca, (h, w, 1))


def carpete(rng, h, w, base, costura):
    """Carpete em placas de 32 com costura escura e textura fininha."""
    img = np.empty((h, w, 3)); img[:] = base
    img += ruido(rng, h, w, 2.2)
    # Fibras: pontinhos um pouco mais claros
    fibras = rng.random((h, w)) < 0.12
    img[fibras] += 5
    # Cada placa com um tom levemente diferente
    for y in range(0, h, T):
        for x in range(0, w, T):
            img[y:y + T, x:x + T] += rng.normal(0, 1.8)
    img[::T, :] = costura
    img[:, ::T] = costura
    return img


def ceramica(rng, h, w, base, rejunte, lado=T, brilho=0.0):
    img = np.empty((h, w, 3)); img[:] = base
    img += ruido(rng, h, w, 1.6)
    for y in range(0, h, lado):
        for x in range(0, w, lado):
            img[y:y + lado, x:x + lado] += rng.normal(0, 2.5)
            if brilho:
                # Reflexo na diagonal de cima de cada peça
                for k in range(3, lado // 2, 2):
                    yy, xx = y + k, x + 3
                    if yy < h and xx + 4 < w:
                        img[yy, xx:xx + 3] += brilho * (1 - k / lado)
    img[::lado, :] = rejunte
    img[:, ::lado] = rejunte
    img[1::lado, :] -= 4  # sombra do rejunte
    return img


def madeira(rng, h, w):
    """Tacos de madeira escura em fileiras de 8 px, emendas desencontradas."""
    tons = np.array([(78, 54, 40), (72, 48, 36), (84, 58, 42), (68, 46, 34)], float)
    img = np.empty((h, w, 3))
    altura = 8
    for i, y in enumerate(range(0, h, altura)):
        x = -int(rng.integers(0, 48))
        while x < w:
            comp = int(rng.integers(40, 96))
            x0, x1 = max(x, 0), min(x + comp, w)
            if x1 > x0:
                img[y:y + altura, x0:x1] = tons[rng.integers(0, len(tons))]
                # Veios
                veio = rng.random((min(altura, h - y), x1 - x0)) < 0.08
                img[y:y + altura, x0:x1][veio] -= 6
                img[y:y + altura, x0] = (42, 28, 22)
            x += comp
        img[y, :] = (46, 30, 24)
    img += ruido(rng, h, w, 1.2)
    return img


def piso_elevado(rng, h, w):
    """Data center: placas metálicas de 32 com furinhos de ventilação em algumas."""
    img = ceramica(rng, h, w, (66, 72, 82), (40, 44, 52))
    for y in range(0, h, T):
        for x in range(0, w, T):
            if rng.random() < 0.3:
                for yy in range(y + 6, min(y + T - 4, h), 4):
                    for xx in range(x + 6, min(x + T - 4, w), 4):
                        img[yy, xx] = (34, 38, 44)
    return img


def concreto(rng, h, w):
    img = np.empty((h, w, 3)); img[:] = (74, 72, 68)
    img += ruido(rng, h, w, 3.0)
    # Manchas grandes, bem suaves
    for _ in range(int(h * w / 9000)):
        cy, cx, r = int(rng.integers(0, h)), int(rng.integers(0, w)), int(rng.integers(10, 30))
        janela = disco(img, cy, cx, r)
        if janela is not None:
            janela[0][janela[1]] -= 4
    # Juntas de dilatação a cada 128
    img[::128, :] = (54, 52, 50)
    img[:, ::128] = (54, 52, 50)
    return img


def vinilico(rng, h, w):
    img = np.empty((h, w, 3))
    yy, xx = np.mgrid[:h, :w]
    xadrez = ((yy // 16) + (xx // 16)) % 2 == 0
    img[xadrez] = (58, 62, 60)
    img[~xadrez] = (50, 54, 53)
    img += ruido(rng, h, w, 1.5)
    return img


def agua(rng, h, w):
    """Área alagada: carpete afundado embaixo de uma lâmina d'água esverdeada."""
    fundo = carpete(rng, h, w, (40, 50, 58), (30, 38, 44))
    img = fundo * 0.75 + np.array((20, 46, 52)) * 0.45
    # Ondinhas: tracinhos claros horizontais
    for _ in range(int(h * w / 260)):
        y, x, n = rng.integers(0, h), rng.integers(0, w - 8), int(rng.integers(3, 8))
        img[y, x:x + n] += (16, 34, 36)
    return img


def organico(rng, h, w):
    """Fundo: o piso foi tomado por algas e veias vermelhas, perto do núcleo da IARA."""
    img = np.empty((h, w, 3)); img[:] = (28, 44, 46)
    img += ruido(rng, h, w, 2.5)
    for _ in range(int(h * w / 5000)):
        cy, cx, r = int(rng.integers(0, h)), int(rng.integers(0, w)), int(rng.integers(6, 22))
        janela = disco(img, cy, cx, r)
        if janela is not None:
            area, m = janela
            area[m] = area[m] * 0.6 + np.array((38, 74, 64)) * 0.4
    # Veias: caminhadas aleatórias
    for _ in range(int(h * w / 30000) + 2):
        y, x = float(rng.integers(0, h)), float(rng.integers(0, w))
        ang = rng.uniform(0, 6.28)
        for _ in range(int(rng.integers(40, 160))):
            ang += rng.normal(0, 0.35)
            y += np.sin(ang); x += np.cos(ang)
            iy, ix = int(y), int(x)
            if 0 <= iy < h and 0 <= ix < w:
                img[iy, ix] = (104, 30, 40)
    return img


PISOS = {
    "OPE": lambda r, h, w: carpete(r, h, w, (60, 66, 82), (46, 51, 64)),
    "TRE": lambda r, h, w: carpete(r, h, w, (64, 64, 68), (50, 50, 54)),
    "REU": lambda r, h, w: carpete(r, h, w, (72, 44, 52), (54, 32, 40)),
    "FIN": lambda r, h, w: carpete(r, h, w, (46, 64, 56), (34, 48, 42)),
    "RHH": lambda r, h, w: carpete(r, h, w, (62, 50, 76), (46, 36, 58)),
    "COP": lambda r, h, w: ceramica(r, h, w, (112, 102, 86), (74, 66, 56)),
    "REC": lambda r, h, w: ceramica(r, h, w, (100, 102, 108), (74, 76, 82), lado=64, brilho=10),
    "CAT": lambda r, h, w: ceramica(r, h, w, (78, 80, 88), (56, 58, 64), lado=64, brilho=8),
    "DIR": madeira,
    "DAT": piso_elevado,
    "ARQ": concreto,
    "SUP": vinilico,
    "ALA": agua,
    "FUN": organico,
}

# Quanta sujeira cada área tem (papéis, café, sangue) por bloco
SUJEIRA = {
    "OPE": (10, 4, 2), "TRE": (6, 2, 1), "REU": (6, 3, 1), "FIN": (12, 3, 1), "RHH": (14, 2, 3),
    "COP": (3, 8, 1), "REC": (5, 2, 2), "CAT": (4, 1, 3), "DIR": (6, 2, 2), "DAT": (2, 1, 1),
    "ARQ": (22, 2, 1), "SUP": (6, 4, 1), "ALA": (8, 0, 0), "FUN": (0, 0, 2),
}


# -----------------------------------------------------------------------------
# Sujeira
# -----------------------------------------------------------------------------

def papel(img, rng, y, x):
    h, w = (6, 8) if rng.random() < 0.5 else (8, 6)
    if y + h >= img.shape[0] or x + w >= img.shape[1]:
        return
    img[y:y + h, x:x + w] = (60, 58, 60)
    img[y + 1:y + h - 1, x + 1:x + w - 1] = (146, 140, 130)
    for k in range(2, h - 2, 2):  # linhas de texto
        img[y + k, x + 2:x + w - 2] = (112, 106, 100)


def mancha(img, rng, y, x, cor, raio, forca):
    hh, ww = img.shape[:2]
    for dy in range(-raio, raio + 1):
        for dx in range(-raio, raio + 1):
            d = dy * dy + dx * dx
            if d <= raio * raio and 0 <= y + dy < hh and 0 <= x + dx < ww and rng.random() < 0.92:
                k = forca * (1.4 if d > (raio - 1.5) ** 2 else 1.0)  # borda mais escura, como café seco
                img[y + dy, x + dx] = img[y + dy, x + dx] * (1 - k) + np.array(cor) * k


def sangue(img, rng, y, x):
    for _ in range(int(rng.integers(3, 8))):
        yy, xx = y + int(rng.integers(-8, 9)), x + int(rng.integers(-8, 9))
        mancha(img, rng, yy, xx, (96, 18, 24), int(rng.integers(1, 3)), 0.85)


def sujar(img, rng, area):
    papeis, cafes, sangues = SUJEIRA[area]
    h, w = img.shape[:2]
    for _ in range(papeis):
        papel(img, rng, int(rng.integers(4, h - 12)), int(rng.integers(4, w - 12)))
    for _ in range(cafes):
        mancha(img, rng, int(rng.integers(8, h - 8)), int(rng.integers(8, w - 8)), (52, 36, 26), int(rng.integers(4, 9)), 0.35)
    for _ in range(sangues):
        if rng.random() < 0.6:
            sangue(img, rng, int(rng.integers(10, h - 10)), int(rng.integers(10, w - 10)))


# -----------------------------------------------------------------------------
# Divisas entre áreas
# -----------------------------------------------------------------------------

def soleira_vertical(img, x, y0, y1):
    img[y0:y1, x - 2:x + 2] = (92, 92, 100)
    img[y0:y1, x - 3] = (30, 30, 36)
    img[y0:y1, x + 2] = (30, 30, 36)
    img[y0:y1, x - 1] = (112, 112, 120)


def soleira_horizontal(img, y, x0, x1):
    img[y - 2:y + 2, x0:x1] = (92, 92, 100)
    img[y - 3, x0:x1] = (30, 30, 36)
    img[y + 2, x0:x1] = (30, 30, 36)
    img[y - 1, x0:x1] = (112, 112, 120)


MOLHADAS = {"ALA", "FUN"}


def borda_molhada(img, rng, areas):
    """Onde a água encosta numa área seca, ela invade o piso vizinho com uma borda irregular."""
    for r in range(ROWS):
        for c in range(COLS):
            a = areas[r][c]
            if a not in MOLHADAS:
                continue
            for dr, dc in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                rr, cc = r + dr, c + dc
                if not (0 <= rr < ROWS and 0 <= cc < COLS) or areas[rr][cc] == a:
                    continue
                x0, y0 = c * CELL_W, r * CELL_H
                largura = 28
                if dc == 1:
                    xs, ys = slice(x0 + CELL_W, x0 + CELL_W + largura), slice(y0, y0 + CELL_H)
                elif dc == -1:
                    xs, ys = slice(x0 - largura, x0), slice(y0, y0 + CELL_H)
                elif dr == 1:
                    xs, ys = slice(x0, x0 + CELL_W), slice(y0 + CELL_H, y0 + CELL_H + largura)
                else:
                    xs, ys = slice(x0, x0 + CELL_W), slice(y0 - largura, y0)
                zona = img[ys, xs]
                hh, ww = zona.shape[:2]
                # Probabilidade de molhar cai com a distância da divisa
                if dc != 0:
                    dist = np.arange(ww)[None, :] if dc == 1 else np.arange(ww)[::-1][None, :]
                    dist = np.repeat(dist, hh, 0)
                else:
                    dist = np.arange(hh)[:, None] if dr == 1 else np.arange(hh)[::-1][:, None]
                    dist = np.repeat(dist, ww, 1)
                molhado = rng.random((hh, ww)) < (1 - dist / largura) ** 1.6
                cor = np.array((24, 50, 54)) if a == "ALA" else np.array((30, 54, 50))
                zona[molhado] = zona[molhado] * 0.55 + cor * 0.45


# -----------------------------------------------------------------------------
# Montagem
# -----------------------------------------------------------------------------

def gerar(previa=False):
    areas = ler_areas()
    rng = np.random.default_rng(SEED)
    mundo = np.zeros((H, W, 3))

    # 1. Cada área recebe o seu piso no retângulo que cobre todos os blocos dela,
    #    assim ladrilhos e tacos continuam de um bloco para o outro da mesma área
    for i, a in enumerate(sorted({a for linha in areas for a in linha})):
        celulas = [(r, c) for r in range(ROWS) for c in range(COLS) if areas[r][c] == a]
        r0, r1 = min(r for r, _ in celulas), max(r for r, _ in celulas) + 1
        c0, c1 = min(c for _, c in celulas), max(c for _, c in celulas) + 1
        piso = PISOS[a](np.random.default_rng(SEED + i), (r1 - r0) * CELL_H, (c1 - c0) * CELL_W)
        for r, c in celulas:
            py, px = (r - r0) * CELL_H, (c - c0) * CELL_W
            mundo[r * CELL_H:(r + 1) * CELL_H, c * CELL_W:(c + 1) * CELL_W] = piso[py:py + CELL_H, px:px + CELL_W]

    # 2. Sujeira por bloco
    for r in range(ROWS):
        for c in range(COLS):
            bloco = mundo[r * CELL_H:(r + 1) * CELL_H, c * CELL_W:(c + 1) * CELL_W]
            sujar(bloco, rng, areas[r][c])

    # 3. Divisas: água invade, áreas secas ganham soleira de metal
    borda_molhada(mundo, rng, areas)
    for r in range(ROWS):
        for c in range(COLS):
            a = areas[r][c]
            if c + 1 < COLS and areas[r][c + 1] != a and not ({a, areas[r][c + 1]} & MOLHADAS):
                soleira_vertical(mundo, (c + 1) * CELL_W, r * CELL_H, (r + 1) * CELL_H)
            if r + 1 < ROWS and areas[r + 1][c] != a and not ({a, areas[r + 1][c]} & MOLHADAS):
                soleira_horizontal(mundo, (r + 1) * CELL_H, c * CELL_W, (c + 1) * CELL_W)

    final = np.clip(mundo, 0, 255).astype(np.uint8)
    for r in range(ROWS):
        for c in range(COLS):
            bloco = final[r * CELL_H:(r + 1) * CELL_H, c * CELL_W:(c + 1) * CELL_W]
            Image.fromarray(bloco, "RGB").convert("RGBA").save(SAIDA / f"fl_{r}_{c}.png", optimize=True)
    if previa:
        destino = RAIZ / "docs/assets/chao-sobrevivencia.png"
        Image.fromarray(final, "RGB").resize((W // 3, H // 3), Image.NEAREST).save(destino)
        print(f"Prévia em {destino.relative_to(RAIZ)}")
    print(f"{ROWS * COLS} blocos gerados em {SAIDA.relative_to(RAIZ)}")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--previa", action="store_true", help="salva também uma prévia reduzida do mapa inteiro")
    gerar(p.parse_args().previa)
