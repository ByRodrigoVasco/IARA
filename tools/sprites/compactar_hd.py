"""Diminui os sprites em alta resolução: PNG de paleta com 256 cores, sem mudar o tamanho da imagem.

Uso, na raiz do repositório (precisa do Pillow e do NumPy), depois de gerar os sprites:
    python tools/sprites/compactar_hd.py              # personagens e Colegas de Plantão
    python tools/sprites/compactar_hd.py Laura JV     # só os personagens pedidos

A paleta é uma só para todos os quadros do personagem, para a cor não tremer na caminhada. A
transparência entra na paleta, e a cor espalhada em volta da figura (usada pelo filtro) é mantida.
Rodar de novo sobre um arquivo já compactado não muda nada.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

PERSONAGENS = Path("code/assets/sprites/PERSONAGENS")
SOBREVIVENCIA = Path("code/assets/sprites/sobrevivencia")
CORES = 256
PASSOS = 24
PESO_ALFA = 1.6            # a borda suave pesa mais que a cor na escolha
PESO_TRANSPARENTE = 0.04   # a cor escondida quase não disputa lugar na paleta


def empacotar(rgba):
    return rgba.astype(np.uint32).reshape(-1, 4) @ np.array([1, 256, 65536, 16777216], np.uint32)


def mais_perto(pontos, centros):
    """O centro mais perto de cada ponto, em blocos para caber na memória."""
    saida = np.empty(len(pontos), np.int64)
    quadrados = (centros ** 2).sum(axis=1)
    for i in range(0, len(pontos), 8192):
        bloco = pontos[i:i + 8192]
        saida[i:i + 8192] = (quadrados - 2.0 * bloco @ centros.T).argmin(axis=1)
    return saida


def paleta_de(cores, pesos):
    """k-médias com peso, partindo de cores bem separadas entre si."""
    sorteio = np.random.default_rng(7)  # sempre o mesmo resultado
    centros = [cores[pesos.argmax()]]
    longe = ((cores - centros[0]) ** 2).sum(axis=1)
    for _ in range(CORES - 1):
        chance = longe * pesos
        i = sorteio.choice(len(cores), p=chance / chance.sum())
        centros.append(cores[i])
        longe = np.minimum(longe, ((cores - cores[i]) ** 2).sum(axis=1))
    centros = np.array(centros)
    for _ in range(PASSOS):
        dono = mais_perto(cores, centros)
        soma = np.zeros(CORES)
        np.add.at(soma, dono, pesos)
        novo = np.zeros_like(centros)
        np.add.at(novo, dono, cores * pesos[:, None])
        usados = soma > 0
        centros[usados] = novo[usados] / soma[usados, None]
    return centros


def compactar(arquivos):
    """Regrava um grupo de PNGs com a mesma paleta. Devolve (bytes antes, bytes depois)."""
    imagens = [Image.open(a) for a in arquivos]
    antes = sum(a.stat().st_size for a in arquivos)
    if all(i.mode == "P" for i in imagens):
        return antes, antes
    quadros = [np.array(i.convert("RGBA")) for i in imagens]
    chaves = np.concatenate([empacotar(q) for q in quadros])
    unicas, vezes = np.unique(chaves, return_counts=True)
    cores = np.stack([(unicas >> d) & 255 for d in (0, 8, 16, 24)], axis=1).astype(float)
    pesos = vezes * np.where(cores[:, 3] == 0, PESO_TRANSPARENTE, 1.0)
    escala = np.array([1.0, 1.0, 1.0, PESO_ALFA])
    centros = paleta_de(cores * escala, pesos) / escala
    paleta = np.clip(np.rint(centros), 0, 255).astype(np.uint8)
    # Transparente continua transparente e opaco continua opaco
    paleta[paleta[:, 3] < 8, 3] = 0
    paleta[paleta[:, 3] > 247, 3] = 255
    indice = np.empty(len(unicas), np.int64)
    for grupo in (cores[:, 3] == 0, cores[:, 3] == 255, (cores[:, 3] > 0) & (cores[:, 3] < 255)):
        alfa = paleta[:, 3]
        if not grupo.any():
            continue
        if cores[grupo][0, 3] == 0:
            servem = np.nonzero(alfa == 0)[0]
        elif (cores[grupo][:, 3] == 255).all():
            servem = np.nonzero(alfa == 255)[0]
        else:
            servem = np.arange(CORES)
        if not len(servem):
            servem = np.arange(CORES)
        indice[grupo] = servem[mais_perto(cores[grupo] * escala, paleta[servem] * escala)]
    for arquivo, quadro in zip(arquivos, quadros):
        mapa = indice[np.searchsorted(unicas, empacotar(quadro))].reshape(quadro.shape[:2]).astype(np.uint8)
        img = Image.fromarray(mapa, "P")
        img.putpalette(paleta[:, :3].reshape(-1).tolist())
        img.save(arquivo, "PNG", optimize=True, transparency=paleta[:, 3].tobytes())
    return antes, sum(a.stat().st_size for a in arquivos)


def grupos(nomes):
    for pasta in sorted(PERSONAGENS.iterdir()):
        if pasta.is_dir() and (pasta / "walk").is_dir() and (not nomes or pasta.name in nomes):
            arquivos = sorted((pasta / "walk").glob("*.png"))
            if pasta.name == "Estagiarios":  # as 8 poses paradas da campanha usam a mesma paleta
                arquivos += sorted(a for a in pasta.glob("*.png") if not a.name.startswith("frame_"))
            yield pasta.name, arquivos
    if not nomes:
        for arquivo in sorted(SOBREVIVENCIA.glob("en_colega_*.png")):
            yield arquivo.stem, [arquivo]


if __name__ == "__main__":
    total = [0, 0]
    for nome, arquivos in grupos(set(sys.argv[1:])):
        antes, depois = compactar(arquivos)
        total = [total[0] + antes, total[1] + depois]
        print(f"{nome}: {antes / 1024:.0f} KB -> {depois / 1024:.0f} KB")
    print(f"total: {total[0] / 1048576:.2f} MB -> {total[1] / 1048576:.2f} MB")
