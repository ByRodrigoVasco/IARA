"""
Gera os cômodos do modo história: a imagem de cada cômodo e o arquivo com as colisões e as portas.

Cada cômodo é uma tela só (como em Zelda): o jogador só vê o cômodo em que está e, ao passar por
uma porta, a tela troca para o cômodo do outro lado. O escritório (office.png) é o cômodo central;
a porta marrom dele, ao lado do bebedouro, leva ao corredor, e do corredor saem os outros cômodos.

    escritório ── corredor ──┬── copa
                             ├── RH
                             ├── sala de reuniões
                             ├── TI
                             └── porão (escada)

Saídas:
    code/assets/sprites/environment/comodos/<cômodo>.png   (531 x 438, desenhado 2x no jogo = tamanho do office.png)
    code/src/game/rooms_data.h                              (colisões, portas e pontos de chegada)

As peças vêm de tools/historia/pecas (recortes dos pacotes Post Apoc de 0_mem0ry), dos objetos do
modo sobrevivência (code/assets/sprites/sobrevivencia/pr_*) e da decoração de tools/sprites/decoracao.
Os pisos reaproveitam as funções de tools/sprites/gerar_chao.py.

Uso, a partir da raiz do repositório:
    python tools/historia/gerar_comodos.py            gera tudo
    python tools/historia/gerar_comodos.py --previa   gera também docs/assets/comodos-historia.png

Precisa de Python 3 com Pillow e NumPy.
"""

import argparse
import importlib.util
from pathlib import Path

import numpy as np
from PIL import Image

RAIZ = Path(__file__).resolve().parents[2]
PECAS = Path(__file__).resolve().parent / "pecas"
SOBREVIVENCIA = RAIZ / "code/assets/sprites/sobrevivencia"
SAIDA_IMG = RAIZ / "code/assets/sprites/environment/comodos"
SAIDA_C = RAIZ / "code/src/game/rooms_data.h"

W, H = 531, 438          # Tamanho do cômodo em pixels de arte; no jogo cada pixel vira 2 (1062 x 876, como o office.png)
ESCALA = 2
FACE = 64                # Altura da parede do fundo (a parede vista de frente)
PAREDE = 6               # Espessura das paredes laterais e de baixo (vistas de cima)
VAZIO = (18, 14, 22)
CONTORNO = (22, 20, 26)

# Funções de piso e decoração do gerador do chão do modo sobrevivência
_spec = importlib.util.spec_from_file_location("gerar_chao", RAIZ / "tools/sprites/gerar_chao.py")
chao = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(chao)

# Direção para onde o jogador olha ao chegar (vira NORTH, SOUTH... de characters.h no arquivo em C)
NORTE, SUL, OESTE, LESTE = 0, 1, 2, 3


def peca(nome):
    """Peça pelo nome; com "~" no fim, espelhada na horizontal (ex.: cadeira de lado virada para o outro lado)."""
    if nome.endswith("~"):
        return peca(nome[:-1]).transpose(Image.FLIP_LEFT_RIGHT)
    for pasta in (PECAS, SOBREVIVENCIA, chao.PASTA_DECORACAO):
        arq = pasta / f"{nome}.png"
        if arq.exists():
            return Image.open(arq).convert("RGBA")
    raise FileNotFoundError(nome)


# -----------------------------------------------------------------------------
# Os cômodos
# -----------------------------------------------------------------------------
# interior: (x, y, largura, altura) do piso, em pixels de arte
# portas: (nome, lado, posição ao longo da parede, largura, destino, porta de destino, rótulo)
#   lado "cima": porta desenhada na parede do fundo; "baixo"/"esquerda"/"direita": vão na parede;
#   "escada": escada dentro do cômodo, encostada na parede da direita
# moveis: (peça, x do centro, y da base, colide)   -- a base é onde o móvel encosta no chão
# parede: (peça, x) penduradas na parede do fundo (quadros, relógio, janelas): não colidem

