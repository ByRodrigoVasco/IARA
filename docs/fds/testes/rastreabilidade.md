# Matriz de rastreabilidade

Relação entre os requisitos do GDD (seção 8.4) e o código, conferida em 03/11/2026 na branch
`test/campanha-100-dias`. O log da execução está em
[`execucao-2026-11-03.txt`](evidencias/execucao-2026-11-03.txt): **76 de 76 casos aprovados**.

**Situação:** ✅ implementado e testado · 🟡 parcial · ❌ não implementado.
A coluna de issue traz só o que foi confirmado no Jira; as demais ficam para a equipe preencher.
O trabalho desta branch ainda não foi commitado, então não há hash de commit para ele.

## Requisitos funcionais

| RF | Requisito | Implementação | Evidência | Issue | Situação | O que falta |
|---|---|---|---|---|---|---|
| RF01 | Save automático ao fechar | `core/save.c`, `main.c` | `test_save`, PKG-05, PKG-11, CT-75 | | ✅ | Retoma no dia em que parou. O andamento dentro do dia não é salvo: fechar no meio refaz os chamados daquele dia |
| RF02 | Rodar sem instalação nem conta | `tools/pacote/` | PKG-01 a PKG-03 | PI2-213 | 🟡 | Rodar o [roteiro em máquina limpa](maquina-limpa.md) |
| RF03 | Escolha de como resolver, chamado a chamado: sozinho, conferindo a IARA ou sem conferir | `screens/computer_loop.c`, `core/game_state.c` | CT-47 a CT-49, CT-55, CT-59 | | 🟡 | Vale para os 9 dias jogáveis. Muda a escolha única por tarefa da US03 e do GDD: a equipe precisa validar. Clique real ainda não verificado à mão |
| RF04 | Dica sem resposta pronta | `game/hints.c` | `test_hints`, CT-38, CT-39 | PI2-203, PI2-204 | 🟡 | Dica por NPC (PI2-202) |
| RF05 | Failsafe após 3 erros | `screens/computer_loop.c`, `screens/game_loop.c` | CT-50, CT-51, CT-56, CT-74 | | ✅ | Revisar as dicas com LMC (PI2-218) |
| RF06 | Flash do dia e salto puro | `game/calendar.c`, `screens/game_loop.c` | CT-45, CT-46, CT-64, CT-71 | | 🟡 | Falta a trilha sonora perder uma camada a cada bloco: o jogo ainda não tem áudio. O flash usa o escritório como imagem, sem arte própria |
| RF07 | Tutorial do Dia 1 e colinha | `screens/computer_loop.c`, `ui/cheat_sheet.c` | CT-17 a CT-20, CT-34, CT-52 a CT-54 | PI2-215 | 🟡 | A colinha depende de pegar o bilhete no mural antes do computador, ver [mapeamento](../../lmc/mapeamento-puzzles.md) |
| RF08 | Lógica ensinada pela narrativa | `screens/computer_loop.c`, `game/logic.c` | [Auditoria](../../lmc/mapeamento-puzzles.md), CT-61, CT-62 | PI2-214, PI2-215 | 🟡 | Vale só para o único puzzle existente |
| RF09 | Revisão de correção lógica | [Mapeamento](../../lmc/mapeamento-puzzles.md) | | PI2-217, PI2-218 | 🟡 | Revisão com o material de LMC (PI2-218) |
| RF10 | Teste automatizado por tarefa | `code/tests/` | Log da suíte, CT-66, CT-71 | | 🟡 | Cobre os 9 dias; os gabaritos são checados, não a regra em Haskell |
| RF11 | Sintomas visuais no lugar de HUD | Nenhum HUD numérico; o escritório escurece e alaga em `screens/game_loop.c` | Inspeção | | 🟡 | O cenário muda com o dia, ainda não com a pontuação (US10). Faltam olheiras e tremor na animação |
| RF12 | Pontuação por método | `resolve_day_tasks()` em `core/game_state.c` | CT-47 a CT-49, CT-51, CT-58, CT-63 | | 🟡 | Pontos por chamado: 10 sozinho ou conferindo, +5 por erro da IARA pego, 0 sem conferir, +5 por investigar. Adapta o GDD 7.3, que pontua por tarefa, e a equipe ainda decide o conflito com a US11. Acumula entre os dias |
| RF13 | Múltiplos finais | `get_game_ending()` em `core/game_state.c` | CT-70, CT-72 | | 🟡 | Os 4 finais seguem o GDD 6.5. O limiar do Vínculo é proposta, e a US12 descreve finais diferentes: a equipe precisa decidir |
| RF14 | Tensão progressiva | Cartões e flashes em `game/calendar.c`, roteiros em `screens/computer_days.h` | CT-71 | | 🟡 | Textos propostos, a validar com a equipe |
| RF15 | Interação opcional com NPCs | Mensagens opcionais nos dias 3, 27 e 58, em `screens/computer_days.h` | CT-72 | | 🟡 | As conversas acontecem pelo computador: ainda não há NPCs andando no mapa |

