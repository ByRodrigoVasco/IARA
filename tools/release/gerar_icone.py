"""
Gera o ícone do IARA a partir do logo da tela inicial (o "IARA" numa linha, com o sangue).

    python tools/release/gerar_icone.py

Saídas:
    code/assets/icons/iara.png  (256x256)  ícone da janela e da barra de tarefas (SetWindowIcon no main.c)
    tools/release/iara.ico      (16 a 256) ícone do iara.exe no Explorer, embutido pelo build_release.ps1

Precisa de Python 3 com Pillow.
"""

from pathlib import Path

from PIL import Image, ImageDraw

RAIZ = Path(__file__).resolve().parents[2]
LOGO = RAIZ / "code/assets/sprites/tela-inicial/iara-nome.png"
PNG = RAIZ / "code/assets/icons/iara.png"
ICO = RAIZ / "tools/release/iara.ico"

FUNDO = (38, 36, 46, 255)
BORDA = (200, 24, 32, 255)


def icone(lado=256):
    im = Image.new("RGBA", (lado, lado), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    raio, espessura = lado * 3 // 16, max(1, lado // 32)
    d.rounded_rectangle([0, 0, lado - 1, lado - 1], raio, fill=FUNDO)
    d.rounded_rectangle([0, 0, lado - 1, lado - 1], raio, outline=BORDA, width=espessura)
    # O logo inteiro numa linha, com o sangue escorrendo, como na tela inicial
    letras = Image.open(LOGO).convert("RGBA").crop((150, 40, 2100, 700))
    letras = letras.crop(letras.getbbox())
    margem = lado * 6 // 100
    letras.thumbnail((lado - 2 * margem, lado - 2 * margem), Image.LANCZOS)
    im.alpha_composite(letras, ((lado - letras.width) // 2, (lado - letras.height) // 2))
    return im


if __name__ == "__main__":
    grande = icone(256)
    grande.save(PNG)
    # Cada tamanho é desenhado de novo (borda e cantos proporcionais) em vez de só reduzido do 256
    tamanhos = [16, 24, 32, 48, 64, 128, 256]
    grande.save(ICO, sizes=[(t, t) for t in tamanhos], append_images=[icone(t) for t in tamanhos[:-1]])
    print(f"{PNG.relative_to(RAIZ)} e {ICO.relative_to(RAIZ)} gerados")
