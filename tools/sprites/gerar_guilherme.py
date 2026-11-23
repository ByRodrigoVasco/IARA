"""
Sprites do Prof. Guilherme (Lógica) no Modo Sobrevivência.

    python tools/sprites/gerar_guilherme.py icone
        ic_negacao (24x24, 4 quadros): ícone da Negação, o símbolo ¬ pulsando numa placa.
        fx_negacao (40x28, 2 quadros): o ¬ gigante que cai (0) e o carimbo no impacto (1).

    python tools/sprites/gerar_guilherme.py personagem imagem_do_gemini.png
        Converte a imagem do Gemini (5 vistas em fundo magenta) nos sprites parados do personagem,
        lendo cada "pixel grande" pelo centro da grade do Gemini (rosto nítido).
        (personagem-escala imagem.png [cortes] usa a redução por escala do gerar_baudel.py.) Depois: python tools/sprites/animar_caminhada.py Guilherme

Precisa de Python 3 com Pillow e NumPy.
"""

import importlib.util
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

RAIZ = Path(__file__).resolve().parents[2]
SOBREVIVENCIA = RAIZ / "code/assets/sprites/sobrevivencia"
PASTA = RAIZ / "code/assets/sprites/PERSONAGENS/Guilherme"
CONTORNO = (14, 8, 22, 255)
LILAS = [(90, 50, 160, 255), (150, 110, 230, 255), (210, 190, 255, 255), (250, 245, 255, 255)]


def contornar(im):
    a = np.asarray(im).copy()
    cheio = a[:, :, 3] > 0
    viz = np.zeros_like(cheio)
    viz[1:, :] |= cheio[:-1, :]
    viz[:-1, :] |= cheio[1:, :]
    viz[:, 1:] |= cheio[:, :-1]
    viz[:, :-1] |= cheio[:, 1:]
    a[viz & ~cheio] = CONTORNO
    return Image.fromarray(a)


def simbolo(d, x, y, larg, alt, esp, cor, luz):
    """O ¬: barra deitada e a perninha descendo na ponta direita."""
    d.rectangle([x, y, x + larg - 1, y + esp - 1], fill=cor)
    d.rectangle([x + larg - esp, y, x + larg - 1, y + alt - 1], fill=cor)
    d.line([(x, y), (x + larg - 2, y)], fill=luz)


