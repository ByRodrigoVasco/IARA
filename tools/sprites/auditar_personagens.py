"""Confere recorte e animação dos quadros em alta resolução dos personagens.

Uso, na raiz do repositório:
    python tools/sprites/auditar_personagens.py [pasta_para_previas]

Reprova: tamanho errado, pés fora da linha de base, cabeça que treme entre os quadros, caco solto,
furo minúsculo, quadro cortado na borda e passo sem movimento das pernas. Mostra também a parte do
contorno que é clara (suspeita de rebarba). Com uma pasta, grava folha e GIF de cada personagem.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

from preparar_personagem_hd import (BALANCO, BASE, DIRECOES, LADO, PERSONAGENS, componentes,
                                    vizinhos)

FUNDO = (105, 112, 130, 255)


def perna_levantada(pasta, direcao, i):
    """'E' ou 'D' conforme a metade que tem menos pé encostado no chão durante o passo."""
    neutro = np.array(Image.open(pasta / f"{direcao}-0.png").convert("RGBA"))[:, :, 3] > 127
    xs = np.nonzero(neutro[BASE - 10:BASE])[1]
    meio = int(xs.min() + xs.max() + 1) // 2
    chao = np.array(Image.open(pasta / f"{direcao}-{i}.png").convert("RGBA"))[BASE - 8:BASE, :, 3] > 127
    esquerda, direita = chao[:, :meio].sum(), chao[:, meio:].sum()
    return "E" if esquerda < 0.75 * direita else "D" if direita < 0.75 * esquerda else "?"


def auditar(personagem):
    pasta = Path("code/assets/sprites/PERSONAGENS") / personagem / "walk"
    falhas, claras, pernas = [], [], []
    for direcao in DIRECOES:
        q = [np.array(Image.open(pasta / f"{direcao}-{i}.png").convert("RGBA")) for i in range(4)]
        if any(a.shape != (LADO, LADO, 4) for a in q):
            falhas.append(f"{direcao}: tamanho")
            continue
        topos = []
        for i, a in enumerate(q):
            opaco = a[:, :, 3] > 127
            ys, xs = np.nonzero(opaco)
            topos.append(ys.min())
            if abs((ys.max() + 1) - BASE) > 1:
                falhas.append(f"{direcao}-{i}: pés em {ys.max() + 1}, base {BASE}")
            if xs.min() == 0 or xs.max() == LADO - 1 or ys.min() == 0 and personagem != "Laura":
                falhas.append(f"{direcao}-{i}: encosta na borda do quadro")
            soltos = [c for c in componentes(opaco)[:] if len(c) < 60]
            if soltos:
                falhas.append(f"{direcao}-{i}: {len(soltos)} caco(s) solto(s)")
            furos = [f for f in componentes(~opaco) if len(f) < 8]
            if len(furos) > 4:  # até 4 vãos minúsculos são fios de cabelo da própria arte
                falhas.append(f"{direcao}-{i}: {len(furos)} furos")
            borda = opaco & (vizinhos(opaco) < 8)
            luz = a[:, :, :3] @ np.array([0.299, 0.587, 0.114])
            claras.append((luz[borda] > 90).mean())
        # Cabeça: no passo ela é a do neutro, só mais alta
        alto = BASE - topos[0]
        fim = topos[0] + int(alto * 0.38)
        for i in (1, 3):
            subida = topos[0] - topos[i]
            if not 0 <= subida <= BALANCO + 1:
                falhas.append(f"{direcao}-{i}: corpo sobe {subida}")
            a = q[0][topos[0]:fim].astype(int)
            b = q[i][topos[i]:topos[i] + (fim - topos[0])].astype(int)
            visivel = (a[:, :, 3] > 127) | (b[:, :, 3] > 127)
            tremor = (np.abs(a - b).max(axis=2)[visivel] > 40).mean()
            if tremor > 0.02:
                falhas.append(f"{direcao}-{i}: cabeça muda {tremor:.0%}")
        baixo = slice(topos[0] + int(alto * 0.72), BASE)
        for i, j in ((0, 1), (0, 3), (1, 3)):
            diferenca = (np.abs(q[i][baixo].astype(int) - q[j][baixo].astype(int)).max(axis=2) > 60).sum()
            pernas.append((diferenca, f"{direcao} {i}x{j}"))
            if diferenca < 120:
                falhas.append(f"{direcao}: pernas iguais nos quadros {i} e {j} ({diferenca})")
    for direcao in ("south", "north"):
        lados = [perna_levantada(pasta, direcao, i) for i in (1, 3)]
        if "?" in lados or lados[0] == lados[1]:
            falhas.append(f"{direcao}: os passos não alternam as pernas {lados}")
    print(f"{personagem:12s} borda clara média {np.mean(claras):.1%} pior {max(claras):.1%} | "
          f"menor movimento de pernas {min(pernas)[0]} ({min(pernas)[1]}) | {len(falhas)} falha(s)")
    for f in falhas:
        print("     ", f)
    return len(falhas)


def previas(personagem, destino):
    pasta = Path("code/assets/sprites/PERSONAGENS") / personagem / "walk"
    folha = Image.new("RGBA", (LADO * 4, LADO * len(DIRECOES)), FUNDO)
    animacao = []
    for linha, direcao in enumerate(DIRECOES):
        for i in range(4):
            folha.alpha_composite(Image.open(pasta / f"{direcao}-{i}.png").convert("RGBA"),
                                  (i * LADO, linha * LADO))
    for i in range(4):
        tira = folha.crop((i * LADO, 0, (i + 1) * LADO, folha.height))
        quadro = Image.new("RGBA", (LADO * len(DIRECOES), LADO), FUNDO)
        for linha in range(len(DIRECOES)):
            quadro.paste(tira.crop((0, linha * LADO, LADO, (linha + 1) * LADO)), (linha * LADO, 0))
        animacao.append(quadro.convert("RGB"))
    folha.save(destino / f"{personagem}.png")
    animacao[0].save(destino / f"{personagem}.gif", save_all=True, append_images=animacao[1:],
                     duration=120, loop=0)


if __name__ == "__main__":
    total = 0
    for nome in PERSONAGENS:
        total += auditar(nome)
        if len(sys.argv) > 1:
            Path(sys.argv[1]).mkdir(parents=True, exist_ok=True)
            previas(nome, Path(sys.argv[1]))
    print("TOTAL de falhas:", total)
    sys.exit(1 if total else 0)
