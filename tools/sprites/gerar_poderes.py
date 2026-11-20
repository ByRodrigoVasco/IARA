"""
Desenha em pixel art os poderes dos chefes Renê e IARA do Modo Sobrevivência, nas cores dos
sprites novos (bo_rene e bo_iara). Cada poder é uma faixa de 4 quadros, no tamanho que o jogo
espera (sv_sprite_table.h).

    Renê: bp_martelo (Decisão Final), bp_sombra (Premissa H), bp_corrente (D -> C), bp_contrato (D)
    IARA: bp_onda (onda com rostos), bp_maos (Mãos d'Água), bp_nota (anéis do Canto)

Uso, a partir da raiz do repositório:
    python tools/sprites/gerar_poderes.py            grava em code/assets/sprites/sobrevivencia
    python tools/sprites/gerar_poderes.py --previa   grava também docs/assets/poderes-rene-iara.png

Precisa de Python 3 com Pillow e NumPy.
"""

import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

RAIZ = Path(__file__).resolve().parents[2]
SAIDA = RAIZ / "code/assets/sprites/sobrevivencia"

CONTORNO = (22, 16, 20, 255)
# Vermelho que brilha no escuro (sv_assets.c procura r >= 170, g <= 95, b <= 95)
BRILHO = (232, 36, 44, 255)

MADEIRA = [(70, 36, 22, 255), (118, 66, 34, 255), (164, 100, 52, 255), (204, 140, 80, 255)]
OURO = [(150, 112, 40, 255), (214, 172, 74, 255)]
FERRO = [(40, 42, 50, 255), (80, 84, 96, 255), (128, 134, 148, 255), (180, 186, 196, 255)]
FERRUGEM = (118, 62, 38, 255)
PAPEL = [(178, 168, 140, 255), (214, 206, 182, 255), (238, 232, 212, 255)]
TINTA = (112, 112, 124, 255)
ROXO = [(34, 18, 48), (66, 38, 96), (104, 66, 150), (150, 110, 200)]
AGUA = [(8, 40, 50, 255), (14, 70, 82, 255), (22, 108, 120, 255), (40, 156, 164, 255), (96, 210, 206, 255), (196, 244, 238, 255)]
PORCELANA = [(150, 168, 172, 255), (212, 222, 222, 255), (240, 244, 242, 255)]
PELE = [(110, 136, 140, 255), (160, 186, 188, 255), (204, 222, 222, 255)]
CIANO = [(30, 140, 170), (100, 224, 236), (210, 250, 252)]


def contornar(im):
    """Contorno escuro de 1 px em volta de tudo que não é transparente."""
    a = np.asarray(im).copy()
    cheio = a[:, :, 3] > 0
    viz = np.zeros_like(cheio)
    viz[1:, :] |= cheio[:-1, :]
    viz[:-1, :] |= cheio[1:, :]
    viz[:, 1:] |= cheio[:, :-1]
    viz[:, :-1] |= cheio[:, 1:]
    a[viz & ~cheio] = CONTORNO
    return Image.fromarray(a)


def faixa(quadros):
    w, h = quadros[0].size
    out = Image.new("RGBA", (w * len(quadros), h), (0, 0, 0, 0))
    for i, q in enumerate(quadros):
        out.alpha_composite(q, (i * w, 0))
    return out


def girar(pontos, cx, cy, graus):
    a = math.radians(graus)
    c, s = math.cos(a), math.sin(a)
    return [(cx + (x - cx) * c - (y - cy) * s, cy + (x - cx) * s + (y - cy) * c) for x, y in pontos]


# -----------------------------------------------------------------------------
# Renê
# -----------------------------------------------------------------------------

def martelo_de_juiz(d, cx, cy, graus):
    """Martelo de juiz: cabo de madeira saindo do cabeçote, que tem duas faixas douradas.
    (cx, cy) é o centro do cabeçote; o cabo aponta para cima e para a direita antes de girar."""
    cab = [(cx - 12, cy - 6), (cx + 12, cy - 6), (cx + 12, cy + 6), (cx - 12, cy + 6)]
    cabo = [(cx - 2, cy - 6), (cx + 2, cy - 6), (cx + 2, cy - 30), (cx - 2, cy - 30)]
    d.polygon(girar(cabo, cx, cy, graus), fill=MADEIRA[1])
    d.polygon(girar([(cx - 1, cy - 8), (cx, cy - 8), (cx, cy - 29), (cx - 1, cy - 29)], cx, cy, graus), fill=MADEIRA[3])
    d.polygon(girar(cab, cx, cy, graus), fill=MADEIRA[2])
    d.polygon(girar([(cx - 12, cy + 2), (cx + 12, cy + 2), (cx + 12, cy + 6), (cx - 12, cy + 6)], cx, cy, graus), fill=MADEIRA[0])
    d.polygon(girar([(cx - 11, cy - 5), (cx + 11, cy - 5), (cx + 11, cy - 4), (cx - 11, cy - 4)], cx, cy, graus), fill=MADEIRA[3])
    for bx in (-8, 6):
        d.polygon(girar([(cx + bx, cy - 6), (cx + bx + 2, cy - 6), (cx + bx + 2, cy + 6), (cx + bx, cy + 6)], cx, cy, graus), fill=OURO[1])