def fx_negacao():
    quadros = []
    for f in range(2):
        im = Image.new("RGBA", (40, 28), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        if f == 0:
            simbolo(d, 4, 6, 32, 18, 6, LILAS[1], LILAS[2])
        else:
            simbolo(d, 4, 6, 32, 18, 6, LILAS[2], LILAS[3])          # No impacto, mais claro
            for x, y in ((2, 2), (37, 3), (1, 25), (38, 24), (19, 1)):
                d.point((x, y), fill=LILAS[3])                     # Faíscas
        quadros.append(contornar(im))
    out = Image.new("RGBA", (80, 28), (0, 0, 0, 0))
    for i, q in enumerate(quadros):
        out.alpha_composite(q, (i * 40, 0))
    out.save(SOBREVIVENCIA / "fx_negacao.png")


def ic_negacao():
    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        d.rectangle([1, 1, 22, 22], fill=CONTORNO)
        d.rectangle([2, 2, 21, 21], fill=(40, 22, 70, 255))
        d.line([(2, 2), (21, 2)], fill=LILAS[0])
        cor = LILAS[2] if f in (1, 2) else LILAS[1]
        simbolo(d, 5, 8 - (1 if f == 2 else 0), 14, 9, 3, cor, LILAS[3])
        if f == 2:
            d.point([(4, 4), (19, 5)], fill=LILAS[3])
        quadros.append(im)
    out = Image.new("RGBA", (96, 24), (0, 0, 0, 0))
    for i, q in enumerate(quadros):
        out.alpha_composite(q, (i * 24, 0))
    out.save(SOBREVIVENCIA / "ic_negacao.png")


def periodo(sinal, lo=4.0, hi=9.0):
    """Acha o tamanho do "pixel grande" do Gemini (e onde a grade começa) por autocorrelação."""
    g = sinal - sinal.mean()
    idx = np.arange(len(g))
    melhor = None
    for p in np.arange(lo, hi, 0.05):
        z = (g * np.exp(2j * np.pi * (idx % p) / p)).sum()
        if melhor is None or abs(z) > melhor[0]:
            melhor = (abs(z), p, (-np.angle(z) / (2 * np.pi) * p) % p)
    return melhor[1], melhor[2]


def enxugar(arr, alt_max=30):
    """Tira as linhas mais repetidas do tronco/pernas até caber em alt_max (o rosto fica intacto)."""
    while arr.shape[0] > alt_max:
        topo = int(arr.shape[0] * 0.45)
        dif = [np.abs(arr[y].astype(int) - arr[y - 1].astype(int)).sum() for y in range(topo, arr.shape[0] - 2)]
        y = topo + int(np.argmin(dif))
        arr = np.delete(arr, y, axis=0)
    return arr


def personagem_grade(caminho, pasta=PASTA):
    """Como gerar_baudel.personagem, mas lê cada "pixel grande" do Gemini pelo centro da grade
    dele, em vez de reduzir por escala fixa: olhos e sorriso saem nítidos."""
    a = np.asarray(Image.open(caminho).convert("RGB")).astype(int)
    fundo = (a[:, :, 0] > 120) & (a[:, :, 2] > 100) & (a[:, :, 1] < np.minimum(a[:, :, 0], a[:, :, 2]) - 50)
    fg = ~fundo
    cols = fg.sum(0)
    w = len(cols)
    cortes = []
    for k in range(1, 5):
        c = k * w / 5
        lo, hi = int(c - w / 12), int(c + w / 12)
        cortes.append(lo + int(np.argmin(cols[lo:hi])))
    lim = [0] + cortes + [w]
    nomes = ["south", "south-east", "east", "north-east", "north"]
    (pasta / "rotations").mkdir(parents=True, exist_ok=True)
    (pasta / "walk").mkdir(parents=True, exist_ok=True)
    vistas = {}
    for nome, x0, x1 in zip(nomes, lim[:-1], lim[1:]):
        ys, xs = np.nonzero(fg[:, x0:x1])
        bx0, by0, bx1, by1 = x0 + xs.min(), ys.min(), x0 + xs.max() + 1, ys.max() + 1
        sub = a[by0:by1, bx0:bx1].astype(float)
        px, fx = periodo(np.abs(np.diff(sub, axis=1)).sum(2).sum(0))
        py, fy = periodo(np.abs(np.diff(sub, axis=0)).sum(2).sum(1))
        nx, ny = int((bx1 - bx0 - fx) / px + 0.5), int((by1 - by0 - fy) / py + 0.5)
        arr = np.zeros((ny + 2, nx + 2, 4), "uint8")
        for j in range(-1, ny + 1):
            for i in range(-1, nx + 1):
                cx, cy = bx0 + fx + (i + 0.5) * px, by0 + fy + (j + 0.5) * py
                xa, xb = int(cx - px * 0.3), int(cx + px * 0.3) + 1
                ya, yb = int(cy - py * 0.3), int(cy + py * 0.3) + 1
                if xa < 0 or ya < 0 or xb > a.shape[1] or yb > a.shape[0]:
                    continue
                m = fg[ya:yb, xa:xb]
                if m.mean() < 0.5:
                    continue
                arr[j + 1, i + 1] = (*np.median(a[ya:yb, xa:xb][m], axis=0).astype(int), 255)
        r, g, bl = (arr[:, :, k].astype(int) for k in range(3))
        rosa = (arr[:, :, 3] > 0) & (r > g + 25) & (bl > g + 25)
        arr[rosa] = (0, 0, 2, 255)                     # Sobras rosadas do fundo viram contorno
        im = Image.fromarray(arr)
        arr = enxugar(np.asarray(im.crop(im.getbbox())).copy())
        alt, larg = arr.shape[:2]
        tela = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        tela.alpha_composite(Image.fromarray(arr), ((32 - larg) // 2, 31 - alt))   # Pés na linha 30
        vistas[nome] = tela
    vistas["south-west"] = vistas["south-east"].transpose(Image.FLIP_LEFT_RIGHT)
    vistas["north-west"] = vistas["north-east"].transpose(Image.FLIP_LEFT_RIGHT)
    vistas["west"] = vistas["east"].transpose(Image.FLIP_LEFT_RIGHT)
    for nome, im in vistas.items():
        im.save(pasta / "rotations" / f"{nome}.png")
    print("rotations de", pasta.name, "geradas")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "personagem":
        personagem_grade(sys.argv[2])
    elif len(sys.argv) > 1 and sys.argv[1] == "personagem-escala":
        spec = importlib.util.spec_from_file_location("gerar_baudel", Path(__file__).with_name("gerar_baudel.py"))
        baudel = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(baudel)
        cortes = [int(c) for c in sys.argv[3].split(",")] if len(sys.argv) > 3 else None
        baudel.personagem(sys.argv[2], cortes, pasta=PASTA)
    else:
        fx_negacao()
        ic_negacao()
        print("fx_negacao.png e ic_negacao.png gerados")
