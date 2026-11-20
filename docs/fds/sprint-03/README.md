# Report da Sprint 03: testes de sistema e bug tracker

Evidências de qualidade da Sprint 03 (Week 15e16 - Prep Apresentação, 07/11 a 20/11/2026) de [FDS](../README.md).

| Recurso | Link |
|---|---|
| Board (Jira, projeto PI2) | [Backlog e sprints](https://csprj-adsr-2p-e8.atlassian.net/jira/software/c/projects/PI2/boards/2/backlog) |
| Bug tracker | [Issues no GitHub](https://github.com/ByRodrigoVasco/IARA/issues?q=is%3Aissue+label%3Abug) |
| Relatório completo da suíte | [Testes de sistema](../testes/README.md) e [bugs.md](../testes/bugs.md) |
| Report anterior | [Sprint 02](../sprint-02/README.md) |

## Testes de sistema das histórias restantes (PI2-298)

A Sprint 03 tem 18 histórias de produto. Doze já tinham caso de teste das sprints anteriores (CT-01 a CT-93). As que ainda não tinham ganharam os casos **CT-94 a CT-105**, no bloco 4 da suíte, [`code/tests/test_sprint03.c`](../../../code/tests/test_sprint03.c).

O bloco monta o jogo de verdade e o conduz quadro a quadro pela máquina de estados. O teclado passa por um gancho: o teste "segura" WASD para andar e "aperta" ESPAÇO para conversar, abrir o computador e sair pela porta, e todo o resto é o código do jogo. Os sons passam por um registro que anota cada arquivo tocado. É assim que os casos da US16 conferem o que o jogador ouviria.

- **Execução:** 20/11/2026, `develop` no commit `2386a1c`
- **Resultado:** **7 de 12 aprovados**. As 5 falhas viraram as issues #101 a #105 (BUG-39 a BUG-43)
- **Como executar:** `powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1` (a suíte inteira) ou `build/tests/test_sprint03 CT-95` (um caso só, como no screencast)

### Casos de teste

| CT | História | Pré-condição | Passos | Resultado esperado | Resultado obtido | Resultado | Screencast |
|---|---|---|---|---|---|---|---|
| CT-94 | US16 · [PI2-262](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-262) | Partida nova, Dia 1, jogador parado na entrada | 1. Segurar D por 1,5 s<br>2. Soltar e esperar 0,5 s | O som de passos toca durante a caminhada e para quando o jogador para | Andou 180 px; passos tocando em 90 de 90 quadros; parado, o som parou | ✅ Passou | [CT-94_US-16.mp4](screencasts/CT-94_US-16.mp4) |
| CT-95 | US16 · [PI2-263](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-263) | Dia 1 com os chamados já resolvidos | 1. Andar até a porta de vidro<br>2. Apertar ESPAÇO e acompanhar 1 s | Toca o som de porta (`PORTA ABRINDO.mp3`) | O dia termina pela porta, mas nenhum som toca; o arquivo nem é carregado | ❌ Falhou: [#101](https://github.com/ByRodrigoVasco/IARA/issues/101) | [CT-95_US-16.mp4](screencasts/CT-95_US-16.mp4) |
| CT-96 | US16 · [PI2-264](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-264) | Dia 1, jogador no escritório | 1. Ficar 3 s no escritório<br>2. Abrir o computador e ficar 2 s no quiz | `SOM AMBIENTE.mp3` toca no escritório e continua, mais baixo, no quiz | Ambiente com peso 1,00 no escritório e 0,20 por baixo do quiz | ✅ Passou | [CT-96_US-16.mp4](screencasts/CT-96_US-16.mp4) |
| CT-97 | US16 · [PI2-264](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-264) | Dia 1, chamados abertos | 1. Andar até o computador e apertar ESPAÇO<br>2. Responder os chamados do dia | Toca o som de teclado (`SOM DE TECLADO.ogg`) | Nenhum som no computador; o arquivo não é carregado | ❌ Falhou: [#102](https://github.com/ByRodrigoVasco/IARA/issues/102) | [CT-97_US-16.mp4](screencasts/CT-97_US-16.mp4) |
| CT-98 | US16 · [PI2-265](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-265) | Dia 1, jogador no escritório | 1. Andar até o Anderson<br>2. Apertar ESPAÇO e acompanhar 1 s da fala<br>3. Passar a conversa até o fim | Toca um voice blip enquanto a fala aparece | A conversa abre, mas a voz não toca em nenhum dos 60 quadros | ❌ Falhou: [#103](https://github.com/ByRodrigoVasco/IARA/issues/103) | [CT-98_US-16.mp4](screencasts/CT-98_US-16.mp4) |
| CT-99 | US17 · [PI2-269](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-269) | Assets em `code/assets/sprites/PERSONAGENS` | 1. Conferir `walk/<direção>-0..3.png` de cada NPC<br>2. Carregar os NPCs como o jogo carrega | Os 7 NPCs completos e `initialize_npcs()` sem erro | 7 de 7 NPCs com as 7 direções × 4 quadros; nenhum arquivo faltando | ✅ Passou | [CT-99_US-17.mp4](screencasts/CT-99_US-17.mp4) |
| CT-100 | US17 · [PI2-267](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-267), [PI2-268](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-268) | Partida no Dia 1 | 1. Abrir o Flash do Dia 2<br>2. Procurar arte de flash, salto e finais<br>3. Abrir o cartão do final | Imagens próprias para o Flash ou o salto e para os 4 finais | Flash = escritório escurecido com texto; salto e os 4 finais só texto; 0 imagens | ❌ Falhou: [#104](https://github.com/ByRodrigoVasco/IARA/issues/104) | [CT-100_US-17.mp4](screencasts/CT-100_US-17.mp4) |
| CT-101 | US10 · [PI2-236](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-236) | Dia 27, escritório à vista | 1. Fotografar com as estatísticas zeradas<br>2. Fotografar com 999 pontos e 9 em cada eixo | Nenhum número de status aparece | 0 pixels diferentes entre as duas fotos | ✅ Passou | [CT-101_US-10.mp4](screencasts/CT-101_US-10.mp4) |
| CT-102 | US10 · [PI2-237](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-237) | Dia 27, escritório à vista | 1. Fotografar com as estatísticas de quem confere tudo<br>2. Fotografar com as de quem só envia sem conferir<br>3. Comparar com o Dia 89 | O escritório fica diferente para os dois jogadores no mesmo dia | 0 pixels diferentes no mesmo dia (entre o Dia 27 e o 89, 413.679): o cenário muda com o dia, não com as escolhas | ❌ Falhou: [#105](https://github.com/ByRodrigoVasco/IARA/issues/105) | [CT-102_US-10.mp4](screencasts/CT-102_US-10.mp4) |
| CT-103 | US13 · [PI2-245](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-245) a [PI2-247](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-247) | Partida nova | 1. Abrir os 9 dias jogáveis em ordem<br>2. Conferir sons de susto | Dia 1 calmo, tensão e água nunca recuam, nenhum grito ou glitch | Tensão de 0,00 a 1,00 e água de 0,00 a 1,00 sem recuar; nenhum som de susto | ✅ Passou | [CT-103_US-13.mp4](screencasts/CT-103_US-13.mp4) |
| CT-104 | US04 · [PI2-202](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-202) | Cada dia jogável, jogador na entrada | 1. Andar até o NPC da dica do dia<br>2. Conversar até o fim | 9 de 9 dias com a dica no diálogo, pontos iguais e nenhuma dica da IARA | Dica ouvida em 9 de 9 dias, pontos iguais em 9 de 9, nunca da IARA | ✅ Passou | [CT-104_US-04.mp4](screencasts/CT-104_US-04.mp4) |
| CT-105 | US11 · [PI2-168](https://csprj-adsr-2p-e8.atlassian.net/browse/PI2-168) | Partida nova, 9 dias com 5 chamados | 1. Jogar os 9 dias com dica, sem dica e só com a IARA<br>2. Repetir trocando o método de um dia só | Com dica > sem dica > IARA, somados, e um dia isolado não muda o final | 496, 451 e 1 pontos (Finais C, A e B); trocando um dia, os finais continuam A e B | ✅ Passou | [CT-105_US-11.mp4](screencasts/CT-105_US-11.mp4) |

As outras histórias da sprint já tinham casos das sprints anteriores, e todos passaram de novo nesta execução: US03 (CT-77 e CT-78), US04b (CT-50, CT-51, CT-74), US05 (CT-45, CT-46, CT-64, CT-71), US06 (CT-52 a CT-54), US07 e US09 (CT-61 a CT-66), US12 (CT-70, CT-72), US14 (CT-79), US15 (CT-92, CT-93) e US20 (CT-80, CT-81). A US02 é coberta pelo teste do pacote (PKG-01 a PKG-11, `tools/pacote/testar_pacote.ps1`), que só roda no Windows e não foi repetido nesta execução.

### Screencasts (PI2-299)

Cada caso foi gravado sozinho: o terminal à esquerda mostra a pré-condição, os passos, o esperado, o obtido e o veredito; à direita, a janela do jogo conduzida pelo teste. Os vídeos são nomeados `CT-xx_US-yy.mp4` e ficam em [`screencasts/`](screencasts/).

A gravação foi feita em Linux (gcc, raylib, tela virtual Xvfb), com os mesmos arquivos e a mesma lista de fontes do `run_tests.ps1`. Na tela virtual não há placa de som: o áudio roda num dispositivo nulo, então o vídeo não tem som, e os casos da US16 mostram no terminal quais sons o jogo mandou tocar.

## Bug tracker (PI2-300 e PI2-301)

Conferido em **20/11/2026**: [Issues do GitHub](https://github.com/ByRodrigoVasco/IARA/issues?q=is%3Aissue+label%3Abug).

![Bug tracker do GitHub em 20/11/2026: BUG-39 a BUG-43 abertos pelos testes da Sprint 03, os BUG-33 a BUG-38 abertos e os anteriores fechados, todos com o label da sprint](bug-tracker-2026-11-20.png)

- **Abertas nesta sprint pelos testes:** BUG-39 a BUG-43 (#101 a #105), com passos para reproduzir, o caso de teste e a história do Jira.
- **Todas as 43 issues de bug** têm agora o label da sprint em que foram achadas (`sprint-01`, `sprint-02` ou `sprint-03`), um responsável e uma seção "Rastreabilidade" com a história do Jira, o teste e o PR e o commit da correção.
- **Abertas:** BUG-17 e BUG-18 (desempenho, gravidade baixa), BUG-33 a BUG-38 (abertas em 20/11 por Rodrigo, a maior parte em branches que ainda não entraram na `develop`) e BUG-39 a BUG-43.
- **Fechadas:** BUG-01 a BUG-16 e BUG-19 a BUG-32, cada uma citando o PR da correção (#47, #55, #61, #63, #76 ou #82).
