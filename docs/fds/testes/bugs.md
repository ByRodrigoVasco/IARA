# Bug tracker — bugs encontrados nos testes de sistema

Os BUG-01 a BUG-08 foram encontrados na execução dos testes de sistema de 22/10/2026
(branch `develop`, commit `b366e7c`) e estão registrados como issue no repositório, com a
label `bug`. Os BUG-09 a BUG-12 foram encontrados na revisão de 03/11/2026
(branch `test/campanha-100-dias`) e também têm issue. O BUG-13 foi relatado em 08/11/2026, durante o
desenvolvimento do Modo Sobrevivência (branch `feature/modo-sobrevivencia`), e ainda não tem issue. Os BUG-14 a BUG-16
saíram do teste à mão de 08/11/2026 e do retorno de um usuário final de 09/11/2026, na mesma branch, e também não têm issue.

Os blocos abaixo repetem o conteúdo de cada um, para consulta sem sair do repositório.

## Status

Atualizar a coluna Status conforme as issues forem corrigidas e fechadas. Conferido em 03/11/2026:
do BUG-01 ao BUG-11 as correções estão na `develop` e as issues foram fechadas. Só o BUG-12 seguia aberto.
Em 08/11/2026, na branch `feature/modo-sobrevivencia`, o BUG-12 foi resolvido e o BUG-13 foi corrigido, e o usuário
testou os dois no jogo real e aprovou. Em 09/11/2026 o BUG-14 foi corrigido na mesma branch. Esses três só chegam
à `develop` quando a branch for para lá, e a issue do BUG-12 fica aberta até então. Os BUG-15 e BUG-16 seguem
abertos, com gravidade baixa: são quadros lentos medidos no teste real, sem efeito na jogabilidade.

