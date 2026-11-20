"""
Refaz a animação de caminhada de um personagem (4 quadros por direção) a partir da pose parada
dele (rotations/<direção>.png), para quando a animação existente quase não mexe as pernas.

O ciclo tem dois passos e dois quadros de "passagem":
    0: passo (pé direito no ar / perna da frente esticada para a frente)
    1: passagem, com o corpo 1 px mais alto (o balanço do andar)
    2: passo com o outro pé
    3: passagem, de novo com o corpo mais alto

De frente e de costas, os pés se alternam no ar. De lado, as pernas abrem em "V" (a de trás um
pouco mais escura). Nas diagonais, o pé no ar também dá um passo para o lado em que ele anda.

Uso, a partir da raiz do repositório:
    python tools/sprites/animar_caminhada.py Lucas            (cintura na linha 25, o padrão)
    python tools/sprites/animar_caminhada.py Lucas --cintura 25 --previa

Grava em code/assets/sprites/PERSONAGENS/<personagem>/walk/<direção>-<0..3>.png.
Precisa de Python 3 com Pillow e NumPy.
"""

import argparse
from pathlib import Path

from collections import Counter

import numpy as np
from PIL import Image, ImageDraw

RAIZ = Path(__file__).resolve().parents[2]
PERSONAGENS = RAIZ / "code/assets/sprites/PERSONAGENS"

# Direção: (tipo, para que lado o passo vai no x)
DIRECOES = {
    "south": ("frente", 0), "north": ("frente", 0), "east": ("lado", 1),
    "south-east": ("diagonal", 1), "north-east": ("diagonal", 1),
    "south-west": ("diagonal", -1), "north-west": ("diagonal", -1),
}


def separar_pernas(a, cintura):
    """Divide a área das pernas em esquerda e direita pela coluna mais vazia entre elas."""
    pernas = a[cintura:, :, 3] > 0
    cols = np.nonzero(pernas.any(0))[0]
    x0, x1 = cols.min(), cols.max()
    topo = pernas[:3, x0 + 1:x1]                       # As coxas, logo abaixo da cintura
    meio = x0 + 1 + int(np.argmin(topo.sum(0))) if topo.size else (x0 + x1) // 2
    esquerda = np.zeros(a.shape[:2], bool)
    direita = np.zeros(a.shape[:2], bool)
    esquerda[cintura:, :meio] = a[cintura:, :meio, 3] > 0
    direita[cintura:, meio:] = a[cintura:, meio:, 3] > 0
    return esquerda, direita


def mover(a, mascara, dx, dy, escurecer=1.0):
    """Copia os pixels da máscara deslocados (dx, dy), opcionalmente mais escuros."""
    out = np.zeros_like(a)
    ys, xs = np.nonzero(mascara)
    ny, nx = ys + dy, xs + dx
    ok = (ny >= 0) & (ny < a.shape[0]) & (nx >= 0) & (nx < a.shape[1])
    px = a[ys[ok], xs[ok]].copy()
    px[:, :3] = (px[:, :3] * escurecer).astype(px.dtype)
    out[ny[ok], nx[ok]] = px
    return out


def por_cima(base, camada):
    m = camada[:, :, 3] > 0
    base[m] = camada[m]
    return base


def inclinar(a, mascara, cintura, desloc, escurecer=1.0):
    """Perna inclinada: cada linha anda mais para o lado quanto mais perto do pé (desloc no pé)."""
    out = np.zeros_like(a)
    ys = np.nonzero(mascara.any(1))[0]
    if len(ys) == 0:
        return out
    pe = ys.max()
    for y in ys:
        dx = int(round(desloc * (y - cintura + 1) / max(1, pe - cintura + 1)))
        linha = np.zeros_like(mascara)
        linha[y] = mascara[y]
        por_cima(out, mover(a, linha, dx, 0, escurecer))
    return out


def cores_das_pernas(a, cintura):
    """Cor da calça (clara, média, escura) e do sapato, tiradas das próprias pernas do personagem."""
    def comuns(linhas):
        px = [tuple(c[:3]) for c in a[linhas].reshape(-1, 4) if c[3] > 0 and max(c[:3]) > 12]
        return [c for c, _ in Counter(px).most_common()]
    calca = sorted(comuns(slice(cintura, cintura + 3))[:3], key=lambda c: sum(int(v) for v in c))
    while len(calca) < 3:
        calca.append(calca[-1])
    sapato = sorted(comuns(slice(a.shape[0] - 4, a.shape[0]))[:2], key=lambda c: sum(int(v) for v in c))
    return calca, sapato or [(30, 20, 20)]


