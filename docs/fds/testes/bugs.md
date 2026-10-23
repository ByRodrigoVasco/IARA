# Bug tracker — bugs encontrados nos testes de sistema

Oito problemas encontrados na execução dos testes de sistema de 22/10/2026
(branch `develop`, commit `8b43f71`).

Cada bloco abaixo é o texto de uma issue. As oito já foram abertas no
[bug tracker do GitHub](https://github.com/ByRodrigoVasco/IARA/issues), e a
coluna "Issue" da tabela abaixo aponta para cada uma.

## Status

Atualizar esta tabela conforme as issues forem abertas e corrigidas.

| ID | Problema | Gravidade | Issue | Status |
|---|---|---|---|---|
| BUG-01 | Fade in para em alpha 250 | Baixa | [#32](https://github.com/ByRodrigoVasco/IARA/issues/32) | Aberto |
| BUG-02 | Voltar ao menu vaza memória | Alta | [#33](https://github.com/ByRodrigoVasco/IARA/issues/33) | Aberto |
| BUG-03 | Textura do jogador liberada duas vezes | Média | [#34](https://github.com/ByRodrigoVasco/IARA/issues/34) | Aberto |
| BUG-04 | Funções de unload nunca chamadas | Baixa | [#35](https://github.com/ByRodrigoVasco/IARA/issues/35) | Aberto |
| BUG-05 | Desenho fora do `BeginDrawing`/`EndDrawing` | Baixa | [#36](https://github.com/ByRodrigoVasco/IARA/issues/36) | Aberto |
| BUG-06 | Caminho de sprites em minúsculo | Média | [#37](https://github.com/ByRodrigoVasco/IARA/issues/37) | Aberto |
| BUG-07 | Pré-menu não descarrega imagens | Média | [#38](https://github.com/ByRodrigoVasco/IARA/issues/38) | Aberto |
| BUG-08 | Texto do quiz vaza para fora da caixa | Média | [#39](https://github.com/ByRodrigoVasco/IARA/issues/39) | Aberto |

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