COMODOS = {
    "corredor": {
        "titulo": "CORREDOR",
        "interior": (40, 160, 451, 140),
        "piso": lambda r, h, w: chao.carpete(r, h, w, (96, 104, 122), (78, 86, 102)),
        "parede": "parede_creme",
        "portas": [
            ("escritorio", "baixo", 36, 52, "escritorio", "corredor", "ESCRITÓRIO"),
            ("copa", "cima", 46, 32, "copa", "corredor", "COPA"),
            ("rh", "cima", 152, 32, "rh", "corredor", "RH"),
            ("reunioes", "cima", 262, 64, "reunioes", "corredor", "SALA DE REUNIÕES"),
            ("ti", "baixo", 300, 52, "ti", "corredor", "TI"),
            ("porao", "escada", 0, 44, "porao", "corredor", "PORÃO"),
        ],
        # Parede livre entre as portas: 0-46, 78-152, 184-262 e 326-407 (depois vem a escada)
        "moveis": [
            ("planta_laranja", 22, 34, True), ("longarina", 115, 36, True), ("garrafao", 204, 30, True),
            ("lixeira_1", 240, 30, True), ("planta_cinza", 372, 34, True),
        ],
        "na_parede": [("relogio", 115), ("mural", 223), ("mod_quadro_3", 366)],
        "animacoes": [],
        "tapete": None,
    },
    # Copa: pacote Professional Kitchen. Bancada no fundo (geladeira, pia, micro-ondas, cafeteira) e duas mesinhas.
    "copa": {
        "titulo": "COPA",
        "interior": (96, 130, 340, 220),
        "piso": "coz_piso",
        "parede": "coz_parede",
        "portas": [("corredor", "baixo", 144, 52, "corredor", "copa", "CORREDOR")],
        "animacoes": [("vapor", "coz_cafeteira", 160, 24), ("vapor", "coz_cafeteira_2", 176, 24),
                      ("luz", "coz_microondas", 213, 22)],
        "moveis": [
            ("coz_geladeira_aberta", 24, 66, True), ("coz_balcao_portas", 72, 44, True), ("coz_pia_dupla", 119, 46, True),
            ("coz_balcao_gavetas", 166, 44, True), ("coz_balcao_liso", 213, 44, True), ("coz_balcao_porta", 252, 44, True),
            ("coz_liquidificador", 66, 24, False, 30), ("coz_cafeteira", 160, 24, False, 30), ("coz_cafeteira_2", 176, 24, False, 30),
            ("coz_microondas", 213, 22, False, 30), ("coz_batedeira", 252, 22, False, 30),
            ("pr_vending", 292, 48, True), ("garrafao", 326, 40, True),
            ("cla_cadeira", 80, 128, False), ("cla_cadeira", 260, 128, False),
            ("cla_mesinha", 80, 156, True), ("cla_mesinha", 260, 156, True),
            ("cla_cadeira_costas", 80, 176, True), ("cla_cadeira_costas", 260, 176, True),
            ("cla_xicaras", 80, 138, False, 30), ("coz_louca", 22, 214, True), ("coz_mantimentos", 22, 150, True),
            ("mod_planta", 324, 212, True), ("lixeira_2", 326, 120, True),
        ],
        "na_parede": [("coz_panelas_parede", 119), ("relogio", 240)],
        "tapete": None,
    },
    # RH: pacote Classic Furniture. Escrivaninha com abajur, diplomas na parede, sala de espera com sofá.
    "rh": {
        "titulo": "RH",
        "interior": (96, 130, 340, 220),
        "piso": "cla_piso",
        "parede": "cla_parede",
        "portas": [("corredor", "baixo", 144, 52, "corredor", "rh", "CORREDOR")],
        "moveis": [
            ("cla_estante", 18, 54, True), ("cla_estante", 50, 54, True), ("cla_armario", 84, 60, True),
            ("cla_estante", 290, 54, True), ("cla_estante", 322, 54, True),
            ("cla_cadeira_verde", 170, 92, False), ("cla_escrivaninha", 170, 118, True),
            ("cla_abajur", 152, 90, False, 40), ("cla_livro", 186, 92, False, 40),
            ("cla_cadeira_verde_costas", 150, 148, True), ("cla_cadeira_verde_costas", 190, 148, True),
            ("cla_gaveteiro", 240, 104, True), ("cla_planta", 118, 64, True),
            # Sala de espera: mesinha no meio, sofá abaixo dela olhando para cima e poltrona ao lado olhando para ela
            ("cla_mesinha", 56, 176, True), ("cla_sofa_costas", 56, 214, True), ("cla_poltrona_lado~", 16, 180, True),
            ("cla_poltrona_lado", 324, 176, True), ("cla_planta_alta", 300, 214, True), ("cla_planta_2", 326, 214, True),
        ],
        "animacoes": [],
        "na_parede": [("cla_diploma", 140), ("cla_quadro", 172), ("cla_diploma_2", 204), ("cla_quadrinho", 250)],
        "tapete": (112, 70, 116, 94, (110, 40, 36), (196, 150, 70)),
    },
    # Sala de reuniões: pacote Stylish Furniture. Mesa comprida, cadeiras pretas, TV no fundo e um cantinho com sofá.
    "reunioes": {
        "titulo": "SALA DE REUNIÕES",
        "interior": (66, 124, 400, 236),
        "piso": "mod_piso",
        "parede": "mod_parede",
        "portas": [("corredor", "baixo", 174, 52, "corredor", "reunioes", "CORREDOR")],
        "mesa": (110, 112, 180, 34),
        "animacoes": [("tv", "mod_tv", 200, 62)],
        "moveis": [
            ("mod_tv", 200, 62, True), ("mod_estante_vidro", 32, 60, True), ("mod_estante_alta", 66, 60, True),
            ("mod_rack", 336, 46, True), ("mod_orquidea", 320, 24, False, 30), ("mod_planta", 384, 52, True),
            ("mod_cadeira", 132, 124, False), ("mod_cadeira", 176, 124, False), ("mod_cadeira", 222, 124, False), ("mod_cadeira", 268, 124, False),
            ("mod_cadeira_costas", 132, 170, True), ("mod_cadeira_costas", 176, 170, True),
            ("mod_cadeira_costas", 222, 170, True), ("mod_cadeira_costas", 268, 170, True),
            ("mod_cadeira_lado~", 100, 146, True), ("mod_cadeira_lado", 300, 146, True),
            # Canto de conversa: sofá na parede da esquerda e poltrona, os dois virados para a mesinha de centro
            ("mod_sofa_lado~", 18, 230, True), ("mod_mesa_centro", 64, 222, True), ("mod_poltrona_lado", 108, 222, True),
            ("mod_planta_2", 146, 232, True), ("mod_planta_2", 330, 232, True), ("mod_luminaria", 386, 226, True),
        ],
        "na_parede": [("mod_quadro", 116), ("mod_quadro_3", 284)],
        "tapete": None,
    },
    "ti": {
        "titulo": "TI",
        "interior": (96, 140, 340, 220),
        "piso": lambda r, h, w: chao.vinilico(r, h, w) * 1.5,
        "parede": "parede_creme",
        "portas": [("corredor", "cima", 154, 32, "corredor", "ti", "CORREDOR")],
        "moveis": [
            ("pr_rackVivo", 30, 70, True), ("pr_rack", 84, 70, True), ("pr_rackVivo", 270, 70, True), ("pr_rack", 318, 70, True),
            ("pr_bancadaTI", 96, 160, True), ("mesa_cinza_1", 268, 150, True), ("pc_verde", 258, 120, False),
            ("cadeira_costas", 268, 176, True), ("pr_console", 216, 214, True),
            ("caixa_1", 20, 210, True), ("caixa_4", 44, 214, True), ("lixeira_1", 318, 206, True),
        ],
        "na_parede": [("janela_2", 100), ("relogio", 232)],
        "tapete": None,
        "cabos": 3,
        # O servidor vermelho (vivo) dá defeito de vez em quando; o rack azul pisca a atividade da rede
        "animacoes": [("servidor_vivo", "pr_rackVivo", 30, 70), ("rack", "pr_rack", 84, 70),
                      ("servidor_vivo", "pr_rackVivo", 270, 70), ("rack", "pr_rack", 318, 70),
                      ("console", "pr_console", 216, 214), ("monitor", "pc_verde", 258, 120)],
    },
    "porao": {
        "titulo": "PORÃO",
        "interior": (66, 124, 400, 250),
        "piso": lambda r, h, w: chao.concreto(r, h, w) * 0.9,
        "parede": "coz_parede",
        "parede_tom": 0.45,
        "portas": [("corredor", "escada_sobe", 0, 44, "corredor", "porao", "CORREDOR")],
        "moveis": [
            ("prateleira_mantimentos", 30, 70, True), ("armario_3", 82, 70, True), ("armario_1", 130, 70, True),
            ("caixote_1", 22, 140, True), ("caixote_2", 40, 170, True), ("caixote_3", 26, 200, True),
            ("bombona", 300, 210, True), ("lata", 318, 220, True), ("cofre", 230, 72, True),
            ("caixa_4", 372, 236, True), ("caixa_1", 352, 232, True),
        ],
        "na_parede": [],
        "tapete": None,
        "poco": (200, 174, 34),
        "animacoes": [("agua",), ("apagao",)],
        "sujeira": ["rachadura_1", "rachadura_2", "poeira_1", "poeira_3", "entulho", "mato_1", "gosma", "jornal_2", "pedra_2"],
    },
}

