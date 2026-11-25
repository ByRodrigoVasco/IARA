"""Transforma as folhas de conceito nos quadros dos personagens em alta resolução.

Uso, na raiz do repositório (precisa do Pillow e do NumPy):
    python tools/sprites/preparar_personagem_hd.py            # os 12 personagens
    python tools/sprites/preparar_personagem_hd.py Laura JV   # só alguns

As folhas ficam em docs/arte/conceito/<pasta em minúsculas>.png: 7 linhas (south, south-east, east,
north-east, north, north-west, south-west) com 4 quadros cada, em fundo transparente.

O ciclo de caminhada é montado para não tremer:
  - quadros 0 e 2 são a mesma pose neutra;
  - nos quadros de passo (1 e 3) a cabeça é a do quadro neutro, emendada no pescoço pelo caminho em
    que as duas imagens mais se parecem, e só o corpo e as pernas mudam;
  - o passo sobe o corpo BALANCO pixels sem tirar os pés da linha de base.

O jogo desenha o quadro no mesmo espaço do pixel art de 32x32 e suaviza a redução
(set_character_filter em characters.c). Depois de gerar, rode tools/sprites/auditar_personagens.py.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

from compactar_hd import compactar, grupos

DIRECOES = ["south", "south-east", "east", "north-east", "north", "north-west", "south-west"]
QUADROS = 4
LADO = 192
BASE = LADO * 31 // 32     # linha logo abaixo dos pés, a mesma do pixel art original
BALANCO = 4                # quanto o corpo sobe no passo (cerca de 1,5 px na tela da campanha)
ALTURA_PADRAO = 30         # em linhas do original de 32
ALTURAS = {"Laura": 31, "Zito": 28}
PERSONAGENS = ["Estagiarios", "Laura", "Anderson", "Julio", "Nicollas", "Vasco", "Zito", "JV",
               "Lucas", "Baudel", "Guilherme", "Danilo"]
ALFA_MINIMO = 200          # abaixo disso é o halo em volta da figura
LUZ_DA_BORDA = 75          # borda mais clara que isto, com contorno escuro logo atrás, é rebarba
CONTORNO_ESCURO = 45
SANGRIA = 12
FAIXA_DO_PESCOCO = (0.42, 0.58)  # frações da altura onde a emenda pode passar
ALTURA_DO_QUADRIL = 0.84    # abaixo disto, entre as mãos, só há pernas
SO_ESPELHA_AS_PERNAS = {"Danilo"}
MEIA_LARGURA_DAS_PERNAS = 24
# Óculos que ficaram sem olhos na redução: (x0, y0, x1, y1) de cada pupila no quadro neutro
PUPILAS = {
    ("Baudel", "south"): [(77, 61, 82, 67), (111, 61, 116, 67)],
    ("Baudel", "south-east"): [(93, 63, 98, 69), (125, 63, 129, 69)],
    ("Baudel", "east"): [(115, 62, 119, 68)],
    ("Baudel", "south-west"): [(63, 60, 67, 66), (95, 60, 100, 66)],
}

# Defeitos que vêm das próprias folhas, vistos na revisão quadro a quadro.
# Nestas direções os dois passos levantam a mesma perna: (passo que fica, passo que vira o espelho dele)
PERNAS_ESPELHADAS = {
    ("Lucas", "south"): (3, 1), ("Lucas", "north"): (1, 3),
    ("Baudel", "south"): (1, 3), ("Baudel", "north"): (1, 3),
    ("Guilherme", "south"): (1, 3), ("Guilherme", "north"): (1, 3),
    ("Danilo", "north"): (1, 3),
}
# Calçado escuro que fica branco em alguns passos
SAPATO_ESCURO = {"Lucas": (48, 32, 28), "Guilherme": (40, 32, 30)}
# Borrões a apagar: (personagem, direção, passo) -> retângulo (x0, y0, x1, y1) no quadro
BORROES = {("Laura", "east", 3): (82, 153, 103, 163)}
# Restos cinzentos presos à pele, pintados de contorno: (personagem, direção, passo) -> retângulo
CINZAS = {("Laura", "east", 3): (83, 141, 96, 153)}


# ---------------------------------------------------------------- recorte

def componentes(mascara):
    """Lista das ilhas (8 vizinhos) de uma máscara, cada uma como lista de (y, x)."""
    altura, largura = mascara.shape
    visto = ~mascara
    ilhas = []
    for y0, x0 in zip(*np.nonzero(mascara)):
        if visto[y0, x0]:
            continue
        visto[y0, x0] = True
        pilha, pixels = [(y0, x0)], []
        while pilha:
            y, x = pilha.pop()
            pixels.append((y, x))
            for ny in range(max(0, y - 1), min(altura, y + 2)):
                for nx in range(max(0, x - 1), min(largura, x + 2)):
                    if not visto[ny, nx]:
                        visto[ny, nx] = True
                        pilha.append((ny, nx))
        ilhas.append(pixels)
    return ilhas


def separar_figuras(folha):
    """As 28 figuras, linha por linha, achadas pelo contorno e não por uma grade fixa."""
    ilhas = sorted(componentes(folha[:, :, 3] > 0), key=len, reverse=True)
    total = len(DIRECOES) * QUADROS
    if len(ilhas) < total or len(ilhas[total - 1]) < len(ilhas[0]) // 3:
        raise SystemExit(f"Esperava {total} figuras separadas na folha")
    figuras = []
    for pixels in ilhas[:total]:
        ys, xs = np.array(pixels).T
        figura = np.zeros((ys.max() - ys.min() + 1, xs.max() - xs.min() + 1, 4), np.uint8)
        figura[ys - ys.min(), xs - xs.min()] = folha[ys, xs]
        figuras.append(((ys.min() + ys.max()) / 2, (xs.min() + xs.max()) / 2, figura))
    figuras.sort(key=lambda f: f[0])
    return [[f[2] for f in sorted(figuras[i:i + QUADROS], key=lambda f: f[1])]
            for i in range(0, total, QUADROS)]


def vizinhos(mascara):
    """Quantos dos 8 vizinhos de cada pixel estão na máscara."""
    m = np.pad(mascara, 1).astype(np.int8)
    soma = sum(np.roll(np.roll(m, dy, 0), dx, 1) for dy in (-1, 0, 1) for dx in (-1, 0, 1)) - m
    return soma[1:-1, 1:-1]


def dilatar(mascara, passos):
    for _ in range(passos):
        mascara = mascara | (vizinhos(mascara) > 0)
    return mascara


def tirar_rebarbas(figura):
    """Remove a franja clara por fora do contorno e as pontas soltas de 1 pixel."""
    figura = np.pad(figura, ((2, 2), (2, 2), (0, 0)))
    opaco = figura[:, :, 3] > 0
    luz = figura[:, :, :3] @ np.array([0.299, 0.587, 0.114])
    for _ in range(3):
        borda = opaco & (vizinhos(opaco) < 8)
        escuro = opaco & (luz < CONTORNO_ESCURO)
        franja = borda & (luz > LUZ_DA_BORDA) & dilatar(escuro, 3)
        if not franja.any():
            break
        opaco &= ~franja
    for _ in range(2):
        opaco &= ~(opaco & (vizinhos(opaco) <= 2))
    figura[~opaco] = 0
    ys, xs = np.nonzero(opaco)
    return figura[ys.min():ys.max() + 1, xs.min():xs.max() + 1]


# ---------------------------------------------------------------- montagem

def reduzir(figura, escala):
    img = Image.fromarray(figura, "RGBA")
    tamanho = (round(img.width * escala), round(img.height * escala))
    return np.array(img.convert("RGBa").resize(tamanho, Image.Resampling.LANCZOS).convert("RGBA"))


def centro_da_cabeca(figura):
    """Meio horizontal do terço de cima, que não muda com o passo das pernas."""
    return np.nonzero(figura[:figura.shape[0] // 3, :, 3] > 127)[1].mean()


def no_quadro(figura, centro):
    """Coloca a figura com os pés na linha de base e a cabeça na coluna pedida."""
    altura, largura = figura.shape[:2]
    x = int(round(centro - centro_da_cabeca(figura)))
    x = max(0, min(LADO - largura, x))
    quadro = np.zeros((LADO, LADO, 4), np.uint8)
    quadro[BASE - altura:BASE, x:x + largura] = figura
    return quadro


def premultiplicado(quadro):
    q = quadro.astype(np.float32)
    q[:, :, :3] *= q[:, :, 3:4] / 255.0
    return q


def alinhar(mestre, passo):
    """Desloca o passo na horizontal até a parte de cima coincidir com a do quadro neutro."""
    topo = np.nonzero(mestre[:, :, 3].any(axis=1))[0][0]
    fim = topo + int((BASE - topo) * 0.55)
    referencia = premultiplicado(mestre)[topo:fim]
    erros = {dx: np.abs(premultiplicado(np.roll(passo, dx, axis=1))[topo:fim] - referencia).sum()
             for dx in range(-10, 11)}
    return np.roll(passo, min(erros, key=erros.get), axis=1)


def emendar(mestre, passo, faixa):
    """Parte de cima do mestre, parte de baixo do passo, cortando onde as duas mais se parecem."""
    topo = np.nonzero(mestre[:, :, 3].any(axis=1))[0][0]
    alto = BASE - topo
    de, ate = topo + int(alto * faixa[0]), topo + int(alto * faixa[1])
    a, b = premultiplicado(mestre), premultiplicado(passo)
    custo = ((a - b) ** 2).sum(axis=2)[de:ate]
    custo = custo + np.roll(custo, -1, axis=0)

    # Programação dinâmica: o caminho da esquerda para a direita de menor diferença
    linhas = custo.shape[0]
    acumulado = custo[:, 0].copy()
    volta = np.zeros((linhas, LADO), np.int16)
    for x in range(1, LADO):
        opcoes = np.stack([np.concatenate(([np.inf], acumulado[:-1])), acumulado,
                           np.concatenate((acumulado[1:], [np.inf]))])
        escolha = opcoes.argmin(axis=0)
        volta[:, x] = np.arange(linhas) + escolha - 1
        acumulado = opcoes[escolha, np.arange(linhas)] + custo[:, x]
    y = int(acumulado.argmin())
    saida = passo.copy()
    for x in range(LADO - 1, -1, -1):
        saida[:de + y + 1, x] = mestre[:de + y + 1, x]
        y = int(volta[y, x])
    return saida


def balancar(quadro):
    """Sobe o corpo BALANCO pixels, esticando as pernas: os pés não saem do chão."""
    topo = np.nonzero(quadro[:, :, 3].any(axis=1))[0][0]
    corte = topo + int((BASE - topo) * 0.84)
    saida = quadro.copy()
    saida[:corte - BALANCO] = quadro[BALANCO:corte]
    saida[corte - BALANCO:corte] = quadro[corte]
    return saida


def espelhar_passo(neutro, origem, destino, so_pernas):
    """O passo que levanta a outra perna, feito espelhando um passo bom no eixo do corpo.

    A cabeça continua a do neutro. Quem tem roupa de um lado só (a bolsa do Danilo) mantém também o
    tronco do passo original, e a emenda desce para o quadril.
    """
    pes = np.nonzero(neutro[BASE - 10:BASE, :, 3] > 127)[1]
    soma = int(pes.min() + pes.max())  # a coluna x vira soma - x
    espelho = np.roll(origem[:, ::-1], soma - (LADO - 1), axis=1)
    if so_pernas:
        topo = np.nonzero(neutro[:, :, 3].any(axis=1))[0][0]
        quadril = topo + int((BASE - topo) * ALTURA_DO_QUADRIL)
        meio = soma // 2
        colunas = slice(meio - MEIA_LARGURA_DAS_PERNAS, meio + MEIA_LARGURA_DAS_PERNAS + 1)
        saida = destino.copy()
        saida[quadril:, colunas] = espelho[quadril:, colunas]
        return saida
    return emendar(balancar(neutro), espelho, FAIXA_DO_PESCOCO)


def escurecer_sapato(quadro, cor):
    """Pinta de escuro o que for claro e sem cor na faixa dos pés, mantendo um leve brilho."""
    faixa = quadro[BASE - 28:BASE]  # o pé levantado leva o branco mais para cima
    rgb = faixa[:, :, :3].astype(int)
    # A calça fica abaixo desta luz: só o branco do tênis e o contorno claro dele passam
    claro = (rgb @ np.array([0.299, 0.587, 0.114]) > 100) & (faixa[:, :, 3] > 0)
    brilho = rgb[claro].mean(axis=1, keepdims=True) / 255.0
    faixa[:, :, :3][claro] = np.clip(np.array(cor) * (0.8 + 0.5 * brilho), 0, 255).astype(np.uint8)
    return quadro


def limpar(quadro):
    """Apaga cacos soltos e fecha furos minúsculos deixados pela redução e pela emenda."""
    opaco = quadro[:, :, 3] > 40
    quadro[~opaco] = 0
    ilhas = sorted(componentes(opaco), key=len, reverse=True)
    for ilha in ilhas[1:]:
        if len(ilha) < 60:
            ys, xs = np.array(ilha).T
            quadro[ys, xs] = 0
    for furo in componentes(quadro[:, :, 3] == 0):
        if len(furo) >= 14:
            continue
        for y, x in furo:
            ao_redor = quadro[max(0, y - 3):y + 4, max(0, x - 3):x + 4].reshape(-1, 4)
            cheios = ao_redor[ao_redor[:, 3] > 200]
            if len(cheios):
                quadro[y, x] = np.median(cheios, axis=0).astype(np.uint8)
    return quadro


def sangrar(quadro):
    """Espalha a cor da borda pelos pixels transparentes, para o filtro não puxar preto."""
    cheio = quadro[:, :, 3] > 0
    for _ in range(SANGRIA):
        for eixo, sentido in ((0, 1), (0, -1), (1, 1), (1, -1)):
            novo = ~cheio & np.roll(cheio, sentido, eixo)
            quadro[:, :, :3][novo] = np.roll(quadro[:, :, :3], sentido, eixo)[novo]
            cheio = cheio | novo
    return quadro


def ciclo(personagem, direcao, figuras, altura):
    """Os 4 quadros de uma direção a partir das 4 figuras da folha."""
    figuras = [tirar_rebarbas(f) for f in figuras]
    escala = (altura - BALANCO) / figuras[0].shape[0]
    neutro_reduzido = reduzir(figuras[0], escala)
    centro = (LADO - neutro_reduzido.shape[1]) // 2 + centro_da_cabeca(neutro_reduzido)
    neutro = limpar(no_quadro(neutro_reduzido, centro))
    for x0, y0, x1, y1 in PUPILAS.get((personagem, direcao), []):
        neutro[y0:y1, x0:x1] = (34, 24, 28, 255)
        neutro[y0:y0 + 2, x0:x0 + 2] = (250, 246, 240, 255)  # brilho
    quadros = [neutro]
    for i in (1, 3):
        passo = reduzir(figuras[i], (altura - BALANCO) / figuras[i].shape[0])
        passo = alinhar(neutro, no_quadro(passo, centro))
        quadros.append(limpar(balancar(emendar(neutro, passo, FAIXA_DO_PESCOCO))))
    saida = [neutro, quadros[1], neutro, quadros[2]]
    for i in (1, 3):
        if personagem in SAPATO_ESCURO:
            saida[i] = escurecer_sapato(saida[i], SAPATO_ESCURO[personagem])
        if (personagem, direcao, i) in BORROES:
            x0, y0, x1, y1 = BORROES[(personagem, direcao, i)]
            area = saida[i][y0:y1, x0:x1]
            area[area[:, :, :3].max(axis=2) < 60] = 0
            saida[i] = limpar(saida[i])
        if (personagem, direcao, i) in CINZAS:
            x0, y0, x1, y1 = CINZAS[(personagem, direcao, i)]
            area = saida[i][y0:y1, x0:x1]
            rgb = area[:, :, :3].astype(int)
            cinza = (rgb.max(axis=2) - rgb.min(axis=2) < 45) & (rgb.max(axis=2) > 110) & (area[:, :, 3] > 0)
            area[:, :, :3][cinza] = (28, 12, 18)
    if (personagem, direcao) in PERNAS_ESPELHADAS:
        fica, vira = PERNAS_ESPELHADAS[(personagem, direcao)]
        so_pernas = personagem in SO_ESPELHA_AS_PERNAS
        saida[vira] = limpar(espelhar_passo(neutro, saida[fica], saida[vira], so_pernas))
    return saida


def gerar(personagem):
    folha = Image.open(f"docs/arte/conceito/{personagem.lower()}.png").convert("RGBA")
    folha = np.array(folha)
    folha[:, :, 3] = np.where(folha[:, :, 3] > ALFA_MINIMO, 255, 0)
    pasta = Path("code/assets/sprites/PERSONAGENS") / personagem
    altura = round(ALTURAS.get(personagem, ALTURA_PADRAO) * LADO / 32)
    parados = {}
    for direcao, figuras in zip(DIRECOES, separar_figuras(folha)):
        for i, quadro in enumerate(ciclo(personagem, direcao, figuras, altura)):
            saida = Image.fromarray(sangrar(quadro.copy()), "RGBA")
            saida.save(pasta / "walk" / f"{direcao}-{i}.png", optimize=True)
            if i == 0:
                parados[direcao] = saida
    if personagem == "Estagiarios":
        # A campanha usa estes 8 arquivos quando a Estagiária está parada
        parados["west"] = parados["east"].transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        for direcao, imagem in parados.items():
            imagem.save(pasta / f"{direcao}.png", optimize=True)
    print(personagem, "ok")


if __name__ == "__main__":
    for nome in sys.argv[1:] or PERSONAGENS:
        gerar(nome)
    for nome, arquivos in grupos(set(sys.argv[1:] or PERSONAGENS)):
        compactar(arquivos)
