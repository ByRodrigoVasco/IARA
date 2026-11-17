"""
Gera o chão do Modo Sobrevivência (code/assets/sprites/sobrevivencia/fl_R_C.png): piso de cada área,
sujeira, tapetes embaixo dos móveis de sala, decoração (peças em tools/sprites/decoracao, recortadas dos
pacotes Post Apoc de 0_mem0ry) e as paredes entre os cômodos, desenhadas no chão com vãos de porta.
As paredes são só visuais: a colisão do jogo continua só nos objetos (sv_map_data.h).

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
# Divisas entre áreas: a água invade o piso vizinho
# -----------------------------------------------------------------------------

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
# Paredes desenhadas no chão (só visuais: o jogo não bloqueia nada aqui)
# -----------------------------------------------------------------------------

PAREDE_ESPESSURA = 8
PORTA_LARGURA = 112
CONTORNO = np.array((22, 20, 26), float)
PAREDE = np.array((122, 120, 130), float)
PAREDE_LUZ = np.array((158, 156, 166), float)
BATENTE = np.array((112, 80, 52), float)
BATENTE_LUZ = np.array((150, 110, 72), float)


def sombra(img, ys, xs, k=0.72):
    img[ys, xs] = img[ys, xs] * k


def trecho_parede(img, x0, y0, x1, y1, vertical, rachada, rng):
    """Um trecho reto de parede vista de cima, com sombra dos dois lados."""
    t = PAREDE_ESPESSURA
    if vertical:
        xa = x0 - t // 2
        sombra(img, slice(y0, y1), slice(max(xa - 4, 0), xa))
        sombra(img, slice(y0, y1), slice(xa + t, min(xa + t + 4, img.shape[1])))
        img[y0:y1, xa:xa + t] = PAREDE
        img[y0:y1, xa + 1] = PAREDE_LUZ
        img[y0:y1, xa] = CONTORNO
        img[y0:y1, xa + t - 1] = CONTORNO
    else:
        ya = y0 - t // 2
        sombra(img, slice(max(ya - 4, 0), ya), slice(x0, x1))
        sombra(img, slice(ya + t, min(ya + t + 4, img.shape[0])), slice(x0, x1))
        img[ya:ya + t, x0:x1] = PAREDE
        img[ya + 1, x0:x1] = PAREDE_LUZ
        img[ya, x0:x1] = CONTORNO
        img[ya + t - 1, x0:x1] = CONTORNO
    if rachada:
        # Perto da água a parede está quebrada: falhas e rachaduras
        comp = (y1 - y0) if vertical else (x1 - x0)
        for _ in range(max(1, comp // 90)):
            k = int(rng.integers(0, max(comp - 14, 1)))
            n = int(rng.integers(6, 14))
            if vertical:
                img[y0 + k:y0 + k + n, x0 - t // 2 + 1:x0 + t // 2 - 1] = img[y0 + k:y0 + k + n, x0 - t // 2 + 1:x0 + t // 2 - 1] * 0.0 + np.array((40, 60, 62))
            else:
                img[y0 - t // 2 + 1:y0 + t // 2 - 1, x0 + k:x0 + k + n] = img[y0 - t // 2 + 1:y0 + t // 2 - 1, x0 + k:x0 + k + n] * 0.0 + np.array((40, 60, 62))


def batente(img, cx, cy):
    """Batente da porta: um pilarzinho de madeira em cada lado do vão."""
    h = 6
    img[cy - h:cy + h, cx - h:cx + h] = BATENTE
    img[cy - h + 1:cy - h + 3, cx - h + 1:cx + h - 1] = BATENTE_LUZ
    img[cy - h, cx - h:cx + h] = CONTORNO
    img[cy + h - 1, cx - h:cx + h] = CONTORNO
    img[cy - h:cy + h, cx - h] = CONTORNO
    img[cy - h:cy + h, cx + h - 1] = CONTORNO


def soleira(img, x0, y0, x1, y1, vertical):
    """Faixa de metal no chão do vão da porta."""
    if vertical:
        img[y0:y1, x0 - 2:x0 + 2] = (84, 84, 92)
        img[y0:y1, x0 - 1] = (104, 104, 112)
    else:
        img[y0 - 2:y0 + 2, x0:x1] = (84, 84, 92)
        img[y0 - 1, x0:x1] = (104, 104, 112)


def parede_com_porta(img, rng, x0, y0, x1, y1, vertical, rachada, portas=1):
    comp = (y1 - y0) if vertical else (x1 - x0)
    vaos = []
    for i in range(portas):
        centro = comp * (i + 1) / (portas + 1) + rng.uniform(-comp / 8, comp / 8)
        vaos.append((int(centro - PORTA_LARGURA / 2), int(centro + PORTA_LARGURA / 2)))
    pos = 0
    for a, b in vaos + [(comp, comp)]:
        if a > pos:
            if vertical:
                trecho_parede(img, x0, y0 + pos, x0, y0 + a, True, rachada, rng)
            else:
                trecho_parede(img, x0 + pos, y0, x0 + a, y0, False, rachada, rng)
        if b > a and a < comp:
            if vertical:
                soleira(img, x0, y0 + a, x0, y0 + b, True)
                batente(img, x0, y0 + a)
                batente(img, x0, y0 + b)
            else:
                soleira(img, x0 + a, y0, x0 + b, y0, False)
                batente(img, x0 + a, y0)
                batente(img, x0 + b, y0)
        pos = b


def paredes(img, rng, areas):
    t = PAREDE_ESPESSURA
    for r in range(ROWS):
        for c in range(COLS):
            a = areas[r][c]
            if c + 1 < COLS and areas[r][c + 1] != a:
                rachada = bool({a, areas[r][c + 1]} & MOLHADAS)
                parede_com_porta(img, rng, (c + 1) * CELL_W, r * CELL_H, (c + 1) * CELL_W, (r + 1) * CELL_H, True, rachada)
            if r + 1 < ROWS and areas[r + 1][c] != a:
                rachada = bool({a, areas[r + 1][c]} & MOLHADAS)
                parede_com_porta(img, rng, c * CELL_W, (r + 1) * CELL_H, (c + 1) * CELL_W, (r + 1) * CELL_H, False, rachada)
    # Contorno do prédio: parede inteira, sem portas
    trecho_parede(img, 0, t // 2, W, t // 2, False, False, rng)
    trecho_parede(img, 0, H - t // 2, W, H - t // 2, False, False, rng)
    trecho_parede(img, t // 2, 0, t // 2, H, True, False, rng)
    trecho_parede(img, W - t // 2, 0, W - t // 2, H, True, False, rng)
    # Pilares onde as paredes se encontram
    for r in range(ROWS + 1):
        for c in range(COLS + 1):
            x, y = min(max(c * CELL_W, t), W - t), min(max(r * CELL_H, t), H - t)
            lados = 0
            if 0 < r < ROWS and 0 < c <= COLS:
                lados += 1
            img[y - 6:y + 6, x - 6:x + 6] = PAREDE
            img[y - 6, x - 6:x + 6] = CONTORNO
            img[y + 5, x - 6:x + 6] = CONTORNO
            img[y - 6:y + 6, x - 6] = CONTORNO
            img[y - 6:y + 6, x + 5] = CONTORNO


# -----------------------------------------------------------------------------
# Objetos do mapa (para os tapetes ficarem embaixo deles e a decoração não sumir atrás)
# -----------------------------------------------------------------------------

def ler_objetos():
    mapa = (RAIZ / "code/src/survival/sv_map_data.h").read_text(encoding="utf-8")
    tabela = (RAIZ / "code/src/survival/sv_sprite_table.h").read_text(encoding="utf-8")
    tam = {n: (int(w), int(h)) for n, w, h in re.findall(r'X\((\w+), "\w+", (\d+), (\d+), \d+\)', tabela)}
    objs = []
    for n, x, y in re.findall(r"\{SPR_(\w+), ([\d.]+)f, ([\d.]+)f", mapa):
        w, h = tam.get(n, (32, 32))
        objs.append((n, float(x), float(y), w, h))
    return objs


TAPETE_CORES = {  # (fundo, borda) por área
    "REU": ((96, 40, 48), (150, 120, 60)), "DIR": ((70, 30, 36), (170, 140, 70)), "REC": ((60, 66, 96), (160, 160, 170)),
    "RHH": ((84, 60, 100), (170, 150, 190)), "OPE": ((56, 70, 90), (120, 130, 150)), "TRE": ((60, 60, 70), (120, 120, 130)),
    "FIN": ((46, 76, 60), (140, 150, 110)), "COP": ((110, 70, 50), (170, 140, 100)),
}
DEBAIXO_DE_TAPETE = {"pr_sofa", "pr_mesaExec", "pr_mesaRedonda", "pr_mesaEntrevista", "pr_mesaLeitura", "pr_mesaContas"}


def tapete(img, rng, x0, y0, x1, y1, fundo, borda):
    h, w = img.shape[:2]
    x0, y0, x1, y1 = max(int(x0), 2), max(int(y0), 2), min(int(x1), w - 2), min(int(y1), h - 2)
    if x1 - x0 < 16 or y1 - y0 < 16:
        return
    area = img[y0:y1, x0:x1]
    area[:] = area * 0.25 + np.array(fundo) * 0.75
    area += rng.normal(0, 2.0, area.shape[:2] + (1,))
    # Borda dupla e franja nas pontas
    for k, cor in ((0, CONTORNO), (3, borda), (4, borda)):
        area[k, k:area.shape[1] - k] = cor
        area[-1 - k, k:area.shape[1] - k] = cor
        area[k:area.shape[0] - k, k] = cor
        area[k:area.shape[0] - k, -1 - k] = cor
    for x in range(2, area.shape[1] - 2, 3):
        if y0 - 2 >= 0:
            img[y0 - 2:y0, x0 + x] = borda
        if y1 + 2 <= h:
            img[y1:y1 + 2, x0 + x] = borda
    # Desgaste: manchas mais claras
    for _ in range(3):
        cy, cx = int(rng.integers(6, area.shape[0] - 6)), int(rng.integers(6, area.shape[1] - 6))
        janela = disco(area, cy, cx, int(rng.integers(3, 8)))
        if janela is not None:
            janela[0][janela[1]] *= 1.12


def tapetes(img, rng, areas, objetos):
    for n, x, y, w, h in objetos:
        if n not in DEBAIXO_DE_TAPETE:
            continue
        a = areas[min(int(y // CELL_H), ROWS - 1)][min(int(x // CELL_W), COLS - 1)]
        if a not in TAPETE_CORES:
            continue
        fundo, borda = TAPETE_CORES[a]
        tapete(img, rng, x - w / 2 - 18, y - h * 0.75 - 6, x + w / 2 + 18, y + 14, fundo, borda)


def cabos(img, rng, x0, y0, x1, y1, quantos):
    """Cabos soltos no chão (data center e suporte): linhas em zigue-zague de ângulo reto."""
    cores = [(30, 30, 34), (40, 60, 120), (130, 40, 40), (200, 170, 60), (60, 120, 70)]
    for _ in range(quantos):
        cor = np.array(cores[int(rng.integers(0, len(cores)))])
        x, y = int(rng.integers(x0 + 20, x1 - 20)), int(rng.integers(y0 + 20, y1 - 20))
        for _ in range(int(rng.integers(3, 7))):
            horizontal = rng.random() < 0.5
            n = int(rng.integers(20, 90)) * (1 if rng.random() < 0.5 else -1)
            nx, ny = (x + n, y) if horizontal else (x, y + n)
            nx, ny = min(max(nx, x0 + 4), x1 - 4), min(max(ny, y0 + 4), y1 - 4)
            ya, yb, xa, xb = min(y, ny), max(y, ny), min(x, nx), max(x, nx)
            img[ya:yb + 2, xa:xb + 2] = CONTORNO
            img[ya:yb + 1, xa:xb + 1] = cor
            x, y = nx, ny


# -----------------------------------------------------------------------------
# Decoração: peças recortadas dos pacotes (tools/sprites/decoracao), achatadas no chão
# -----------------------------------------------------------------------------

PASTA_DECORACAO = Path(__file__).resolve().parent / "decoracao"
ESCRITORIO = ["papel_1", "papel_2", "papel_3", "papel_4", "papel_5", "papel_6", "papeis_1", "papeis_2", "papeis_3",
              "papeis_4", "papeis_5", "papeis_6", "pasta", "pasta_aberta", "caneta_1", "caneta_2", "caneta_3", "post_it",
              "caneca", "bolinha_papel_1", "bolinha_papel_2", "poeira_4", "sujeira_4"]
DECORACAO = {  # (peças, quantas por bloco)
    "OPE": (ESCRITORIO + ["telefone_1", "telefone_3", "cracha_1", "cracha_2", "disquete_1", "rachadura_1"], 16),
    "COP": (["caneca", "caneca", "garrafa_1", "garrafa_2", "lata", "lata", "bolinha_papel_1", "sujeira_1", "sujeira_2", "sujeira_3", "poeira_2"], 14),
    "REU": (ESCRITORIO + ["prancheta_1", "prancheta_2", "caderno", "livro_aberto_1"], 13),
    "DIR": (["livro_1", "livro_2", "livro_3", "livro_aberto_1", "livro_aberto_2", "livro_aberto_3", "prancheta_2", "telefone_2", "pasta", "caneta_1", "caneca", "foto"], 11),
    "DAT": (["disquete_1", "disquete_2", "entulho", "placa_raio", "rachadura_1", "rachadura_2", "poeira_1", "poeira_3"], 10),
    "TRE": (["caderno", "livro_1", "livro_2", "livro_3", "livro_aberto_2", "prancheta_1", "post_it", "papel_2", "papel_5", "caneta_2", "caneta_3"], 13),
    "FIN": (ESCRITORIO + ["nota_fiscal", "nota_fiscal", "telefone_1", "papeis_5", "papeis_6"], 16),
    "ARQ": (ESCRITORIO + ["jornal_1", "jornal_2", "jornal_3", "jornal_4", "papeis_1", "papeis_2", "papeis_3", "papeis_5", "bolinha_papel_2", "entulho", "poeira_3"], 26),
    "SUP": (["disquete_1", "disquete_2", "telefone_3", "telefone_2", "lata", "entulho", "caneca", "placa_raio", "poeira_2"], 12),
    "RHH": (["aviso_demissao", "aviso_demissao", "ficha", "documento", "cracha_1", "cracha_2", "cracha_2", "kit_socorro", "papeis_4", "pasta"], 14),
    "CAT": (["cracha_1", "cracha_2", "cracha_1", "placa_atencao", "placa_perigo", "rachadura_1", "rachadura_2", "entulho", "poeira_1"], 11),
    "REC": (["jornal_1", "jornal_2", "jornal_3", "jornal_4", "foto", "caneca", "garrafa_1", "sujeira_2", "poeira_4"], 11),
    "ALA": (["mato_1", "mato_2", "mato_3", "mato_4", "moita", "garrafa_1", "garrafa_2", "placa_atencao", "papel_6", "bolinha_papel_1"], 16),
    "FUN": (["gosma", "gosma", "moita", "moita", "mato_1", "mato_3", "mascara_gas", "rachadura_1", "rachadura_2"], 16),
}


def carregar_pecas():
    pecas = {}
    for arq in PASTA_DECORACAO.glob("*.png"):
        pecas[arq.stem] = np.asarray(Image.open(arq).convert("RGBA")).astype(float)
    return pecas


def carimbar(img, peca, cx, cy):
    """Cola a peça centrada em (cx, cy), respeitando a transparência."""
    ph, pw = peca.shape[:2]
    x0, y0 = int(cx - pw / 2), int(cy - ph / 2)
    if x0 < 2 or y0 < 2 or x0 + pw > img.shape[1] - 2 or y0 + ph > img.shape[0] - 2:
        return
    a = peca[:, :, 3:4] / 255.0
    img[y0:y0 + ph, x0:x0 + pw] = img[y0:y0 + ph, x0:x0 + pw] * (1 - a) + peca[:, :, :3] * a


def livre(x, y, objetos, folga=26):
    """Longe dos objetos (senão a peça some atrás deles) e das paredes."""
    if (x % CELL_W) < 18 or (x % CELL_W) > CELL_W - 18 or (y % CELL_H) < 18 or (y % CELL_H) > CELL_H - 18:
        return False
    for _, ox, oy, w, h in objetos:
        if ox - w / 2 - folga < x < ox + w / 2 + folga and oy - h - folga < y < oy + folga:
            return False
    return True


def decorar(img, rng, areas, objetos):
    pecas = carregar_pecas()
    for r in range(ROWS):
        for c in range(COLS):
            a = areas[r][c]
            nomes, quantas = DECORACAO[a]
            nomes = [n for n in nomes if n in pecas]
            x0, y0 = c * CELL_W, r * CELL_H
            if a in ("DAT", "SUP"):
                cabos(img, rng, x0, y0, x0 + CELL_W, y0 + CELL_H, 4 if a == "DAT" else 2)
            if a == "ALA":
                for _ in range(5):  # poças mais escuras e brilhantes
                    cy, cx = int(rng.integers(y0 + 30, y0 + CELL_H - 30)), int(rng.integers(x0 + 30, x0 + CELL_W - 30))
                    janela = disco(img, cy, cx, int(rng.integers(10, 24)))
                    if janela is not None:
                        janela[0][janela[1]] = janela[0][janela[1]] * 0.7 + np.array((20, 70, 80)) * 0.3
            colocadas, tentativas = 0, 0
            while colocadas < quantas and tentativas < quantas * 12:
                tentativas += 1
                x, y = x0 + rng.uniform(20, CELL_W - 20), y0 + rng.uniform(20, CELL_H - 20)
                if not livre(x, y, objetos):
                    continue
                peca = pecas[nomes[int(rng.integers(0, len(nomes)))]]
                giro = int(rng.integers(0, 4)) if peca.shape[0] < 20 and peca.shape[1] < 20 else 0
                if giro:
                    peca = np.rot90(peca, giro)
                if rng.random() < 0.5:
                    peca = peca[:, ::-1]
                carimbar(img, peca, x, y)
                colocadas += 1


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

    # 3. A água invade o piso vizinho
    borda_molhada(mundo, rng, areas)

    # 4. Tapetes embaixo de sofás e mesas, decoração espalhada e, por cima, as paredes com portas
    objetos = ler_objetos()
    tapetes(mundo, rng, areas, objetos)
    decorar(mundo, rng, areas, objetos)
    paredes(mundo, rng, areas)

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