# Porta do escritório (office.png): a porta marrom ao lado do bebedouro, em coordenadas do office.png
# relativas ao centro (as mesmas das colisões do game_loop.c)
PORTA_ESCRITORIO = {"zona": (244.0, -344.0, 56.0, 24.0), "chegada": (272.0, -296.0), "olhar": SUL}


# -----------------------------------------------------------------------------
# Desenho
# -----------------------------------------------------------------------------

# Porta desenhada na parede do corredor: cada uma no estilo do cômodo que ela abre
ESTILO_PORTA = {"copa": "coz_porta", "rh": "cla_porta", "reunioes": "mod_porta_dupla"}


def destino_porta(comodo, pos):
    return next(d for _, lado, p, _, d, *_ in comodo["portas"] if lado == "cima" and p == pos)


def mesa_reuniao(img, x, y, w, h):
    """Mesa de reunião comprida, tampo escuro com borda clara, frente e pés (vista de cima, como o resto)."""
    arr = np.asarray(img).copy()
    arr[y:y + h, x:x + w] = (58, 56, 66, 255)
    arr[y + 2:y + h - 2, x + 2:x + w - 2] = (74, 72, 84, 255)
    arr[y + 3, x + 4:x + w - 4] = (104, 102, 116, 255)
    arr[y + h:y + h + 6, x:x + w] = (40, 38, 46, 255)
    for lx in (x + 3, x + w - 6):
        arr[y + h + 6:y + h + 10, lx:lx + 3] = (*CONTORNO, 255)
    arr[y, x:x + w] = arr[y + h + 5, x:x + w] = (*CONTORNO, 255)
    arr[y:y + h + 6, x] = arr[y:y + h + 6, x + w - 1] = (*CONTORNO, 255)
    return Image.fromarray(arr)


