"""
Converte duas imagens geradas por IA (pose normal e pose de ataque, cada uma com frente, lado e
costas lado a lado em fundo magenta #FF00FF) num chefe do Modo Sobrevivência: uma faixa de 24
quadros (0-11 andando, 12-23 atacando; em cada metade: frente 0-3, lado 4-7, costas 8-11).

As duas poses usam a mesma escala, para o chefe não mudar de tamanho ao atacar. Chefe não anda
de pernas: o movimento é o corpo subindo e descendo; no ataque ele treme de um lado para o outro.

Uso:
    python tools/sprites/gemini_para_chefe.py normal.png ataque.png saida.png [tamanho=64] [cores=24] [retoques.json]

retoques.json (opcional), como no gemini_para_inimigo.py, com uma chave por pose:
    {"normal": {"cortes": [x1, x2], "0": [[x, y, [r, g, b]], ...]}, "ataque": {...}}

Precisa de Python 3 com Pillow e NumPy.
"""

import json
import sys

import numpy as np
from PIL import Image

OUTLINE = (22, 16, 20, 255)


def separar(caminho, cortes):
    im = Image.open(caminho).convert('RGB')
    a = np.asarray(im).astype(int)
    # Fundo: magenta e as bordas rosadas que a IA deixa em volta do desenho
    fg = ~((a[:, :, 0] > 120) & (a[:, :, 2] > 100) & (a[:, :, 1] < np.minimum(a[:, :, 0], a[:, :, 2]) - 50))
    colsum = fg.sum(0)
    W = len(colsum)

    def corte(c):
        lo, hi = int(c - W / 8), int(c + W / 8)
        return lo + int(np.argmin(colsum[lo:hi]))

    c1 = (cortes or [None, None])[0] or corte(W / 3)
    c2 = (cortes or [None, None])[1] or corte(2 * W / 3)
    caixas = []
    for x0, x1 in ((0, c1), (c1, c2), (c2, W)):
        m = grandes(fg[:, x0:x1])
        fg[:, x0:x1] = m
        ys, xs = np.where(m)
        caixas.append((x0 + int(xs.min()), int(ys.min()), x0 + int(xs.max()) + 1, int(ys.max()) + 1))
    return im, fg, caixas


def grandes(mask):
    """Só os pedaços conectados grandes: tira sujeira do fundo e restos da vista vizinha."""
    h, w = mask.shape
    lab = np.zeros((h, w), int)
    tamanhos, cur = {}, 0
    for y in range(h):
        for x in range(w):
            if mask[y, x] and not lab[y, x]:
                cur += 1
                pilha = [(y, x)]
                lab[y, x] = cur
                n = 0
                while pilha:
                    cy, cx = pilha.pop()
                    n += 1
                    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        ny, nx = cy + dy, cx + dx
                        if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not lab[ny, nx]:
                            lab[ny, nx] = cur
                            pilha.append((ny, nx))
                tamanhos[cur] = n
    maior = max(tamanhos.values())
    return np.isin(lab, [k for k, n in tamanhos.items() if n >= maior * 0.015])


def reduzir(im, fg, caixa, escala):
    x0, y0, x1, y1 = caixa
    w, h = max(1, round((x1 - x0) * escala)), max(1, round((y1 - y0) * escala))
    arr = np.asarray(im.crop(caixa)).copy()
    arr[~fg[y0:y1, x0:x1]] = OUTLINE[:3]  # o fundo vira contorno, para a borda não puxar o magenta
    rgb = Image.fromarray(arr).resize((w, h), Image.BOX)
    m = Image.fromarray((fg[y0:y1, x0:x1] * 255).astype('uint8')).resize((w, h), Image.BOX)
    img = rgb.convert('RGBA')
    img.putalpha(m.point(lambda v: 255 if v > 110 else 0))
    return img


def acabar(v, paleta, tam):
    q = v.convert('RGB').quantize(palette=paleta, dither=Image.Dither.NONE).convert('RGBA')
    q.putalpha(v.getchannel('A'))
    c = Image.new('RGBA', (tam, tam), (0, 0, 0, 0))
    c.alpha_composite(q, ((tam - q.width) // 2, tam - 1 - q.height))
    px = c.load()
    A = [[px[x, y][3] > 0 for x in range(tam)] for y in range(tam)]
    for y in range(tam):
        for x in range(tam):
            if not A[y][x] and any(0 <= x + dx < tam and 0 <= y + dy < tam and A[y + dy][x + dx]
                                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                px[x, y] = OUTLINE
    return c


def deslocar(img, dx, dy):
    c = Image.new('RGBA', img.size, (0, 0, 0, 0))
    c.alpha_composite(img, (dx, dy))
    return c


def main():
    normal, ataque, saida = sys.argv[1:4]
    tam = int(sys.argv[4]) if len(sys.argv) > 4 else 64
    cores = int(sys.argv[5]) if len(sys.argv) > 5 else 24
    ret = json.load(open(sys.argv[6])) if len(sys.argv) > 6 else {}

    poses = []
    for nome, caminho in (('normal', normal), ('ataque', ataque)):
        r = dict(ret.get(nome, {}))
        poses.append((nome, *separar(caminho, r.pop('cortes', None)), r))

    # Uma escala só para as 6 vistas, deixando 1 px livre em cima (o corpo sobe) e 1 de cada lado (tremida)
    escala = min(min((tam - 4) / (c[2] - c[0]), (tam - 3) / (c[3] - c[1])) for _, _, _, cx, _ in poses for c in cx)
    reduzidas = [[reduzir(im, fg, c, escala) for c in cx] for _, im, fg, cx, _ in poses]

    pix = np.concatenate([np.asarray(v)[np.asarray(v)[:, :, 3] > 0][:, :3] for vs in reduzidas for v in vs]).astype('uint8')
    paleta = Image.fromarray(pix.reshape(1, -1, 3)).quantize(cores, method=Image.Quantize.MEDIANCUT)

    faixa = Image.new('RGBA', (tam * 24, tam), (0, 0, 0, 0))
    for p, ((nome, _, _, _, retoque), vistas) in enumerate(zip(poses, reduzidas)):
        for vi, v in enumerate(vistas):
            base = acabar(v, paleta, tam)
            px = base.load()
            for x, y, cor in retoque.get(str(vi), []):
                px[x, y] = tuple(cor) + (255,)
            for f in range(4):
                if nome == 'normal':
                    quadro = deslocar(base, 0, -1 if f in (1, 3) else 0)   # respira
                else:
                    quadro = deslocar(base, (0, 1, 0, -1)[f], 0)           # treme
                faixa.alpha_composite(quadro, ((p * 12 + vi * 4 + f) * tam, 0))
    faixa.save(saida)
    print('ok, escala', round(escala, 4))


if __name__ == '__main__':
    main()