def pernas_de_lado(a, cintura, lado):
    """Desenha as duas pernas vistas de lado, abertas em V: a de trás mais escura, a da frente na cor normal."""
    cheio = a[:, :, 3] > 0
    linhas = np.nonzero(cheio[cintura:].any(1))[0]
    pe = cintura + linhas.max() if len(linhas) else a.shape[0] - 2
    cols = np.nonzero(cheio[cintura])[0]
    quadril = (cols.min() + cols.max()) / 2.0 if len(cols) else a.shape[1] / 2.0
    calca, sapato = cores_das_pernas(a, cintura)
    im = Image.new("RGBA", (a.shape[1], a.shape[0]), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for desloc, escuro in ((-2 * lado, 0.7), (3 * lado, 1.0)):     # Primeiro a de trás, depois a da frente
        cor = tuple(int(c * escuro) for c in calca[1]) + (255,)
        luz = tuple(int(c * escuro) for c in calca[2]) + (255,)
        fx = quadril + desloc
        d.line([(quadril, cintura), (fx, pe - 1)], fill=cor, width=3)
        d.line([(quadril + lado, cintura), (fx + lado, pe - 2)], fill=luz, width=1)
        bico = int(round(fx)) + (lado if lado else 1)
        d.rectangle([min(int(round(fx)) - 1, bico), pe - 1, max(int(round(fx)) - 1, bico) + 1, pe],
                    fill=tuple(int(c * escuro) for c in sapato[0]) + (255,))
    b = np.asarray(im).copy()
    # Contorno escuro só por fora das pernas novas
    cheio_b = b[:, :, 3] > 0
    viz = np.zeros_like(cheio_b)
    viz[1:, :] |= cheio_b[:-1, :]
    viz[:-1, :] |= cheio_b[1:, :]
    viz[:, 1:] |= cheio_b[:, :-1]
    viz[:, :-1] |= cheio_b[:, 1:]
    contorno = viz & ~cheio_b
    contorno[:cintura] = False
    b[contorno] = (0, 0, 2, 255)
    return b


def quadro(a, cintura, tipo, lado, passo, balanco):
    corpo = a.copy()
    corpo[cintura:] = 0
    pernas = np.zeros_like(a)
    esquerda, direita = separar_pernas(a, cintura)
    coxa = np.zeros(a.shape[:2], bool)

    if passo is None:                                   # Passagem: pernas juntas
        por_cima(pernas, a * (a[:, :, 3:4] > 0) * 1)
        pernas[:cintura] = 0
    elif tipo == "lado":
        por_cima(pernas, pernas_de_lado(a, cintura, lado))
    else:
        no_ar = direita if passo == 0 else esquerda
        no_chao = esquerda if passo == 0 else direita
        # Pé no ar: a perna sobe 1 px (a coxa fica, só a canela e o pé sobem)
        coxa[cintura:cintura + 1] = True
        por_cima(pernas, mover(a, no_chao, 0, 0))
        dx = lado if tipo == "diagonal" else 0
        por_cima(pernas, mover(a, no_ar & coxa, 0, 0))
        por_cima(pernas, mover(a, no_ar & ~coxa, dx, -2))

    out = np.zeros_like(a)
    if balanco:
        # Corpo 1 px mais alto e a cintura esticada para não abrir buraco
        por_cima(out, pernas)
        por_cima(out, mover(a, np.pad(np.ones((cintura, a.shape[1]), bool), ((0, a.shape[0] - cintura), (0, 0))) & (a[:, :, 3] > 0), 0, -1))
        faixa = a[cintura:cintura + 1].copy()
        m = faixa[0, :, 3] > 0
        out[cintura - 1, m] = faixa[0, m]
    else:
        por_cima(out, pernas)
        por_cima(out, corpo)
    return out


def animar(nome, cintura, previa=False):
    pasta = PERSONAGENS / nome
    linhas = []
    for direcao, (tipo, lado) in DIRECOES.items():
        a = np.asarray(Image.open(pasta / "rotations" / f"{direcao}.png").convert("RGBA")).copy()
        quadros = [quadro(a, cintura, tipo, lado, 0, False), quadro(a, cintura, tipo, lado, None, True),
                   quadro(a, cintura, tipo, lado, 1, False), quadro(a, cintura, tipo, lado, None, True)]
        for i, q in enumerate(quadros):
            Image.fromarray(q).save(pasta / "walk" / f"{direcao}-{i}.png")
        linhas.append(quadros)
    if previa:
        s = 4
        h, w = linhas[0][0].shape[:2]
        out = Image.new("RGBA", (4 * (w * s + 6), len(linhas) * (h * s + 6)), (70, 66, 80, 255))
        for r, qs in enumerate(linhas):
            for c, q in enumerate(qs):
                out.alpha_composite(Image.fromarray(q).resize((w * s, h * s), Image.NEAREST), (c * (w * s + 6), r * (h * s + 6)))
        out.save(RAIZ / f"docs/assets/caminhada-{nome.lower()}.png")
    print(f"{nome}: {len(linhas)} direções x 4 quadros em {(pasta / 'walk').relative_to(RAIZ)}")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("personagem")
    p.add_argument("--cintura", type=int, default=25, help="primeira linha das pernas (abaixo do cinto)")
    p.add_argument("--previa", action="store_true")
    args = p.parse_args()
    animar(args.personagem, args.cintura, args.previa)
