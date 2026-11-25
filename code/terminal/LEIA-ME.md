# IARA no terminal

A versão do jogo que roda **dentro** do terminal: texto, sem janela nenhuma. É a entrega
da Unidade 2 de PIF.

Ela não é um jogo separado. Os dias, os enunciados e as fórmulas de lógica são os mesmos
do jogo gráfico, lidos dos mesmos arquivos. O que muda é só quem desenha: aqui é `printf`,
lá é raylib.

## Como compilar

No **MSYS2 UCRT64**, na pasta do projeto:

```bash
gcc -O2 -std=c11 code/terminal/*.c code/src/game/logic.c -Icode/terminal -Icode/src/game -Icode/src/core -Icode/src/screens -o iara-terminal.exe
```

Repare que **não tem `-lraylib`**: esta versão não depende do raylib. Só precisa do gcc.

## Como jogar

```bash
./iara-terminal.exe
```

O menu abre no próprio terminal. Responda com o número da opção e Enter.

Você é o estagiário do suporte da CISSA. A cada dia chega uma **mesa de triagem**: uma
lista de linhas e uma norma escrita em lógica. Decida cada linha pela norma e envie a mesa.
O jogo diz **quantas** linhas estão erradas, nunca **quais** — conferir é com você.

Os símbolos são os da disciplina: `¬` não, `∧` e, `∨` ou, `→` se…então, `↔` se e somente se.

A IARA, a inteligência artificial da empresa, dá um palpite quando você pede. Ela acerta
quase sempre. Quase.

## Os dias

| Dia | Nome | O que cobra | Mesa |
|---|---|---|---|
| 3 | A mesa do Tiago | conectivos e tabela-verdade | tabela-verdade |
| 12 | O fechamento do mês | De Morgan, dupla negação e tautologia | equivalência |
| 27 | Cavaleiros e escudeiros | consistência de um conjunto de afirmações | atribuição |
| 41 | O treino da mini-IA | quantificadores e viés de dados | treino |
| 58 | Os crachás | implicação e contrapositiva | atribuição |
| 74 | A auditoria | atribuição de valores que satisfaz as regras | atribuição |
| 89 | A lista do RH | silogismo categórico | silogismo |
| 100 | A prova | refutação por contradição | refutação |

Os dias **12**, **58** e **74** deixam uma evidência, e a prova do Dia 100 só fecha com
elas. Jogando pelo menu `Escolher um dia`, o Dia 100 recebe as três emprestadas, senão
não teria como ser jogado sozinho.

## De onde vem cada coisa

| Arquivo | O que faz |
|---|---|
| `jogo.c` | O menu, os dias e as telas. Tem a `main()` |
| `mesa.c` | Corrige as mesas: diz quantas linhas estão erradas |
| `entrada.c` | Lê o teclado e recusa o que não for resposta válida |
| `texto.c` | Monta as molduras e conta a largura do texto em caracteres |
| [`../src/screens/computer_days.h`](../src/screens/computer_days.h) | O roteiro dos 8 dias, compartilhado com o jogo gráfico |
| [`../src/screens/roteiro.h`](../src/screens/roteiro.h) | Os tipos do roteiro (`DialogStep`, `TriageCard`), sem raylib |
| [`../src/game/logic.c`](../src/game/logic.c) | O avaliador de lógica, compartilhado com o jogo gráfico |

**Nenhuma resposta certa está escrita no roteiro.** O gabarito sai sempre das fórmulas,
avaliadas por `logic.c`. É por isso que as duas versões do jogo cobram a mesma resposta
sem nenhuma tabela de respostas para manter em dia.

## O teste

[`../tests/test_terminal.c`](../tests/test_terminal.c) confere, à força bruta, que **toda
mesa tem solução**: para cada mesa ele experimenta todas as combinações de escolhas e
conta quantas o corretor aceita. Se alguma der zero, o dia seria impossível de fechar.

```bash
gcc -O2 -std=c11 code/tests/test_terminal.c code/terminal/mesa.c code/terminal/texto.c code/src/game/logic.c -Icode/terminal -Icode/src/game -Icode/src/core -Icode/src/screens -o test_terminal.exe && ./test_terminal.exe
```

A última execução: **8 mesas conferidas, 0 falhas**. Nas quatro mesas em que cada linha
tem a sua resposta, a solução é única; nas de conjunto, o Dia 100 aceita 3 provas
diferentes, porque mais de um conjunto de premissas fecha a contradição.

## Capítulos do livro-base

| Capítulo | Onde está no código |
|---|---|
| 4 · condicionais | `switch` do menu em `jogo.c`, `switch` por tipo de passo em `jogar_dia()`, `switch` por tipo de mesa em `mesa.c` |
| 5 · funções e modularização | quatro módulos com header próprio; todos os protótipos vêm antes da `main()` |
| 7 · vetores, matrizes e strings | `escolhas[]` da mesa, `lines[][]` do roteiro (matriz de strings), e `texto.c` inteiro, com `strncpy`, `strncat`, `strlen`, `memmove` e `memcpy` sempre limitados ao tamanho do destino |
| 8 · struct, typedef e enum | `Missao`, `Sessao`, `FimDeDia` e `OpcaoDoMenu` em `jogo.c`; `DialogStep` e `TriageCard` em `roteiro.h`; `LogicExpression` e `LogicOperator` em `logic.h` |
| 9 · ponteiros (bônus) | a árvore de fórmulas de `LogicExpression`, percorrida por ponteiro em `logic_evaluate()`; `const LogicExpression *const *clauses` nas mesas de atribuição; e todo parâmetro `Sessao *` que a função altera no original |