def sombra(img, cx, by, w):
    """Sombra suave embaixo do móvel, para ele assentar no chão."""
    arr = np.asarray(img).astype(float)
    yy, xx = np.ogrid[:H, :W]
    d = ((xx - cx) / (w * 0.55)) ** 2 + ((yy - by) / 4.0) ** 2
    m = d <= 1
    arr[m, :3] *= 0.72
    return Image.fromarray(arr.astype(np.uint8))


def colar(img, p, x, y):
    """Cola a peça com o canto de cima à esquerda em (x, y)."""
    img.alpha_composite(p, (int(round(x)), int(round(y))))


def desenhar_parede_fundo(img, comodo):
    x, y, w, h = comodo["interior"]
    tile = peca(comodo["parede"])
    if comodo.get("parede_tom"):            # Escurece a parede (o porão usa o azulejo da copa, sujo e sem luz)
        arr = np.asarray(tile).astype(float)
        arr[:, :, :3] *= comodo["parede_tom"]
        tile = Image.fromarray(arr.astype(np.uint8))
    for tx in range(x, x + w, tile.width):
        t = tile if tx + tile.width <= x + w else tile.crop((0, 0, x + w - tx, tile.height))
        colar(img, t, tx, y - FACE)


def linha_parede(arr, x0, y0, x1, y1, cor=(132, 130, 140)):
    c = (*CONTORNO, 255)[:arr.shape[2]]
    arr[y0:y1, x0:x1] = cor[:arr.shape[2]]
    arr[y0, x0:x1] = c
    arr[y1 - 1, x0:x1] = c
    arr[y0:y1, x0] = c
    arr[y0:y1, x1 - 1] = c


def escada(img, x, y, w, h, descendo=True):
    """Degraus vistos de cima: cada um mais escuro (descendo) ou mais claro (subindo)."""
    arr = np.asarray(img).copy()
    n = 6
    for i in range(n):
        k = i / (n - 1)
        tom = (1 - k) * 120 + k * 40 if descendo else k * 120 + (1 - k) * 40
        x0 = x + int(i * w / n)
        arr[y:y + h, x0:x + int((i + 1) * w / n)] = (tom * 0.95, tom * 0.92, tom, 255)
        arr[y:y + h, x0] = (*CONTORNO, 255)
    arr[y, x:x + w] = (*CONTORNO, 255)
    arr[y + h - 1, x:x + w] = (*CONTORNO, 255)
    return Image.fromarray(arr)


def poco(img, cx, cy, r):
    """Poço de inspeção: aro de concreto e água escura esverdeada, com reflexo."""
    arr = np.asarray(img).astype(float)
    yy, xx = np.ogrid[:H, :W]
    d = np.sqrt((xx - cx) ** 2 + ((yy - cy) * 1.4) ** 2)
    aro = (d <= r) & (d > r - 6)
    agua = d <= r - 6
    arr[aro] = (118, 116, 112, 255)
    arr[(d <= r) & (d > r - 1.2)] = (*CONTORNO, 255)
    arr[(d <= r - 5) & (d > r - 6.2)] = (60, 58, 56, 255)
    arr[agua] = (18, 54, 58, 255)
    rng = np.random.default_rng(7)
    for _ in range(14):
        a = rng.uniform(0, 6.28)
        rr = rng.uniform(4, r - 12)
        px, py = int(cx + np.cos(a) * rr), int(cy + np.sin(a) * rr / 1.4)
        arr[py, px - 2:px + 3] = (60, 130, 130, 255)
    return Image.fromarray(arr.astype(np.uint8))