def rachaduras(d, cx, cy, tamanho, cor):
    rng = np.random.default_rng(3)
    for k in range(6):
        a = k * math.pi / 3 + rng.uniform(-0.3, 0.3)
        x, y = cx, cy
        for _ in range(tamanho):
            nx = x + math.cos(a) * 2.2
            ny = y + math.sin(a) * 1.3
            d.line([(x, y), (nx, ny)], fill=cor)
            x, y = nx, ny
            a += rng.uniform(-0.5, 0.5)


def bp_martelo():
    """64x64. O martelo desce (0, 1), bate no chão com a onda de choque (2) e deixa o chão rachado (3)."""
    quadros = []
    for f, (graus, hx, hy) in enumerate([(-55, 30, 18), (-25, 32, 30), (0, 32, 40), (8, 34, 38)]):
        im = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        if f >= 2:
            # Onda de choque e rachaduras no chão, em perspectiva (achatadas)
            r = 22 if f == 2 else 28
            d.ellipse([32 - r, 47 - r * 0.45, 32 + r, 47 + r * 0.45], outline=(196, 188, 170, 255 if f == 2 else 150))
            rachaduras(d, 32, 47, 6 if f == 2 else 9, (36, 26, 26, 255))
            if f == 2:
                for x, y in ((18, 40), (46, 41), (24, 36), (41, 35)):
                    d.rectangle([x, y, x + 1, y + 1], fill=(220, 210, 190, 255))   # Lascas voando
        martelo_de_juiz(d, hx, hy, graus)
        quadros.append(contornar(im))
    return faixa(quadros)


def bp_sombra():
    """32x32. A sombra da Premissa H: o Renê na cadeira em roxo, aparecendo aos poucos (0-2) até ficar inteira (3)."""
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        d.rounded_rectangle([6, 3, 25, 29], radius=5, fill=ROXO[0] + (255,))           # Encosto da cadeira
        d.rounded_rectangle([4, 17, 27, 29], radius=2, fill=ROXO[1] + (255,))          # Braços da cadeira
        d.polygon([(9, 15), (22, 15), (24, 28), (7, 28)], fill=ROXO[1] + (255,))       # Paletó
        d.ellipse([11, 6, 20, 15], fill=ROXO[2] + (255,))                              # Cabeça
        d.rectangle([12, 6, 19, 7], fill=ROXO[3] + (255,))                             # Cabelo penteado
        d.rectangle([13, 10, 13, 10], fill=BRILHO)
        d.rectangle([17, 10, 17, 10], fill=BRILHO)
        d.line([(13, 13), (18, 13)], fill=ROXO[3] + (255,))                            # Sorriso
        for x, y in ((13, 19), (13, 20), (13, 21), (13, 22), (13, 23), (17, 19), (17, 20), (17, 21), (17, 22), (17, 23), (14, 21), (15, 21), (16, 21)):
            im.putpixel((x, y), ROXO[3] + (255,))                                      # "H" no peito
        a = np.asarray(contornar(im)).copy()
        if f < 3:
            # Pontilhado: mostra 1/4, 2/4 e 3/4 dos pixels, como quem está se materializando
            yy, xx = np.mgrid[:32, :32]
            mostra = ((xx * 3 + yy * 5) % 4) < (f + 1)
            a[~mostra, 3] = 0
            a[(a[:, :, 3] > 0), 3] = 120 + 40 * f
        quadros.append(Image.fromarray(a))
    return faixa(quadros)


