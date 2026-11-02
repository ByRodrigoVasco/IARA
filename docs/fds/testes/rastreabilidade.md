# Matriz de rastreabilidade

Relação entre os requisitos do GDD (seção 8.4) e o código, conferida em 02/11/2026 na branch
`feature/animacao-todas-direcoes`. O log da execução está em
[`execucao-2026-11-02.txt`](evidencias/execucao-2026-11-02.txt): **41 de 42 casos aprovados**,
com a falha CT-15 registrada como bug. O CT-24 passou nesta execução, mas oscila entre execuções, e o BUG-02 segue aberto.

**Situação:** ✅ implementado e testado · 🟡 parcial · ❌ não implementado.
A coluna de issue traz só o que foi confirmado no Jira; as demais ficam para a equipe preencher.
O trabalho desta branch ainda não foi commitado, então não há hash de commit para ele.

## Requisitos funcionais

| RF | Requisito | Implementação | Evidência | Issue | Situação | O que falta |
|---|---|---|---|---|---|---|
| RF01 | Save automático ao fechar | `core/save.c`, `main.c` | `test_save`, PKG-05, PKG-11 | | ✅ | Nenhuma tela avança o dia ainda, então o save sempre guarda o Dia 1 |
| RF02 | Rodar sem instalação nem conta | `tools/pacote/` | PKG-01 a PKG-03 | PI2-213 | 🟡 | Rodar o [roteiro em máquina limpa](maquina-limpa.md) |
| RF03 | Escolha de modo manual ou automático | Só o campo `mode_type` | | | ❌ | Tela de escolha e modo automático |
| RF04 | Dica sem resposta pronta | `game/hints.c` | `test_hints`, CT-38, CT-39 | PI2-203, PI2-204 | 🟡 | Dica por NPC (PI2-202) |
| RF05 | Failsafe após 3 erros | | | | ❌ | Tudo; o quiz hoje não pede nova tentativa |
| RF06 | Flash do dia e salto puro | Marcadores em `state_machine.c` | | | ❌ | As duas telas |
| RF07 | Tutorial do Dia 1 e colinha | `screens/computer_loop.c` | CT-17 a CT-20, CT-34 | PI2-215 | 🟡 | Colinha visual, ver [mapeamento](../../lmc/mapeamento-puzzles.md) |
| RF08 | Lógica ensinada pela narrativa | `screens/computer_loop.c` | [Auditoria](../../lmc/mapeamento-puzzles.md) | PI2-214, PI2-215 | 🟡 | Vale só para o único puzzle existente |
| RF09 | Revisão de correção lógica | [Mapeamento](../../lmc/mapeamento-puzzles.md) | | PI2-217, PI2-218 | 🟡 | Revisão com o material de LMC (PI2-218) |
| RF10 | Teste automatizado por tarefa | `code/tests/` | Log da suíte | | 🟡 | Cobre só o Dia 1; os gabaritos são checados, não a regra em Haskell |
| RF11 | Sintomas visuais no lugar de HUD | Nenhum HUD numérico de estatísticas | Inspeção | | 🟡 | Sintomas visuais no cenário |
| RF12 | Pontuação por método | Só a `hidden_statistic` | | | ❌ | Nenhuma tela altera os pontos |
| RF13 | Múltiplos finais | | | | ❌ | Tudo |
| RF14 | Tensão progressiva | Só o texto do Dia 1 | | | ❌ | Depende dos demais dias |
| RF15 | Interação opcional com NPCs | Só sprites e frames | CT-06 | | ❌ | Diálogos e NPCs no mapa |

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
| Dia 1 exercita condicional e modus ponens | Passo 6 de `dialog_sequence` | Leitura do enunciado e do gabarito | Respeitada; falta revisão de LMC |
| Dica não reduz a pontuação | `toggle_hint_note()` | `test_hints`, CT-39 | Respeitada |
| Dica não entrega a resposta | Bilhete em `hints.c` | `test_hints` procura termos da resposta | Respeitada; a avaliação pedagógica é da equipe |
| Dica não vem da IA | Item no mural | Inspeção | Respeitada |
| Sem tela de teoria isolada | Telas do Dia 1 | [Auditoria](../../lmc/mapeamento-puzzles.md) | Respeitada |
| Manual = 10, automático = -5, failsafe = -10 | Nenhum | Busca no código | **Não implementada** |
| Pontos nunca negativos (GDD 7.3/7.5) | Nenhum | Leitura do GDD | **Conflita com a US11**, que prevê -5 e -10; a equipe precisa decidir |
| 3 erros acionam o failsafe | Nenhum | Busca no código | **Não implementada** |
| Finais determinísticos | Nenhum | Busca no código | **Não implementada** |

## Inconsistências encontradas

- **Nome da empresa:** a arte do escritório mostra "NEXO LABS", e o GDD e o quiz usam "CISSA".
- **Camada em Haskell:** as regras puras previstas no GDD não existem no repositório.
- **Saída do quiz:** depois de concluir, o jogador não volta ao escritório (PI2-167, PI2-179, PI2-180).
