"""
Sprites do Prof. Baudel (IHC) no Modo Sobrevivência.

    python tools/sprites/gerar_baudel.py icone
        ic_calor (24x24, 4 quadros): ícone do Mapa de Calor, uma tela com o mapa pulsando e o cursor do mouse.

    python tools/sprites/gerar_baudel.py personagem imagem_de_referencia.png [cortes x1,x2,x3,x4]
        Converte a imagem de referência (5 vistas lado a lado em fundo magenta: frente, frente-direita,
        lado direito, costas-direita, costas) nos sprites parados de 32x32 do personagem
        (code/assets/sprites/PERSONAGENS/Baudel/rotations). As vistas da esquerda são o espelho das da direita.
        Depois, a caminhada sai de: python tools/sprites/animar_caminhada.py Baudel

Precisa de Python 3 com Pillow e NumPy.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

RAIZ = Path(__file__).resolve().parents[2]
SOBREVIVENCIA = RAIZ / "code/assets/sprites/sobrevivencia"
PASTA = RAIZ / "code/assets/sprites/PERSONAGENS/Baudel"
CONTORNO = (8, 10, 16, 255)


def ic_calor():
    rampa = [(40, 70, 220), (40, 200, 230), (70, 210, 90), (245, 220, 60), (235, 45, 40)]

    def cor(h):
        x = max(0.0, min(0.999, h)) * 4
        k = int(x)
        t = x - k
        a, b = rampa[k], rampa[k + 1]
        return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3)) + (255,)

    quadros = []
    for f in range(4):
        im = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        d.rectangle([1, 2, 22, 19], fill=CONTORNO)
        d.rectangle([2, 3, 21, 18], fill=(30, 36, 52, 255))                # Tela
        pulso = (0.0, 0.08, 0.14, 0.08)[f]
        for y in range(4, 18):
            for x in range(3, 21):
                h = max(1 - ((x - 9) ** 2 + (y - 9) ** 2) ** 0.5 / 7.5, 1 - ((x - 16) ** 2 + (y - 13) ** 2) ** 0.5 / 4.5 - 0.25)
                if h > 0.05:
                    im.putpixel((x, y), cor(h + pulso))
        d.rectangle([9, 20, 14, 21], fill=CONTORNO)                       # Pé do monitor
        # Cursor do mouse (seta branca com contorno)
        seta = ["#......", "##.....", "#o#....", "#oo#...", "#ooo#..", "#oooo#.", "#oo###.", "##o#...", "#..#..."]
        cx, cy = 13 + (f % 2), 9
        for dy, linha in enumerate(seta):
            for dx, v in enumerate(linha):
                if v == "#":
                    im.putpixel((cx + dx, cy + dy), CONTORNO)
                elif v == "o":
                    im.putpixel((cx + dx, cy + dy), (250, 250, 250, 255))
        quadros.append(im)
    out = Image.new("RGBA", (96, 24), (0, 0, 0, 0))
    for i, q in enumerate(quadros):
        out.alpha_composite(q, (i * 24, 0))
    out.save(SOBREVIVENCIA / "ic_calor.png")
    print("ic_calor.png gerado")


def personagem(caminho, cortes=None, pasta=PASTA):
    im = Image.open(caminho).convert("RGB")
    a = np.asarray(im).astype(int)
    fundo = (a[:, :, 0] > 120) & (a[:, :, 2] > 100) & (a[:, :, 1] < np.minimum(a[:, :, 0], a[:, :, 2]) - 50)
    fg = ~fundo
    cols = fg.sum(0)
    w = len(cols)
    if not cortes:
        cortes = []
        for k in range(1, 5):
            c = k * w / 5
            lo, hi = int(c - w / 12), int(c + w / 12)
            cortes.append(lo + int(np.argmin(cols[lo:hi])))
    limites = [0] + list(cortes) + [w]
    caixas = []
    for x0, x1 in zip(limites[:-1], limites[1:]):
        ys, xs = np.nonzero(fg[:, x0:x1])
        caixas.append((x0 + xs.min(), ys.min(), x0 + xs.max() + 1, ys.max() + 1))
    # Uma escala só: a vista mais alta ocupa 30 px (linhas 1 a 30, como os outros personagens).
    # O desenho de referência já tem contorno preto; cada pixel final recebe a cor mais comum do seu bloco
    # (e não a média), para as bordas, os óculos e a gravata continuarem nítidos.
    altura = max(c[3] - c[1] for c in caixas)
    escala = 30.0 / altura
    # Paleta com vagas reservadas para as cores vivas (cordão, calça, pele), que são poucas mas
    # identificam o personagem; o resto vem das cores mais comuns (o preto da roupa e do contorno)
    pix = a[fg].reshape(-1, 3).astype("uint8")
    f = pix.astype(float) / 255.0
    vivas = (f.max(1) - f.min(1) > 0.25) & (f.max(1) > 0.3)
    partes = []
    if vivas.sum() > 12:
        pv = Image.fromarray(pix[vivas].reshape(1, -1, 3)).quantize(12, method=Image.Quantize.MEDIANCUT)
        partes += [tuple(pv.getpalette()[3 * i:3 * i + 3]) for i in range(12)]
    pr = Image.fromarray(pix[~vivas].reshape(1, -1, 3)).quantize(32 - len(partes), method=Image.Quantize.MEDIANCUT)
    partes += [tuple(pr.getpalette()[3 * i:3 * i + 3]) for i in range(32 - len(partes))]
    paleta = Image.new("P", (1, 1))
    paleta.putpalette([c for cor in partes for c in cor] + [0, 0, 0] * (256 - len(partes)))
    idx_img = np.asarray(Image.fromarray(a.astype("uint8")).quantize(palette=paleta, dither=Image.Dither.NONE))
    cores = np.array(paleta.getpalette()[:768]).reshape(-1, 3)
    nomes = ["south", "south-east", "east", "north-east", "north"]
    (pasta / "rotations").mkdir(parents=True, exist_ok=True)
    (pasta / "walk").mkdir(parents=True, exist_ok=True)
    vistas = {}
    for nome, (x0, y0, x1, y1) in zip(nomes, caixas):
        larg, alt = max(1, round((x1 - x0) * escala)), max(1, round((y1 - y0) * escala))
        arr = np.zeros((alt, larg, 4), "uint8")
        for ty in range(alt):
            sy0, sy1 = y0 + int(ty / escala), max(y0 + int(ty / escala) + 1, y0 + int((ty + 1) / escala))
            for tx in range(larg):
                sx0, sx1 = x0 + int(tx / escala), max(x0 + int(tx / escala) + 1, x0 + int((tx + 1) / escala))
                bloco = fg[sy0:sy1, sx0:sx1]
                if bloco.mean() < 0.5:
                    continue
                k = np.bincount(idx_img[sy0:sy1, sx0:sx1][bloco], minlength=256).argmax()
                arr[ty, tx] = (*cores[k], 255)
        # Sobras rosadas do fundo magenta na borda viram contorno
        r, g, bl = arr[:, :, 0].astype(int), arr[:, :, 1].astype(int), arr[:, :, 2].astype(int)
        rosa = (arr[:, :, 3] > 0) & (r > g + 25) & (bl > g + 25)
        arr[rosa] = (0, 0, 2, 255)
        tela = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        tela.alpha_composite(Image.fromarray(arr), ((32 - larg) // 2, 31 - alt))   # Pés na linha 30, como os outros
        vistas[nome] = tela
    vistas["south-west"] = vistas["south-east"].transpose(Image.FLIP_LEFT_RIGHT)
    vistas["north-west"] = vistas["north-east"].transpose(Image.FLIP_LEFT_RIGHT)
    vistas["west"] = vistas["east"].transpose(Image.FLIP_LEFT_RIGHT)
    for nome, v in vistas.items():
        v.save(pasta / "rotations" / f"{nome}.png")
    print(f"rotations de {pasta.name} gerados; cortes usados:", cortes)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "personagem":
        cortes = [int(c) for c in sys.argv[3].split(",")] if len(sys.argv) > 3 else None
        personagem(sys.argv[2], cortes)
    else:
        ic_calor()
