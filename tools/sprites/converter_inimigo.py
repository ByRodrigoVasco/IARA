"""
Converte uma imagem de referência (frente, lado e costas lado a lado, em fundo magenta #FF00FF)
num inimigo do Modo Sobrevivência: uma faixa de 12 quadros (frente 0-3, lado 4-7, costas 8-11),
com contorno escuro, paleta reduzida e uma passada simples (corpo sobe 1 px, pernas alternam).

Uso:
    python tools/sprites/converter_inimigo.py entrada.png saida.png [tamanho=32] [cores=16] [retoques.json]

retoques.json (opcional) pinta pixels depois da redução, por vista ("0" frente, "1" lado, "2" costas),
e pode fixar as colunas onde a imagem é cortada entre as vistas:
    {"cortes": [x1, x2], "0": [[x, y, [r, g, b]], ...]}
Serve para devolver o vermelho vivo de olhos e detalhes que se perdem na redução
(o vermelho vivo é o que brilha no escuro no jogo).

Precisa de Python 3 com Pillow e NumPy.
"""
import sys
import numpy as np
from PIL import Image

src, out = sys.argv[1], sys.argv[2]
size = int(sys.argv[3]) if len(sys.argv) > 3 else 32
colors = int(sys.argv[4]) if len(sys.argv) > 4 else 16
OUTLINE = (22, 16, 20, 255)

im = Image.open(src).convert('RGB'); a = np.asarray(im).astype(int)
fg = ~((a[:, :, 0] > 160) & (a[:, :, 1] < 120) & (a[:, :, 2] > 160))
# Separa as 3 vistas: corta nas colunas mais vazias perto de 1/3 e 2/3 da largura
colsum = fg.sum(0); W = len(colsum)
def cut(c):
    lo, hi = int(c - W / 8), int(c + W / 8)
    return lo + int(np.argmin(colsum[lo:hi]))
c1, c2 = cut(W / 3), cut(2 * W / 3)
# O retoques.json pode fixar onde cortar ("cortes": [x1, x2]) quando um enfeite fica entre duas vistas
RETOQUES = {}
if len(sys.argv) > 5:
    import json
    RETOQUES = json.load(open(sys.argv[5]))
    cortes = RETOQUES.pop("cortes", [None, None])
    c1 = cortes[0] or c1
    c2 = cortes[1] or c2

def biggest(mask):
    """Só os pedaços conectados grandes (o corpo e enfeites soltos, como um ícone acima da cabeça);
    tira restos da vista vizinha e sujeira do fundo."""
    h, w = mask.shape; lab = np.zeros((h, w), int); best, best_n, cur = 0, 0, 0
    sizes = {}
    for y in range(h):
        for x in range(w):
            if mask[y, x] and not lab[y, x]:
                cur += 1; stack = [(y, x)]; lab[y, x] = cur; n = 0
                while stack:
                    cy, cx = stack.pop(); n += 1
                    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        ny, nx = cy + dy, cx + dx
                        if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not lab[ny, nx]:
                            lab[ny, nx] = cur; stack.append((ny, nx))
                sizes[cur] = n
                if n > best_n: best, best_n = cur, n
    keep = [k for k, n in sizes.items() if n >= best_n * 0.015]
    return np.isin(lab, keep)

boxes = []
for x0, x1 in ((0, c1), (c1, c2), (c2, W)):
    m = np.zeros_like(fg); m[:, x0:x1] = biggest(fg[:, x0:x1]); fg[:, x0:x1] = m[:, x0:x1]
    ys, xs = np.where(m)
    boxes.append((int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1))
# Mesma escala para as três vistas, cabendo em size-2 (sobra 1 px de contorno de cada lado)
scale = min(min((size - 2) / (b[2] - b[0]), (size - 3) / (b[3] - b[1])) for b in boxes)  # 1 px livre em cima para o corpo subir

def shrink(b):
    x0, y0, x1, y1 = b
    w, h = max(1, round((x1 - x0) * scale)), max(1, round((y1 - y0) * scale))
    # O fundo vira contorno escuro antes de reduzir, para a borda não puxar o magenta
    arr = np.asarray(im.crop(b)).copy(); arr[~fg[y0:y1, x0:x1]] = OUTLINE[:3]
    rgb = Image.fromarray(arr).resize((w, h), Image.BOX)
    m = Image.fromarray((fg[y0:y1, x0:x1] * 255).astype('uint8')).resize((w, h), Image.BOX)
    img = rgb.convert('RGBA'); img.putalpha(m.point(lambda v: 255 if v > 110 else 0))
    return img

views = [shrink(b) for b in boxes]
# Paleta comum às três vistas
allpix = np.concatenate([np.asarray(v)[np.asarray(v)[:, :, 3] > 0][:, :3] for v in views]).astype('uint8')
pal_img = Image.fromarray(allpix.reshape(1, -1, 3)).quantize(colors, method=Image.Quantize.MEDIANCUT)

def finish(v):
    q = v.convert('RGB').quantize(palette=pal_img, dither=Image.Dither.NONE).convert('RGBA')
    q.putalpha(v.getchannel('A'))
    c = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    c.alpha_composite(q, ((size - q.width) // 2, size - 1 - q.height))
    px = c.load(); A = [[px[x, y][3] > 0 for x in range(size)] for y in range(size)]
    for y in range(size):
        for x in range(size):
            if not A[y][x] and any(0 <= x + dx < size and 0 <= y + dy < size and A[y + dy][x + dx] for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                px[x, y] = OUTLINE
    return c

base = [finish(v) for v in views]

# Retoques à mão (opcional): {"0": [[x, y, [r, g, b]], ...], "1": [...], "2": [...]} por vista
if RETOQUES:
    for k, pts in RETOQUES.items():
        px = base[int(k)].load()
        for x, y, c in pts: px[x, y] = tuple(c) + (255,)

def walk(img, f):
    """Passada: corpo sobe 1 px nos quadros 1 e 3; uma perna de cada vez fica 1 px mais curta."""
    arr = np.asarray(img).copy(); h = arr.shape[0]
    alpha = arr[:, :, 3] > 0; ys = np.where(alpha.any(1))[0]; top, bot = ys[0], ys[-1]
    legs_from = bot - max(3, (bot - top) // 5)
    out = np.zeros_like(arr)
    bob = 1 if f in (1, 3) else 0
    out[top - bob:legs_from - bob] = arr[top:legs_from]
    xs = np.where(alpha[legs_from:].any(0))[0]; mid = (xs[0] + xs[-1]) // 2 + 1 if len(xs) else size // 2
    for half, lifted in ((slice(0, mid), f == 1), (slice(mid, size), f == 3)):
        seg = arr[legs_from:bot + 1, half]
        if lifted:
            out[legs_from - bob:bot - bob, half] = np.where(seg[1:, :, 3:4] > 0, seg[1:], out[legs_from - bob:bot - bob, half])
        else:
            dst = out[legs_from:bot + 1, half]
            out[legs_from:bot + 1, half] = np.where(seg[:, :, 3:4] > 0, seg, dst)
    # Fecha o buraco entre corpo e pernas quando o corpo sobe
    if bob:
        gap = legs_from - 1
        row = arr[legs_from - 1]
        out[gap] = np.where(out[gap][:, 3:4] > 0, out[gap], row)
    return Image.fromarray(out)

strip = Image.new('RGBA', (size * 12, size), (0, 0, 0, 0))
for vi, v in enumerate(base):
    for f in range(4):
        strip.alpha_composite(walk(v, f), ((vi * 4 + f) * size, 0))
strip.save(out)
print('ok', [b for b in boxes], round(scale, 4))
