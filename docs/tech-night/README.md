# Página da Tech Night

Página que o público abre pelo QR code no estande da Tech Night: explica o Modo Sobrevivência em poucos minutos, pelo celular.

Tem como jogar, a dica da lanterna, os cinco chefes, os monstros, os personagens, como funciona o ranking da feira e quem fez o jogo.

## Arquivos

| Arquivo | O que é |
|---|---|
| [`index.html`](index.html) | A página inteira em um arquivo só (cerca de 490 KB): estilo, sprites e imagens vão dentro dele. Só as fontes vêm do Google Fonts. |

Os sprites e as imagens das salas são os do jogo, tirados de `code/assets/sprites/sobrevivencia/`. As duas imagens do escuro foram montadas com a arte e as regras de luz de `code/src/survival/sv_light.c`. Elas não são capturas da tela do jogo.

## Publicação na Vercel

1. Na Vercel, abra **Add New → Project** e importe este repositório.
2. Em **Root Directory**, escolha `docs/tech-night`. Em **Framework Preset**, escolha **Other**. Não precisa de comando de build.
3. Clique em **Deploy**.

A Vercel publica a branch `main` por padrão. Enquanto a página não chegar na `main`, troque a branch em **Settings → Git → Production Branch** para `develop`.

A cada push na branch configurada na Vercel, a página é publicada de novo no mesmo endereço, então o QR code impresso continua valendo.

## Ver no computador

Abra o `index.html` direto no navegador. Não precisa de servidor.