def desenhar(nome, comodo, rng):
    x, y, w, h = comodo["interior"]
    base = np.zeros((H, W, 3)) + VAZIO
    if isinstance(comodo["piso"], str):
        t = np.asarray(peca(comodo["piso"]).convert("RGB")).astype(float)
        reps = (h // t.shape[0] + 1, w // t.shape[1] + 1, 1)
        base[y:y + h, x:x + w] = np.tile(t, reps)[:h, :w]
    else:
        base[y:y + h, x:x + w] = comodo["piso"](rng, h, w)
    if comodo.get("tapete"):
        tx, ty, tw, th, fundo, borda = comodo["tapete"]
        chao.tapete(base, rng, x + tx, y + ty, x + tx + tw, y + ty + th, fundo, borda)
    if comodo.get("cabos"):
        chao.cabos(base, rng, x + 10, y + 10, x + w - 10, y + h - 10, comodo["cabos"])
    if comodo.get("sujeira"):
        pecas = chao.carregar_pecas()
        for _ in range(26):
            p = pecas[comodo["sujeira"][int(rng.integers(0, len(comodo["sujeira"])))]]
            chao.carimbar(base, p, x + rng.uniform(16, w - 16), y + rng.uniform(16, h - 16))
    # Paredes laterais e de baixo, vistas de cima (com os vãos das portas)
    img = Image.fromarray(np.clip(base, 0, 255).astype(np.uint8)).convert("RGBA")
    desenhar_parede_fundo(img, comodo)
    arr = np.asarray(img).copy()
    vaos = {"baixo": [], "esquerda": [], "direita": []}
    for _, lado, pos, larg, *_ in comodo["portas"]:
        if lado in vaos:
            vaos[lado].append((pos, pos + larg))

    def com_vaos(inicio, fim, lista, desenha):
        cur = inicio
        for a, b in sorted(lista):
            if a > cur - inicio:
                desenha(cur, inicio + a)
            cur = inicio + b
        if cur < fim:
            desenha(cur, fim)

    cinza = (132, 130, 140, 255)
    com_vaos(x, x + w, vaos["baixo"], lambda a, b: linha_parede(arr, a, y + h, b, y + h + PAREDE, cinza))
    linha_parede(arr, x - PAREDE, y - FACE, x, y + h + PAREDE, cinza)
    linha_parede(arr, x + w, y - FACE, x + w + PAREDE, y + h + PAREDE, cinza)
    # Tapete de porta nos vãos de baixo
    for a, b in vaos["baixo"]:
        arr[y + h - 8:y + h, x + a + 4:x + b - 4] = (84, 70, 56, 255)
        arr[y + h - 8, x + a + 4:x + b - 4] = (*CONTORNO, 255)
    img = Image.fromarray(arr)

    # Portas desenhadas na parede do fundo e escadas
    for _, lado, pos, larg, *_ in comodo["portas"]:
        if lado == "cima":
            estilo = ESTILO_PORTA.get(destino_porta(comodo, pos), None)
            p = peca(estilo if estilo else ("porta_dupla" if larg >= 64 else "porta_marrom"))
            colar(img, p, x + pos, y - p.height)
        elif lado == "escada":
            img = escada(img, x + w - larg, y + 8, larg, h - 16, descendo=True)
        elif lado == "escada_sobe":
            img = escada(img, x + w - larg, y + 8, larg, 70, descendo=False)

    # Penduradas na parede
    for nome_p, px in comodo["na_parede"]:
        p = peca(nome_p)
        colar(img, p, x + px - p.width / 2, y - FACE + (FACE - p.height) / 2 - 6)

    if comodo.get("poco"):
        cx, cy, r = comodo["poco"]
        img = poco(img, x + cx, y + cy, r)


    # Móveis, de trás para a frente (o 5º campo, se houver, empurra a peça para frente na ordem: objeto em cima da mesa)
    for m in comodo["moveis"]:
        if m[3]:
            img = sombra(img, x + m[1], y + m[2] - 1, peca(m[0]).width)
    itens = list(comodo["moveis"])
    if comodo.get("mesa"):                      # A mesa entra na mesma ordem: cadeiras atrás dela ficam por baixo
        tx, ty, tw, th = comodo["mesa"]
        itens.append(("__mesa__", 0, ty + th, False))
    for m in sorted(itens, key=lambda m: m[2] + (m[4] if len(m) > 4 else 0)):
        if m[0] == "__mesa__":
            img = mesa_reuniao(img, x + tx, y + ty, tw, th)
            continue
        p = peca(m[0])
        colar(img, p, x + m[1] - p.width / 2, y + m[2] - p.height)
    return img


# -----------------------------------------------------------------------------
# Colisões e portas (em coordenadas do office.png relativas ao centro: pixel de arte * 2 - (531, 438))
# -----------------------------------------------------------------------------

def ref_rect(x, y, w, h):
    return (x * ESCALA - W, y * ESCALA - H, w * ESCALA, h * ESCALA)


def ref_ponto(x, y):
    return (x * ESCALA - W, y * ESCALA - H)


def colisoes(comodo):
    x, y, w, h = comodo["interior"]
    rects = [(0, 0, W, y)]                                   # parede do fundo e tudo acima
    rects.append((0, y, x, H - y))                           # esquerda
    rects.append((x + w, y, W - x - w, H - y))               # direita
    baixo = sorted((pos, pos + larg) for _, lado, pos, larg, *_ in comodo["portas"] if lado == "baixo")
    cur = x
    for a, b in baixo:
        if x + a > cur:
            rects.append((cur, y + h, x + a - cur, H - y - h))
        rects.append((x + a, y + h + 14, b - a, H - y - h - 14))   # depois do vão: vazio
        cur = x + b
    rects.append((cur, y + h, x + w - cur, H - y - h))
    for _, lado, pos, larg, *_ in comodo["portas"]:
        if lado in ("escada", "escada_sobe"):
            # A escada fica atrás de um corrimão: só dá para entrar por baixo dela (descendo) ou por baixo (subindo)
            alto = (h - 16) if lado == "escada" else 70
            rects.append((x + w - larg - 4, y, 4, alto - 26))
    if comodo.get("mesa"):
        tx, ty, tw, th = comodo["mesa"]
        rects.append((x + tx, y + ty + 8, tw, th + 2))
    for nome_p, mx, my, colide, *_ in comodo["moveis"]:
        if not colide:
            continue
        p = peca(nome_p)
        cw = p.width * 0.86
        ch = min(max(p.height * 0.3, 8), 18)
        rects.append((x + mx - cw / 2, y + my - ch, cw, ch))
    if comodo.get("poco"):
        cx, cy, r = comodo["poco"]
        rects.append((x + cx - r, y + cy - r / 1.4, 2 * r, 2 * r / 1.4))
    return [ref_rect(*r) for r in rects]


def portas(comodo):
    """Zona que leva a outro cômodo e ponto onde o jogador chega quando vem dela."""
    x, y, w, h = comodo["interior"]
    out = []
    for nome, lado, pos, larg, destino, porta_destino, rotulo in comodo["portas"]:
        if lado == "cima":
            zona = (x + pos, y, larg, 8)
            chegada, olhar = (x + pos + larg / 2, y + 26), SUL
        elif lado == "baixo":
            zona = (x + pos, y + h + 2, larg, 10)
            chegada, olhar = (x + pos + larg / 2, y + h - 22), NORTE
        elif lado == "escada":
            zona = (x + w - larg + 6, y + 20, larg - 10, h - 46)
            chegada, olhar = (x + w - larg - 26, y + h - 30), OESTE
        else:  # escada_sobe
            zona = (x + w - larg + 6, y + 12, larg - 10, 40)
            chegada, olhar = (x + w - larg / 2, y + 100), SUL
        out.append({"nome": nome, "zona": ref_rect(*zona), "chegada": ref_ponto(*chegada), "olhar": olhar,
                    "destino": destino, "porta_destino": porta_destino, "rotulo": rotulo})
    return out


# -----------------------------------------------------------------------------
# Animações: o script acha nas próprias peças os pixels que mexem (LEDs, olhos, telas)
# e o jogo desenha o efeito por cima da imagem do cômodo (rooms.c)
# -----------------------------------------------------------------------------

ANIM_C = {"leds": "ROOM_ANIM_LEDS", "onda": "ROOM_ANIM_LEDS_WAVE", "olhos": "ROOM_ANIM_EYES",
          "defeito": "ROOM_ANIM_GLITCH", "pisca": "ROOM_ANIM_BLINK", "tela": "ROOM_ANIM_SCREEN",
          "slides": "ROOM_ANIM_SLIDES", "vapor": "ROOM_ANIM_STEAM", "luz": "ROOM_ANIM_GLOW",
          "agua": "ROOM_ANIM_WATER", "apagao": "ROOM_ANIM_DARK_FLICKER"}


def _pixels(arr, cond, ox, oy):
    ys, xs = np.nonzero(cond)
    return [(ox + int(x), oy + int(y), *[int(c) for c in arr[y, x, :3]]) for y, x in zip(ys, xs)]


def maior_mancha(mascara):
    """Maior grupo de pixels vizinhos (4 direções) da máscara."""
    visto = np.zeros_like(mascara)
    melhor = []
    for y0, x0 in zip(*np.nonzero(mascara)):
        if visto[y0, x0]:
            continue
        pilha, grupo = [(y0, x0)], []
        visto[y0, x0] = True
        while pilha:
            y, x = pilha.pop()
            grupo.append((y, x))
            for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
                if 0 <= ny < mascara.shape[0] and 0 <= nx < mascara.shape[1] and mascara[ny, nx] and not visto[ny, nx]:
                    visto[ny, nx] = True
                    pilha.append((ny, nx))
        if len(grupo) > len(melhor):
            melhor = grupo
    saida = np.zeros_like(mascara)
    for y, x in melhor:
        saida[y, x] = True
    return saida


def _bbox(cond, ox, oy):
    ys, xs = np.nonzero(cond)
    if len(xs) == 0:
        return None
    return (ox + xs.min(), oy + ys.min(), xs.max() - xs.min() + 1, ys.max() - ys.min() + 1)


def analisar_animacoes(comodo, rng):
    """Lista de (tipo, área em pixels de arte, pixels [(x, y, r, g, b)])."""
    x0, y0, w, h = comodo["interior"]
    out = []
    for spec in comodo.get("animacoes", []):
        tipo = spec[0]
        if tipo == "agua":
            cx, cy, r = comodo["poco"]
            out.append(("agua", (x0 + cx - r + 6, y0 + cy - (r - 6) / 1.4, 2 * (r - 6), 2 * (r - 6) / 1.4), []))
            continue
        if tipo == "apagao":
            out.append(("apagao", (0, 0, W, H), []))
            continue
        nome, mx, my = spec[1:]
        p = peca(nome)
        a = np.asarray(p).astype(int)
        ox, oy = int(round(x0 + mx - p.width / 2)), int(round(y0 + my - p.height))
        r_, g_, b_, al = a[:, :, 0], a[:, :, 1], a[:, :, 2], a[:, :, 3] > 0
        vermelho = al & (r_ > 150) & (g_ < 100) & (b_ < 100)
        verde = al & (g_ > 140) & (r_ < 130)
        if tipo == "servidor_vivo":
            # Olhos: vermelho dentro do painel preto; LEDs: o resto do vermelho vivo
            preto = al & (a[:, :, :3].max(axis=2) < 16)
            painel = _bbox(maior_mancha(preto), 0, 0)          # O painel é a maior mancha preta
            px, py, pw, ph = painel
            dentro = np.zeros_like(vermelho)
            dentro[py:py + ph, px:px + pw] = True
            out.append(("onda", (ox, oy, p.width, p.height), _pixels(a, vermelho & ~dentro, ox, oy)))
            out.append(("olhos", (ox, oy, p.width, p.height), _pixels(a, vermelho & dentro, ox, oy)))
            out.append(("defeito", (ox, oy, p.width, p.height), []))
        elif tipo == "rack":
            out.append(("leds", (ox, oy, p.width, p.height), _pixels(a, vermelho | verde, ox, oy)))
        elif tipo == "console":
            metade = np.zeros_like(vermelho)
            metade[:, p.width // 2:] = True
            letras = al & (r_ > 200) & (r_ > g_ + 60) & (b_ < 160)                      # "ERRO" em vermelho claro
            out.append(("pisca", (ox, oy, p.width, p.height), _pixels(a, letras & metade, ox, oy)))
            tela = al & (g_ > 70) & (g_ > r_ + 25) & (g_ > b_ + 10) & ~metade
            out.append(("tela", _bbox(tela, ox, oy), []))
        elif tipo == "monitor":
            tela = al & (g_ > 80) & (b_ > 60) & (r_ < 30)
            out.append(("tela", _bbox(tela, ox, oy), []))
        elif tipo == "tv":
            tela = al & (a[:, :, :3].max(axis=2) < 40)
            out.append(("slides", _bbox(tela, ox, oy), []))
        elif tipo == "vapor":
            out.append(("vapor", (ox + p.width / 2 - 3, oy - 12, 6, 16), []))
        elif tipo == "luz":
            janela = al & (r_ > 200) & (g_ > 170) & (b_ < 140)
            out.append(("luz", _bbox(janela, ox, oy), []))
    return out


def animacoes_escritorio():
    """LEDs dos racks no canto de cima à direita do office.png (pixels coloridos e vivos)."""
    a = np.asarray(Image.open(RAIZ / "code/assets/sprites/environment/office.png").convert("RGB")).astype(int)
    x0, y0, x1, y1 = 920, 42, 1050, 97
    c = a[y0:y1, x0:x1]
    vivo = (c.max(axis=2) > 110) & (c.max(axis=2) - c.min(axis=2) > 60)
    ow, oh = 1062 / 2, 877 / 2
    pix = [(x - ow, y - oh, r, g, b) for (x, y, r, g, b) in _pixels(c, vivo, x0, y0)]
    return [("leds", (x0 - ow, y0 - oh, x1 - x0, y1 - y0), pix, 1.0)]


def anims_em_ref(lista):
    """Converte de pixels de arte do cômodo para o espaço de referência (pixel de arte = 2)."""
    out = []
    for tipo, area, pix in lista:
        out.append((tipo, ref_rect(*area), [(*ref_ponto(x, y), r, g, b) for x, y, r, g, b in pix], float(ESCALA)))
    return out


# -----------------------------------------------------------------------------
# Saída em C
# -----------------------------------------------------------------------------

ENUM = {"escritorio": "ROOM_ESCRITORIO", "corredor": "ROOM_CORREDOR", "copa": "ROOM_COPA", "rh": "ROOM_RH",
        "reunioes": "ROOM_REUNIOES", "ti": "ROOM_TI", "porao": "ROOM_PORAO"}


DIRECAO_C = {NORTE: "NORTH", SUL: "SOUTH", OESTE: "WEST", LESTE: "EAST"}   # Constantes de characters.h


def c_rect(r):
    return "{%.1ff, %.1ff, %.1ff, %.1ff}" % r


def escrever_c(dados):
    ordem = ["escritorio", "corredor", "copa", "rh", "reunioes", "ti", "porao"]
    indice_porta = {n: {p["nome"]: i for i, p in enumerate(d["portas"])} for n, d in dados.items()}
    linhas = ["// Gerado por tools/historia/gerar_comodos.py: não editar à mão (mude o script e gere de novo).",
              "// Coordenadas no espaço do office.png, relativas ao centro (as mesmas das colisões do game_loop.c).",
              "// Porta: zona que leva ao outro cômodo, cômodo e porta de destino, centro da hitbox do jogador ao chegar,",
              "// direção em que ele chega olhando e o nome que aparece perto da porta.",
              "#ifndef ROOMS_DATA_H", "#define ROOMS_DATA_H", "", '#include <stddef.h>', '#include "rooms.h"', ""]
    for n in ordem:
        d = dados[n]
        if d["colisoes"]:
            linhas.append(f"static const Rectangle room_obstacles_{n}[] = {{")
            linhas += [f"    {c_rect(r)}," for r in d["colisoes"]]
            linhas.append("};")
        linhas.append(f"static const room_door room_doors_{n}[] = {{")
        for p in d["portas"]:
            alvo = indice_porta[p["destino"]][p["porta_destino"]]
            linhas.append('    {%s, %s, %d, {%.1ff, %.1ff}, %s, "%s"},' % (
                c_rect(p["zona"]), ENUM[p["destino"]], alvo, p["chegada"][0], p["chegada"][1], DIRECAO_C[p["olhar"]], p["rotulo"]))
        linhas.append("};")
        for k, (tipo, area, pix, tam) in enumerate(d["anims"]):
            if pix:
                linhas.append(f"static const room_pixel room_pixels_{n}_{k}[] = {{")
                linhas += ["    " + " ".join("{%.1ff, %.1ff, %d, %d, %d}," % q for q in pix[i:i + 4]) for i in range(0, len(pix), 4)]
                linhas.append("};")
        if d["anims"]:
            linhas.append(f"static const room_anim room_anims_{n}[] = {{")
            for k, (tipo, area, pix, tam) in enumerate(d["anims"]):
                ref = f"room_pixels_{n}_{k}, (int)(sizeof(room_pixels_{n}_{k}) / sizeof(room_pixels_{n}_{k}[0]))" if pix else "NULL, 0"
                linhas.append("    {%s, %s, %s, %.1ff, %.2ff}," % (ANIM_C[tipo], c_rect(area), ref, tam, (k * 0.37 + len(n) * 0.61) % 1.0))
            linhas.append("};")
        linhas.append("")
    linhas.append("static const room_def room_defs[ROOM_COUNT] = {")
    for n in ordem:
        d = dados[n]
        img = f'"code/assets/sprites/environment/comodos/{n}.png"' if n != "escritorio" else "NULL"
        obs = f"room_obstacles_{n}, (int)(sizeof(room_obstacles_{n}) / sizeof(room_obstacles_{n}[0]))" if d["colisoes"] else "NULL, 0"
        anims = f"room_anims_{n}, (int)(sizeof(room_anims_{n}) / sizeof(room_anims_{n}[0]))" if d["anims"] else "NULL, 0"
        linhas.append(f'    [{ENUM[n]}] = {{"{d["titulo"]}", {img}, {obs}, room_doors_{n}, (int)(sizeof(room_doors_{n}) / sizeof(room_doors_{n}[0])), {anims}}},')
    linhas += ["};", "", "#endif // ROOMS_DATA_H", ""]
    SAIDA_C.write_text("\n".join(linhas), encoding="utf-8")


def gerar(previa=False):
    SAIDA_IMG.mkdir(parents=True, exist_ok=True)
    dados = {"escritorio": {"titulo": "ESCRITÓRIO", "colisoes": [], "portas": [{
        "nome": "corredor", "zona": PORTA_ESCRITORIO["zona"], "chegada": PORTA_ESCRITORIO["chegada"],
        "olhar": PORTA_ESCRITORIO["olhar"], "destino": "corredor", "porta_destino": "escritorio", "rotulo": "CORREDOR"}],
        "anims": animacoes_escritorio()}}
    imagens = []
    for i, (nome, comodo) in enumerate(COMODOS.items()):
        img = desenhar(nome, comodo, np.random.default_rng(100 + i))
        img.save(SAIDA_IMG / f"{nome}.png", optimize=True)
        imagens.append(img)
        dados[nome] = {"titulo": comodo["titulo"], "colisoes": colisoes(comodo), "portas": portas(comodo),
                       "anims": anims_em_ref(analisar_animacoes(comodo, None))}
    escrever_c(dados)
    if previa:
        cols = 3
        prev = Image.new("RGBA", (cols * W, ((len(imagens) + cols - 1) // cols) * H), (*VAZIO, 255))
        for k, im in enumerate(imagens):
            prev.alpha_composite(im, ((k % cols) * W, (k // cols) * H))
        prev.save(RAIZ / "docs/assets/comodos-historia.png")
    print(f"{len(imagens)} cômodos gerados em {SAIDA_IMG.relative_to(RAIZ)} e {SAIDA_C.relative_to(RAIZ)}")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--previa", action="store_true")
    gerar(p.parse_args().previa)