def bp_corrente():
    """72x16, esticado ao longo do feixe. Elos de ferro andando para a frente, com o lacre vermelho na ponta."""
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (72, 16), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        passo = 12
        for k in range(-1, 7):
            x = k * passo + f * 3
            if x > 56:
                continue
            # Elo deitado (anel) e elo em pé (barra), alternando
            d.ellipse([x, 4, x + 9, 11], outline=FERRO[2], width=2)
            d.line([(x + 2, 5), (x + 6, 5)], fill=FERRO[3])
            d.rectangle([x + 8, 6, x + 14, 9], fill=FERRO[1])
            d.line([(x + 9, 6), (x + 13, 6)], fill=FERRO[3])
            if k % 3 == 1:
                d.rectangle([x + 3, 10, x + 4, 10], fill=FERRUGEM)
        # Ponta: lacre de cera vermelha, que brilha no escuro
        d.ellipse([60, 3, 69, 12], fill=(150, 20, 30, 255))
        d.ellipse([62, 5, 67, 10], fill=BRILHO)
        d.point((63, 6), fill=(255, 160, 150, 255))
        quadros.append(contornar(im))
    return faixa(quadros)


def bp_contrato():
    """32x26. O contrato que persegue o jogador: folha balançando, linhas de texto, assinatura e lacre vermelho."""
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (32, 26), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        dobra = (0, 3, 0, -3)[f]
        folha = [(6, 3 + max(dobra, 0)), (25, 3 + max(-dobra, 0)), (25, 23), (6, 23)]
        d.polygon(folha, fill=PAPEL[2])
        d.polygon([(6, 18), (25, 18), (25, 23), (6, 23)], fill=PAPEL[1])
        if dobra > 0:
            d.polygon([(6, 3), (9, 3), (6, 6)], fill=PAPEL[0])           # Canto dobrado
        if dobra < 0:
            d.polygon([(25, 3), (22, 3), (25, 6)], fill=PAPEL[0])
        for k, largura in enumerate((15, 12, 15, 9)):
            d.line([(9, 8 + k * 3), (9 + largura - 4, 8 + k * 3)], fill=TINTA)
        d.line([(9, 20), (12, 18), (14, 20), (17, 18)], fill=(40, 40, 80, 255))   # Assinatura
        d.ellipse([18, 16, 25, 23], fill=(150, 20, 30, 255))                       # Lacre
        d.ellipse([19, 17, 24, 22], fill=BRILHO)
        d.text((20, 16), "", fill=None)
        # Pedaço de corrente preso no canto
        d.ellipse([1, 0, 6, 4], outline=FERRO[2])
        d.ellipse([4, 2, 9, 6], outline=FERRO[1])
        quadros.append(contornar(im))
    return faixa(quadros)


# -----------------------------------------------------------------------------
# IARA
# -----------------------------------------------------------------------------

def rosto(d, x, y, boca):
    """Rostinho de porcelana gritando dentro da água (7 x 9)."""
    d.ellipse([x, y, x + 6, y + 8], fill=PORCELANA[1])
    d.ellipse([x + 1, y, x + 5, y + 4], fill=PORCELANA[2])
    d.point([(x + 2, y + 3), (x + 4, y + 3)], fill=CONTORNO)
    d.rectangle([x + 2, y + 5, x + 4, y + 5 + boca], fill=CONTORNO)


def bp_onda():
    """96x48 (o jogo desenha pela metade). A onda da IARA com a crista se enrolando e rostos gritando dentro."""
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (96, 48), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        topo = []
        for x in range(96):
            y = 18 + 6 * math.sin((x + f * 6) / 96 * 2 * math.pi * 1.5) + 3 * math.sin((x - f * 4) / 11.0)
            topo.append((x, y))
        d.polygon(topo + [(95, 47), (0, 47)], fill=AGUA[1])
        # Faixas mais claras perto da superfície, mais escuras no fundo
        for desloc, cor in ((3, AGUA[2]), (1, AGUA[3])):
            d.line([(x, y + desloc) for x, y in topo], fill=cor, width=2)
        d.polygon([(0, 38), (95, 38), (95, 47), (0, 47)], fill=AGUA[0])
        # Crista: espuma por cima e o rolo da onda caindo para a frente
        d.line(topo, fill=AGUA[5])
        ponta = max(range(96), key=lambda x: -topo[x][1])
        px, py = topo[ponta]
        for k, (dx, dy) in enumerate(((1, -2), (3, -3), (5, -2), (7, -1), (4, -5), (8, -4))):
            if (k + f) % 3:
                d.point((min(95, px + dx), py + dy), fill=AGUA[5 if k % 2 else 4])   # Respingos saindo da crista
        for k, (rx, ry) in enumerate(((14, 26), (44, 29), (74, 25))):
            rosto(d, rx, ry + (1 if (f + k) % 2 else 0), 1 + (f + k) % 2)
        # Bolhas e respingos
        for x, y in ((8 + f * 7, 34), (60 - f * 5, 36), (88, 30 - f)):
            d.point((x % 96, y), fill=AGUA[4])
        quadros.append(contornar(im))
    return faixa(quadros)


