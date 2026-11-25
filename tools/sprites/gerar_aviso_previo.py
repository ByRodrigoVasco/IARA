"""Desenha o avião de papel e o ícone do poder Aviso Prévio.

Uso, na raiz do repositório (precisa do Pillow):
    python tools/sprites/gerar_aviso_previo.py

Saída em code/assets/sprites/sobrevivencia: fx_aviso.png (4 quadros 28x16, voando para a direita)
e ic_aviso.png (4 quadros 24x24).
"""

from PIL import Image, ImageDraw

SAIDA = "code/assets/sprites/sobrevivencia/"
CONTORNO = (7, 4, 10, 255)
CLARO = (240, 242, 236, 255)
MEIO = (198, 204, 220, 255)
SOMBRA = (122, 133, 171, 255)
VERMELHO = (214, 40, 48, 255)


def contornar(img):
    """Um pixel de contorno em volta de tudo o que foi desenhado."""
    origem = img.load()
    saida = img.copy()
    destino = saida.load()
    for y in range(img.height):
        for x in range(img.width):
            if origem[x, y][3]:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                a, b = x + dx, y + dy
                if 0 <= a < img.width and 0 <= b < img.height and origem[a, b][3]:
                    destino[x, y] = CONTORNO
                    break
    return saida


def aviao(asa):
    """Visto de cima e um pouco de lado; 'asa' abre e fecha as pontas durante o voo."""
    img = Image.new("RGBA", (28, 16))
    d = ImageDraw.Draw(img)
    bico, meio = (26, 8), 8
    d.polygon([bico, (3, 2 - asa), (10, meio)], fill=CLARO)             # asa de cima
    d.polygon([bico, (4, 13 + asa), (10, meio)], fill=MEIO)             # asa de baixo
    d.polygon([bico, (10, meio), (6, 11)], fill=SOMBRA)                 # dobra do meio
    d.line([(10, meio), (24, meio)], fill=SOMBRA)
    d.rectangle([12, 4, 14, 6], fill=VERMELHO)                          # o carimbo
    return contornar(img)


def icone(subida):
    img = Image.new("RGBA", (24, 24))
    d = ImageDraw.Draw(img)
    y = 1 - subida
    d.polygon([(21, 3 + y), (2, 9 + y), (10, 13 + y)], fill=CLARO)
    d.polygon([(21, 3 + y), (10, 13 + y), (15, 20 + y)], fill=MEIO)
    d.polygon([(21, 3 + y), (10, 13 + y), (10, 18 + y)], fill=SOMBRA)
    d.rectangle([7, 9 + y, 9, 11 + y], fill=VERMELHO)
    img = contornar(img)
    d = ImageDraw.Draw(img)
    # Riscos de velocidade atrás do avião
    for x, linha in ((1, 19), (3, 22)):
        d.line([(x + subida, linha), (x + 4 + subida, linha - 2)], fill=SOMBRA)
    return img


def tira(quadros, nome):
    largura, altura = quadros[0].size
    saida = Image.new("RGBA", (largura * len(quadros), altura))
    for i, quadro in enumerate(quadros):
        saida.paste(quadro, (i * largura, 0))
    saida.save(SAIDA + nome, optimize=True)


if __name__ == "__main__":
    tira([aviao(a) for a in (0, 1, 0, -1)], "fx_aviso.png")
    tira([icone(s) for s in (0, 1, 0, -1)], "ic_aviso.png")
    print("fx_aviso.png e ic_aviso.png gerados")
