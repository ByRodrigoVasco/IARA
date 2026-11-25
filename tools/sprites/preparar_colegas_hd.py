"""Transforma as folhas de conceito dos Colegas de Plantão nas tiras do modo sobrevivência.

Uso, na raiz do repositório (precisa do Pillow e do NumPy):
    python tools/sprites/preparar_colegas_hd.py              # os 4 colegas
    python tools/sprites/preparar_colegas_hd.py enfermeira   # só um

As folhas ficam em docs/arte/conceito/colegas/<nome>.png: 4 linhas (frente, lado, costas, golpe de
lado) com 4 figuras cada, em fundo transparente.

Saída: sobrevivencia/en_colega_<nome>.png, 16 quadros de 192x192 numa fileira, na ordem das linhas.
A caminhada usa o mesmo ciclo dos personagens. No golpe os pés ficam no lugar e o corpo avança.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

from compactar_hd import compactar
from preparar_personagem_hd import (ALFA_MINIMO, BALANCO, BASE, LADO, ciclo, componentes, limpar,
                                    no_quadro, reduzir, sangrar, tirar_rebarbas)

NOMES = ["seguranca", "suporte", "enfermeira", "entregador"]
FOLHA = "docs/arte/conceito/colegas/{}.png"
SAIDA = "code/assets/sprites/sobrevivencia/en_colega_{}.png"
ALTURA = round(30 * LADO / 32)  # a mesma dos personagens
PES = 12                        # linhas acima da base usadas para achar os pés


def figuras_da_folha(folha):
    """As 16 figuras, linha por linha, achadas pelo contorno."""
    ilhas = sorted(componentes(folha[:, :, 3] > 0), key=len, reverse=True)
    if len(ilhas) < 16 or len(ilhas[15]) < len(ilhas[0]) // 3:
        raise SystemExit("Esperava 16 figuras separadas na folha")
    figuras = []
    for pixels in ilhas[:16]:
        ys, xs = np.array(pixels).T
        figura = np.zeros((ys.max() - ys.min() + 1, xs.max() - xs.min() + 1, 4), np.uint8)
        figura[ys - ys.min(), xs - xs.min()] = folha[ys, xs]
        figuras.append(((ys.min() + ys.max()) / 2, (xs.min() + xs.max()) / 2, figura))
    figuras.sort(key=lambda f: f[0])
    return [[f[2] for f in sorted(figuras[i:i + 4], key=lambda f: f[1])] for i in range(0, 16, 4)]


def meio_dos_pes(quadro):
    xs = np.nonzero(quadro[BASE - PES:BASE, :, 3] > 127)[1]
    return (int(xs.min()) + int(xs.max())) / 2


def golpe(figuras, parado_de_lado, escala):
    """Os 4 quadros do soco, com o meio dos pés onde está no quadro parado de lado."""
    alvo = meio_dos_pes(parado_de_lado)
    quadros = []
    for figura in figuras:
        reduzida = reduzir(tirar_rebarbas(figura), escala)
        quadro = no_quadro(reduzida, LADO // 2)
        xs = np.nonzero(quadro[:, :, 3].any(axis=0))[0]
        desvio = int(round(alvo - meio_dos_pes(quadro)))
        desvio = max(-int(xs.min()), min(LADO - 1 - int(xs.max()), desvio))  # nada sai do quadro
        quadros.append(limpar(np.roll(quadro, desvio, axis=1)))
    return quadros


def sem_brilho(quadro):
    """No sobrevivência o vermelho vivo (R>=170, G<=95, B<=95) brilha no escuro: só olhos e sangue."""
    r, g, b = quadro[:, :, 0], quadro[:, :, 1], quadro[:, :, 2]
    vivo = (r >= 170) & (g <= 95) & (b <= 95) & (quadro[:, :, 3] > 0)
    quadro[:, :, 0][vivo] = 168
    return quadro


def montar(nome):
    folha = np.array(Image.open(FOLHA.format(nome)).convert("RGBA"))
    folha[:, :, 3] = np.where(folha[:, :, 3] > ALFA_MINIMO, 255, 0)
    linhas = figuras_da_folha(folha)
    quadros = []
    for direcao, figuras in zip(("south", "east", "north"), linhas[:3]):
        quadros += ciclo(nome, direcao, figuras, ALTURA)
    # O golpe usa a escala da caminhada de lado: a pose agachada continua mais baixa
    escala = (ALTURA - BALANCO) / tirar_rebarbas(linhas[1][0]).shape[0]
    quadros += golpe(linhas[3], quadros[4], escala)
    tira = Image.new("RGBA", (LADO * len(quadros), LADO))
    for i, quadro in enumerate(quadros):
        tira.paste(Image.fromarray(sangrar(sem_brilho(quadro.copy())), "RGBA"), (i * LADO, 0))
    tira.save(SAIDA.format(nome), optimize=True)
    compactar([Path(SAIDA.format(nome))])
    print(nome, "ok", tira.size)


if __name__ == "__main__":
    for nome in sys.argv[1:] or NOMES:
        montar(nome)
