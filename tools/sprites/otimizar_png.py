"""Diminui os PNGs do jogo sem perder nenhum pixel.

Uso, na raiz do repositório (precisa do Pillow, o mesmo das outras ferramentas de tools/sprites):
    python tools/sprites/otimizar_png.py                 # code/assets e docs/assets/team
    python tools/sprites/otimizar_png.py pasta_ou_arquivo ...

Para cada PNG:
  - se a imagem tem até 256 cores (contando a transparência), vira PNG de paleta: cada pixel passa a
    ocupar 1 byte em vez de 4, com as mesmas cores e o mesmo alfa;
  - senão, é regravada em RGBA com a compressão máxima do PNG.
O arquivo só é trocado se ficar menor E se a imagem lida de volta for idêntica à original, pixel a
pixel (RGBA). O raylib abre PNG de paleta normalmente: na memória e na placa de vídeo nada muda.

Rode de novo depois de gerar ou editar sprites: as ferramentas de geração salvam em RGBA.
"""

import io
import sys
from pathlib import Path

from PIL import Image

PADRAO = ["code/assets", "docs/assets/team"]


def para_paleta(img):
    """PNG de paleta exato, ou None se a imagem tem mais de 256 cores."""
    cores = img.getcolors(256)
    if cores is None:
        return None
    paleta = [cor for _, cor in cores]
    indice = {cor: i for i, cor in enumerate(paleta)}
    p = Image.new("P", img.size)
    pixels = img.get_flattened_data() if hasattr(img, "get_flattened_data") else img.getdata()
    p.putdata([indice[c] for c in pixels])
    rgb = []
    for r, g, b, _ in paleta:
        rgb += [r, g, b]
    p.putpalette(rgb + [0] * (768 - len(rgb)))
    alfas = bytes(a for _, _, _, a in paleta)
    if any(a != 255 for a in alfas):
        p.info["transparency"] = alfas
    return p


def gravar(img, **opcoes):
    saida = io.BytesIO()
    img.save(saida, "PNG", optimize=True, **opcoes)
    return saida.getvalue()


def identica(dados, original):
    lida = Image.open(io.BytesIO(dados)).convert("RGBA")
    return lida.size == original.size and lida.tobytes() == original.tobytes()


def otimizar(caminho):
    antes = caminho.read_bytes()
    original = Image.open(io.BytesIO(antes)).convert("RGBA")
    candidatos = []

    paleta = para_paleta(original)
    if paleta is not None:
        transparencia = paleta.info.get("transparency")
        candidatos.append(gravar(paleta, transparency=transparencia) if transparencia else gravar(paleta))
    candidatos.append(gravar(original))

    melhor = min(candidatos, key=len)
    if len(melhor) < len(antes) and identica(melhor, original):
        caminho.write_bytes(melhor)
        return len(antes), len(melhor)
    return len(antes), len(antes)


def main():
    alvos = [Path(a) for a in (sys.argv[1:] or PADRAO)]
    arquivos = []
    for alvo in alvos:
        arquivos += [alvo] if alvo.is_file() else sorted(alvo.rglob("*.png"))

    total_antes = total_depois = trocados = 0
    for arquivo in arquivos:
        antes, depois = otimizar(arquivo)
        total_antes += antes
        total_depois += depois
        trocados += depois < antes

    print(f"{len(arquivos)} PNGs, {trocados} diminuídos: "
          f"{total_antes / 1048576:.1f} MB -> {total_depois / 1048576:.1f} MB")


if __name__ == "__main__":
    main()