## Requisitos não funcionais

| RNF | Requisito | Implementação | Evidência | Situação | O que falta |
|---|---|---|---|---|---|
| RNF01 | Portabilidade em Windows | `tools/pacote/empacotar.ps1` | PKG-01, PKG-02 | 🟡 | Máquina limpa |
| RNF02 | Desempenho qualitativo | | | 🟡 | Avaliar na máquina do laboratório; não foi medido |
| RNF03 | Persistência de save | `core/save.c` | `test_save` | ✅ | |
| RNF04 | Executável autocontido | Zip com exe, DLLs e assets | PKG-01 | 🟡 | É uma pasta, não um arquivo único; confirmar se atende |
| RNF05 | Save ausente ou corrompido | `core/save.c` | `test_save`, PKG-07 a PKG-09 | ✅ | |
| RNF06 | Entrada inválida no puzzle | Quiz aceita só cliques nos botões | Inspeção | 🟡 | Sem teste específico; rever com puzzles de entrada livre |

## Verificação regra × código

O repositório ainda não tem as regras formalizadas por LMC. A verificação usa as regras escritas no GDD.

| Regra | Onde está | Como foi verificada | Resultado |
|---|---|---|---|
| Dia 1 exercita condicional e modus ponens | Mesa de triagem em `dialog_sequence`, com o gabarito calculado por `game/logic.c` | CT-61, CT-62 | Respeitada; falta revisão de LMC |
| Dica não reduz a pontuação | `toggle_hint_note()` | `test_hints`, CT-39 | Respeitada |
| Dica não entrega a resposta | Bilhete em `hints.c` | `test_hints` procura termos da resposta | Respeitada; a avaliação pedagógica é da equipe |
| Dica não vem da IA | Item no mural e dicas de erro do roteiro | Inspeção | Respeitada |
| Sem tela de teoria isolada | Telas do Dia 1 | [Auditoria](../../lmc/mapeamento-puzzles.md) | Respeitada |
| Manual = 10, automático = -5, failsafe = -10 (US11) | Nenhum | Busca no código | **Não implementada**: o jogo usa 10, 0 e 0 por chamado |
| Pontos nunca negativos (GDD 7.3/7.5) | CT-47 a CT-49, CT-51, CT-58 | Leitura do GDD | **Conflita com a US11**, que prevê -5 e -10; a equipe precisa decidir |
| 3 erros seguidos acionam o failsafe | `continue_after_answer()` | CT-50, CT-51, CT-56 | Respeitada |
| Finais determinísticos | Nenhum | Busca no código | **Não implementada** |

## Inconsistências encontradas

- **Nome da empresa:** a arte do escritório mostra "NEXO LABS", e o GDD e o quiz usam "CISSA".
- **Camada em Haskell:** as regras puras previstas no GDD não existem no repositório.
- **Depois do Dia 1:** o jogador volta ao mapa e encerra o dia pela porta (PI2-167, PI2-179, PI2-180), mas ainda não há um próximo dia: o jogo volta ao menu e INICIAR recomeça o Dia 1. O texto do cartão de fim do dia é provisório.