| ID | Problema | Gravidade | Issue | Status |
|---|---|---|---|---|
| BUG-01 | Fade in para em alpha 250 | Baixa | [#32](https://github.com/ByRodrigoVasco/IARA/issues/32) | Corrigido na `develop`, issue fechada |
| BUG-02 | Voltar ao menu vaza memória | Alta | [#33](https://github.com/ByRodrigoVasco/IARA/issues/33) | Corrigido na `develop`, issue fechada |
| BUG-03 | Textura do jogador liberada duas vezes | Média | [#34](https://github.com/ByRodrigoVasco/IARA/issues/34) | Corrigido na `develop`, issue fechada |
| BUG-04 | Funções de unload nunca chamadas | Baixa | [#35](https://github.com/ByRodrigoVasco/IARA/issues/35) | Corrigido na `develop`, issue fechada |
| BUG-05 | Desenho fora do `BeginDrawing`/`EndDrawing` | Baixa | [#36](https://github.com/ByRodrigoVasco/IARA/issues/36) | Corrigido na `develop`, issue fechada |
| BUG-06 | Caminho de sprites em minúsculo | Média | [#37](https://github.com/ByRodrigoVasco/IARA/issues/37) | Corrigido na `develop`, issue fechada |
| BUG-07 | Pré-menu não descarrega imagens | Média | [#38](https://github.com/ByRodrigoVasco/IARA/issues/38) | Corrigido na `develop`, issue fechada |
| BUG-08 | Texto do quiz vaza para fora da caixa | Média | [#39](https://github.com/ByRodrigoVasco/IARA/issues/39) | Corrigido na `develop`, issue fechada |
| BUG-09 | Rótulos das mesas vazam dos botões | Média | [#43](https://github.com/ByRodrigoVasco/IARA/issues/43) | Corrigido na `develop`, issue fechada |
| BUG-10 | Falha ao trocar o save pode perder o progresso | Média | [#44](https://github.com/ByRodrigoVasco/IARA/issues/44) | Corrigido na `develop`, issue fechada |
| BUG-11 | Fundo do cartão do dia é carregado e nunca desenhado | Baixa | [#45](https://github.com/ByRodrigoVasco/IARA/issues/45) | Corrigido na `develop`, issue fechada |
| BUG-12 | Menu não tem como começar uma partida nova | Média | [#46](https://github.com/ByRodrigoVasco/IARA/issues/46) | Resolvido na `feature/modo-sobrevivencia` (`ad1ec96`) e aprovado em teste real; issue aberta até a branch ir para a `develop` |
| BUG-13 | Carregamentos sem retorno visual e engasgos no menu | Média | ainda sem issue | Corrigido na `feature/modo-sobrevivencia` e aprovado em teste real |
| BUG-14 | Telas de modo e de save em lista vertical, pouco organizadas | Baixa | ainda sem issue | Corrigido na `feature/modo-sobrevivencia` |
| BUG-15 | Quadro de 1,67 s no menu, sete segundos depois de abrir | Baixa | ainda sem issue | Aberto, não reproduzido |
| BUG-16 | Troca de música e ambiente lê o arquivo na hora | Baixa | ainda sem issue | Aberto |

**Sobre o BUG-02:** os dois vazamentos descritos abaixo já não existem. Restava uma carga duplicada do menu ao abrir o jogo, removida. O CT-24 oscilava porque media sem desenhar quadros, e o driver de vídeo só devolve uma textura apagada depois de um quadro. Com quadros, 400 voltas ao menu cresceram 19 MB (cerca de 0,05 MB por volta, contra os 7 MB originais).

---

<a id="bug-01"></a>

## BUG-01 — Fade in nunca chega a 100% de opacidade

**Labels:** `bug`, `ui`, `gravidade: baixa`

Todas as telas do jogo aparecem com cerca de 98% de opacidade em vez de 100%, de
forma permanente, porque a animação de fade in termina antes de completar.

Em `fade_in_texture()` a condição de parada é `(color->a + fade_speed) < 255`.
Quando o alpha chega perto do fim, a soma deixa de ser menor que 255, a função
marca a animação como concluída e **sai sem atribuir 255 ao alpha**. O valor final
depende da velocidade do fade: com `MEDIUM_FADE_SPEED` (5) o alpha congela em 250.

O `fade_out_texture()` logo abaixo faz certo: ele atribui `color->a = 0` antes de
sinalizar o fim.

**Como reproduzir**

```powershell
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

O caso CT-15 falha com a mensagem `fade in terminou com alpha = 250 (esperado 255)`.

**Esperado:** o fade in termina com alpha 255.
**Obtido:** termina com alpha 250 e a textura nunca fica totalmente opaca.

**Onde:** `code/src/ui/animations.c:10`
**Coberto por:** CT-15 (`code/tests/test_sistema.c`)

---

<a id="bug-02"></a>

## BUG-02 — Voltar ao menu vaza memória sem limite

**Labels:** `bug`, `performance`, `gravidade: alta`

Cada vez que o menu principal é aberto, o jogo consome memória que nunca mais
devolve. Como o jogo volta ao menu depois dos créditos — e vai voltar mais vezes
conforme as telas forem implementadas — o consumo cresce sem teto durante a
sessão.

São dois vazamentos na mesma textura, o fundo secundário do menu:

1. `initialize_menu_textures()` carrega `second_bg_image`, redimensiona para o
   tamanho da tela inteira e envia para a GPU, mas **não chama `UnloadImage()`**
   nessa imagem. As outras três (`bg_office_image`, `iara_name_image`,
   `arrow_icon_image`) são descarregadas corretamente logo abaixo.
2. `unload_menu()` descarrega `bg_office_texture`, `arrow_icon_texture` e
   `iara_name_gt`, mas **esquece `second_bg_texture`**.

Em 1280x720 cada volta ao menu deixa para trás uma imagem de 1280 × 720 × 4 bytes
(cerca de 3,5 MB em RAM) mais a textura equivalente na placa de vídeo.

**Como reproduzir**

```powershell
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

O caso CT-24 abre e fecha o menu 20 vezes medindo a memória do processo e falha com:

```
memoria do processo: 92 MB antes, 229 MB depois de 20 ciclos
crescimento de 137 MB (limite aceitavel: 16 MB)
```

**Esperado:** abrir e fechar o menu deixa a memória estável.
**Obtido:** cerca de 7 MB a mais por volta ao menu, sem nunca liberar.

**Onde:** `code/src/screens/menu.c:52` (falta `UnloadImage`) e
`code/src/screens/menu.c:269` (falta `UnloadTexture`)
**Coberto por:** CT-24 (`code/tests/test_conteudo.c`)

---

<a id="bug-03"></a>

## BUG-03 — `unload_office()` libera a textura do jogador duas vezes

**Labels:** `bug`, `gravidade: média`

`current_estagiario_texture_direction` é sempre uma cópia de um dos elementos de
`estagiario_texture[]` — a direção para onde o personagem está olhando. As duas
variáveis guardam o **mesmo id de textura na placa de vídeo**.

`unload_office()` descarrega primeiro `current_estagiario_texture_direction` e
logo depois percorre `estagiario_texture[]` inteiro, descarregando de novo aquele
mesmo id.

Hoje o efeito é inofensivo porque o OpenGL ignora o pedido de apagar um id que já
não existe, e nenhuma textura nova é criada entre as duas chamadas. Mas é uma
liberação dupla à espera de virar problema: basta a ordem mudar para o jogo passar
a apagar uma textura recém-criada que tenha reaproveitado o id.

**Correção sugerida:** remover a linha solta e deixar só o laço, que já cobre as
8 direções.

**Onde:** `code/src/screens/game_loop.c:695`
**Coberto por:** nenhum teste automatizado (achado por inspeção)

---

<a id="bug-04"></a>

## BUG-04 — `unload_game_loop()` e `unload_computer_loop()` nunca são chamadas

**Labels:** `bug`, `limpeza`, `gravidade: baixa`

As duas funções existem, estão documentadas nos cabeçalhos e fazem a limpeza
correta — mas nenhuma delas é chamada em lugar nenhum do projeto. A `main.c`
encerra o jogo chamando apenas `unload_fonts()` e `CloseWindow()`.

Na prática o sistema operacional recupera tudo quando o processo termina, então
não há sintoma visível hoje. O problema é que a limpeza fica sem uso e vai
apodrecendo: quando o jogo passar a trocar de dia e recarregar o escritório, a
falta dessas chamadas vira vazamento de verdade.

Vale conferir junto com o [BUG-03](#bug-03): `unload_game_loop()` repete a mesma
liberação dupla de `unload_office()` se as duas forem chamadas em sequência.

**Onde:** `code/src/screens/game_loop.c:817`, `code/src/screens/computer_loop.c:456`,
`code/src/main.c:55`
**Coberto por:** nenhum teste automatizado (achado por inspeção)

---

<a id="bug-05"></a>

## BUG-05 — `close_pre_menu()` desenha fora do `BeginDrawing`/`EndDrawing`

**Labels:** `bug`, `ui`, `gravidade: baixa`

`render_pre_menu()` fecha o quadro com `EndDrawing()` e **depois** chama
`close_pre_menu()`, que executa `ClearBackground()` e dois `DrawTexture()`.

Esses desenhos caem fora do par `BeginDrawing`/`EndDrawing`. A raylib acumula as
chamadas em lote e só as envia no `EndDrawing()` seguinte, então o fade de saída
do pré-menu aparece um quadro atrasado, misturado ao quadro seguinte. Funciona por
acidente, não por desenho.

**Correção sugerida:** mover a chamada de `close_pre_menu()` para dentro do bloco
de desenho, antes do `EndDrawing()`.

**Onde:** `code/src/screens/pre_menu.c:58`
**Coberto por:** nenhum teste automatizado (achado por inspeção)

---

<a id="bug-06"></a>

## BUG-06 — `PERSONAGENS_PATH` em minúsculo quebra o jogo fora do Windows

**Labels:** `bug`, `portabilidade`, `gravidade: média`

A macro aponta para `code/assets/sprites/personagens/`, em minúsculo, mas a pasta
versionada no repositório é `code/assets/sprites/PERSONAGENS/` — o `git ls-files`
confirma o nome em maiúsculo.

No Windows o sistema de arquivos ignora a diferença e tudo carrega. Em Linux e
macOS, onde o caminho é sensível a maiúsculas, **nenhum sprite de personagem
carrega**: o jogador fica invisível e `load_walk_frames()` devolve `false` para
todos os personagens.

Isso afeta todas as macros derivadas (`ESTAGIARIO_1_*`) e o caminho montado dentro
de `load_walk_frames()`.

**Correção sugerida:** escolher um dos dois e usar sempre o mesmo. Renomear a pasta
para minúsculo no git exige `git mv` em dois passos, porque o Windows não distingue
os dois nomes; corrigir a macro para `PERSONAGENS/` é o caminho mais curto.

**Onde:** `code/src/core/macros.h:72`
**Coberto por:** CT-03 e CT-05 passam no Windows e não detectariam isso em outro sistema

---

<a id="bug-07"></a>

## BUG-07 — Pré-menu nunca descarrega as imagens da memória RAM

**Labels:** `bug`, `performance`, `gravidade: média`

`initialize_pre_menu_textures()` carrega duas imagens, redimensiona a primeira
para a tela cheia e envia as duas para a placa de vídeo — mas não chama
`UnloadImage()` em nenhuma das duas. O arquivo inteiro não tem uma única chamada
de `UnloadImage`.

Depois do upload para a GPU, a cópia em RAM não serve para mais nada. Em 1280x720
são cerca de 3,5 MB da imagem do escritório, mais a do nome do jogo, presos até o
jogo fechar.

É o mesmo descuido do [BUG-02](#bug-02), em outra tela. A diferença é que o
pré-menu roda só uma vez por sessão, então não cresce.

**Correção sugerida:** chamar `UnloadImage()` nas duas imagens no fim de
`initialize_pre_menu_textures()`, como `menu.c` faz com três das suas quatro.

**Onde:** `code/src/screens/pre_menu.c:29` e `code/src/screens/pre_menu.c:32`
**Coberto por:** nenhum teste automatizado (achado por inspeção)

---

<a id="bug-08"></a>

## BUG-08 — Texto do quiz vaza para fora da caixa e dos botões

**Labels:** `bug`, `ui`, `gravidade: média`

Na tela do computador, enunciados e alternativas mais longos do que a caixa são
desenhados por cima da borda e saem pelos dois lados, cortados.

`render_current_step()` calcula a largura da caixa como
`maior_linha + 2 × padding`, mas aplica um teto de 85% da largura da janela
(`QUIZ_MAX_WIDTH_RATIO`). Quando o texto passa desse teto, **a caixa para de
crescer e o texto não**: ele continua sendo desenhado centralizado, na largura
que tiver. O mesmo acontece com os botões de alternativa.

Falta quebra de linha automática ou redução da fonte quando o texto não cabe.

**Como reproduzir**

```powershell
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

O caso CT-35 refaz a mesma conta de largura e lista os oito pontos onde o texto
passa do limite:

```
passo 1: texto tem 594 px e a caixa so tem 532 px
passo 3, opcao 1: texto tem 677 px e o botao so tem 532 px
...
```

O pior caso é a alternativa B da pergunta sobre viés
("Tratamento desigual devido aos dados de treino"), com 677 px num botão de
532 px — 27% a mais do que cabe.

Dá para ver o efeito na evidência
[`evidencias/quiz-respondido.png`](evidencias/quiz-respondido.png): o enunciado
"O que é Inteligência Artificial, no sentido usado no dia a dia?" aparece com a
primeira e a última letra fora da caixa cinza.

**Esperado:** o texto cabe dentro da caixa e dos botões, em qualquer resolução.
**Obtido:** oito trechos do roteiro atual vazam pelas laterais.

**Onde:** `code/src/screens/computer_loop.c:266` (caixa) e
`code/src/screens/computer_loop.c:350` (botões de alternativa)
**Coberto por:** CT-35 (`code/tests/test_fluxo.c`)

---

<a id="bug-09"></a>

## BUG-09 — Rótulos das mesas vazam dos botões

**Labels:** `bug`, `ui`, `gravidade: média`

Nas mesas dos dias seguintes, os botões de marcar usavam a fonte do quiz sem conferir se o
texto cabia. O CT-35 não pegava o problema porque só media o roteiro do Dia 1 e não olhava
esses botões.

Medido em 6 tamanhos de janela, do mínimo (960x540) a 2560x1080:

- `[Verdade]`, no Dia 27, passa cerca de 13% da largura do botão em todos os tamanhos
  (75 px em 66 px na janela mínima);
- `[Difere]`, no Dia 12, e `[Ativar]`, no Dia 58, ocupam o botão inteiro, sem margem;
- no Dia 41, as linhas ficam mais baixas que a fonte (17,9 px para fonte de 19,5 px na janela
  mínima), então o texto dos botões passa da altura deles.

**Esperado:** o texto cabe nos botões das mesas, em qualquer dia e tamanho de janela.
**Obtido:** o texto encosta na borda do botão ou passa dela.

**Correção:** `get_board_button_font_size()` reduz a fonte para o texto caber na largura e na
altura de cada botão da mesa.

**Onde:** `draw_choice_button()` e `render_ticket_triage()` em `code/src/screens/computer_loop.c`
**Coberto por:** CT-76 (`code/tests/test_fluxo.c`)

---

<a id="bug-10"></a>

## BUG-10 — Falha ao trocar o save pode perder o progresso

**Labels:** `bug`, `gravidade: média`

`save_game_state()` grava o save novo em `iara.save.tmp`, apaga `iara.save` e só então renomeia
o temporário. Se a troca falhasse depois de apagar o save antigo, o código apagava também o
temporário e o jogador ficava sem nenhum dos dois. Se o jogo ou o computador caísse entre as
duas etapas, o save antigo já não existia e o carregamento nem procurava o temporário.

Achado por inspeção; a falha não foi reproduzida em uso real.

**Esperado:** uma gravação interrompida deixa pelo menos o save anterior no disco.
**Obtido:** o jogador podia perder toda a campanha.

**Correção:** o save antigo vira `iara.save.bak` até o novo estar no lugar. Se a troca falhar,
ele volta; se o jogo cair no meio, `load_game_state()` carrega a cópia de segurança.

**Onde:** `save_game_state()` e `load_game_state()` em `code/src/core/save.c`
**Coberto por:** `code/tests/test_save.c` cobre a queda no meio da troca. O retorno do save
antigo quando o `rename()` falha não tem teste automatizado.

---

<a id="bug-11"></a>

## BUG-11 — Fundo do cartão do dia é carregado e nunca desenhado

**Labels:** `bug`, `limpeza`, `gravidade: baixa`

`initialize_game_loop_variables()` carregava `second-background.png` para a placa de vídeo a cada
início de partida, mas `render_calendar_day_card()` não desenha essa textura. O cartão continua
igual sem ela.

**Correção:** a textura e `unload_calendar_day_textures()` foram removidas.

**Onde:** `code/src/screens/game_loop.c`
**Coberto por:** nenhum teste automatizado (achado por inspeção)

---

<a id="bug-12"></a>

## BUG-12 — Menu não tem como começar uma partida nova

**Labels:** `bug`, `ui`, `gravidade: média`

O menu só oferece `INICIAR`, `CRÉDITOS`, `CONFIGURAÇÕES` e `SAIR`. Com um save no disco, `INICIAR`
sempre retoma a partida. Só dá para recomeçar terminando o Dia 100 ou apagando `iara.save` à mão,
o que atrapalha quem vai testar a campanha mais de uma vez.

**Esperado:** o jogador consegue começar do Dia 1, com uma confirmação para não perder a partida
sem querer.
**Obtido:** não há essa opção no jogo.

**Em aberto:** decidir se a opção fica no menu ou em `CONFIGURAÇÕES`.

**Correção (08/11/2026):** a decisão foi uma escolha em duas telas depois de `INICIAR`: primeiro o modo, depois
o save. Cada modo aceita vários saves, cada um com nome único e na própria pasta (`saves/normal/<nome>/`).
Criar um save novo começa do Dia 1 sem tocar nos outros, e um save pode ser apagado, com confirmação. Branch
`feature/modo-sobrevivencia`, commit `ad1ec96`. O usuário testou o fluxo no jogo real em 08/11/2026 e aprovou.
O visual das duas telas mudou depois, no BUG-14.

**Onde:** `code/src/screens/menu.c` e `handle_menu_action()` em `code/src/core/state_machine.c`
**Coberto por:** CT-79 a CT-87 (fluxo) e SL-01 a SL-29 (saves em pastas); antes, nenhum teste automatizado (achado por inspeção)

---

<a id="bug-13"></a>

## BUG-13 — Carregamentos sem retorno visual e engasgos no menu

**Labels:** `bug`, `desempenho`, `gravidade: média`

Relatado em 08/11/2026: o jogo travava algumas vezes e demorava para carregar, até no menu, e durante o
carregamento a tela ficava congelada, sem nenhum sinal de que algo acontecia.

**Esperado:** quadros regulares no menu e nas telas de escolha, e uma tela de carregamento com a porcentagem
sempre que a carga passar de um quadro.
**Obtido:** o fundo do menu tinha 3334x2160 e levava cerca de 210 ms para decodificar. O menu carregava o escritório
e o nome IARA que a abertura já tinha carregado (166 ms a mais), e as configurações recarregavam o escritório a
cada abertura. A partida do Sobrevivência decodificava todas as imagens e os sons antes do primeiro quadro,
com a tela parada.

**Correção:** (1) tela de carregamento com a porcentagem, que só cresce, e a carga dividida em passos, com
orçamento de 10 ms por quadro; (2) as imagens do menu carregam por trás da abertura, uma por quadro; (3) fundo do
menu reduzido para 1112x720 (15 ms para decodificar); (4) cada imagem grande carrega uma vez só, com contagem de
usos (`acquire_smooth_texture`); (5) o jogo grava em `iara-desempenho.log` os quadros acima de 60 ms. Medido com o
jogo real: em 18 s de abertura e menu parado, os únicos quadros acima de 60 ms foram dois, perto de 1,4 s, quando a
placa de vídeo usa pela primeira vez as imagens grandes.

**Teste real (08/11/2026):** o usuário jogou cerca de 18 minutos no Sobrevivência e aprovou. Depois do carregamento,
nenhum quadro passou de 60 ms durante a partida. Os quadros lentos que sobraram estão nos BUG-15 e BUG-16.

**Onde:** `code/src/screens/loading.c`, `code/src/screens/menu.c`, `code/src/core/helper.c`, `code/src/core/state_machine.c`, `code/src/main.c`
**Coberto por:** CT-88 (a porcentagem sobe) e CT-89 a CT-91 (imagens compartilhadas); os tempos foram medidos à parte, sem teste automatizado

---

<a id="bug-14"></a>

## BUG-14 — Telas de modo e de save em lista vertical, pouco organizadas

**Labels:** `bug`, `ui`, `gravidade: baixa`

Retorno de um usuário final, repassado em 09/11/2026: a escolha do modo mostrava os dois modos um embaixo do outro,
colados à esquerda, e a lista de saves era uma pilha de linhas sem separação, com o nome, o resumo e a data
misturados numa linha só.

**Esperado:** o modo no centro da tela, com os dois lado a lado e a opção de passar de um para o outro; os saves
em cartões organizados.
**Obtido:** lista vertical alinhada à esquerda nas duas telas.

**Correção (09/11/2026):** a escolha de modo virou dois cartões lado a lado no centro, com esquerda e direita para
passar de um para o outro, o cartão escolhido destacado e o botão `VOLTAR` embaixo. A lista de saves virou uma coluna
centralizada de cartões, cada um com o nome, o resumo do progresso à esquerda e a data à direita; o save escolhido
mostra o botão de apagar no canto, a barra de rolagem aparece quando nem todos cabem, e `VOLTAR` é um botão fixo
embaixo. O mouse e o teclado seguem iguais. Branch `feature/modo-sobrevivencia`.

**Onde:** `code/src/screens/mode_select.c`, `code/src/screens/save_select.c`, `code/src/ui/screen_style.c`
**Coberto por:** CT-79 a CT-87 (fluxo) seguem passando; o visual foi conferido por captura de tela em 1600x900 e 1280x720

---

<a id="bug-15"></a>

## BUG-15 — Quadro de 1,67 s no menu, sete segundos depois de abrir

**Labels:** `bug`, `desempenho`, `gravidade: baixa`

No teste real de 08/11/2026, `iara-desempenho.log` registrou um quadro de 1671 ms na tela do menu, 7,1 s depois
de abrir o jogo. Não voltou nos 18 minutos seguintes, e o jogo não registrou erro.

**Esperado:** nenhum quadro acima de 60 ms no menu.
**Obtido:** um quadro de 1,67 s. Entre a abertura e o menu o jogo só carrega quatro imagens pequenas e não inicia o áudio,
então a causa não está numa carga pesada do código. Pode ter sido um evento do Windows (arrastar ou focar a janela)
ou a placa de vídeo, mas isso não foi confirmado.

**Em aberto:** reproduzir. Se voltar, medir o tempo por etapa dentro do quadro (`handle_state_machine()` e `EndDrawing()`).

**Onde:** abertura e menu (`code/src/screens/pre_menu.c`, `code/src/screens/menu.c`)
**Coberto por:** nenhum teste; achado pelo log do jogo real

---

<a id="bug-16"></a>

## BUG-16 — Troca de música e ambiente lê o arquivo na hora

**Labels:** `bug`, `desempenho`, `gravidade: baixa`

No mesmo teste real, três quadros passaram de 60 ms: 78 ms e 129 ms na tela de carregamento do Sobrevivência (abrir
o dispositivo de áudio e carregar o lote de sons) e 130 ms no começo de uma partida, quando `sv_music_play()` e
`sv_ambient_play()` carregam a música e o ambiente com `LoadMusicStream` no próprio quadro. Durante a partida, as
trocas de música e de ambiente ficaram abaixo de 60 ms.

**Esperado:** nenhum quadro acima de 60 ms ao começar a partida.
**Obtido:** 130 ms no quadro em que a partida começa; nos dois quadros do carregamento a barra de progresso fica
parada por um instante.

**Em aberto:** carregar a música e o ambiente da primeira fase durante a tela de carregamento, e dividir o lote de
sons em passos menores.

**Onde:** `code/src/survival/sv_audio.c` (`start_stream()`, `sv_audio_init_step()`)
**Coberto por:** nenhum teste; achado pelo log do jogo real