def mao(d, x, base, altura, aberta):
    """Mão pálida da IARA saindo da água: braço, palma e quatro dedos compridos com membrana."""
    topo = base - altura
    if topo + 8 < base:                                                  # Braço (só quando já saiu da água)
        d.rectangle([x - 2, topo + 8, x + 3, base], fill=PELE[1])
        d.line([(x - 2, topo + 9), (x - 2, base)], fill=PELE[0])
        d.line([(x + 1, topo + 9), (x + 1, base)], fill=PELE[2])
    d.rectangle([x - 4, topo + 3, x + 5, min(topo + 9, base)], fill=PELE[1])   # Palma
    if aberta:
        dedos = [(-4, -1), (-1, -3), (2, -3), (5, -1)]
        d.polygon([(x - 4, topo + 4), (x + 6, topo + 4), (x + 5, topo + 1), (x - 3, topo + 1)], fill=(120, 190, 190, 255))  # Membrana
    else:
        dedos = [(-3, 1), (-1, 0), (2, 0), (4, 1)]                    # Fechando para agarrar
    for dx, dy in dedos:
        d.line([(x + dx, topo + 3), (x + dx, topo + dy - 2)], fill=PELE[2])
    d.point([(x + dx, topo + dy - 2) for dx, dy in dedos], fill=PELE[0])
    # Pingos de água escorrendo
    d.point([(x + 4, topo + 12), (x - 3, topo + 15)], fill=AGUA[4])


def bp_maos():
    """48x44. Mãos d'Água: duas mãos sobem da poça (0-2) e se fecham para prender o jogador (3)."""
    quadros = []
    for f, altura in enumerate((6, 16, 27, 25)):
        im = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        d.ellipse([3, 33, 44, 43], fill=AGUA[1])
        d.ellipse([7, 34, 40, 41], fill=AGUA[2])
        d.arc([9, 35, 38, 41], 200, 340, fill=AGUA[4])
        for k, x in enumerate((16, 31)):
            mao(d, x, 38, altura - 2 * k, aberta=f < 3)
        d.arc([10 - f, 36, 22 + f, 41], 180, 360, fill=AGUA[5])      # Anel de água em volta dos braços
        d.arc([25 - f, 36, 37 + f, 41], 180, 360, fill=AGUA[5])
        quadros.append(contornar(im))
    return faixa(quadros)


def bp_nota():
    """24x24. Projétil do Canto: o olho de sonar da IARA (ponto vermelho) com anéis ciano se abrindo."""
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        for k, r in enumerate((3 + f, 7 + f, 11)):
            if r > 11:
                continue
            cor = CIANO[1] if k < 2 else CIANO[0]
            alfa = 255 if k < 2 else 140 + 25 * f
            d.ellipse([12 - r, 12 - r, 11 + r, 11 + r], outline=cor + (alfa,))
        d.ellipse([9, 9, 14, 14], fill=(22, 16, 20, 255))
        d.ellipse([10, 10, 13, 13], fill=BRILHO)
        d.point((10, 10), fill=CIANO[2] + (255,))
        quadros.append(Image.fromarray(np.asarray(im)))
    return faixa(quadros)


PODERES = {"bp_martelo": bp_martelo, "bp_sombra": bp_sombra, "bp_corrente": bp_corrente, "bp_contrato": bp_contrato,
           "bp_onda": bp_onda, "bp_maos": bp_maos, "bp_nota": bp_nota}


def gerar(previa=False):
    faixas = {}
    for nome, func in PODERES.items():
        img = func()
        img.save(SAIDA / f"{nome}.png")
        faixas[nome] = img
    if previa:
        escala = 4
        linhas = [f.resize((f.width * escala, f.height * escala), Image.NEAREST) for f in faixas.values()]
        out = Image.new("RGBA", (max(l.width for l in linhas) + 16, sum(l.height + 12 for l in linhas) + 4), (46, 42, 54, 255))
        y = 8
        for l in linhas:
            out.alpha_composite(l, (8, y))
            y += l.height + 12
        out.save(RAIZ / "docs/assets/poderes-rene-iara.png")
    print(f"{len(faixas)} poderes gerados em {SAIDA.relative_to(RAIZ)}")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--previa", action="store_true")
    gerar(p.parse_args().previa)
