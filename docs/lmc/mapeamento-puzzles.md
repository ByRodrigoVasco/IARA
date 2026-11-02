# Mapeamento de puzzles e auditoria de teoria

Levantamento feito em 02/11/2026 sobre o código da branch `feature/animacao-todas-direcoes`.
Cobre o PI2-217 (puzzle → conceito formal), o PI2-214 (auditoria de telas de teoria) e o
PI2-215 (conceito embutido na narrativa). A revisão com o material de LMC é o PI2-218 e
**ainda não foi feita**: nada aqui substitui a validação de quem cuida da lógica na equipe.

## Puzzles implementados

O jogo tem um único conteúdo jogável: o quiz de integração do Dia 1, em
[`computer_loop.c`](../../code/src/screens/computer_loop.c). Os outros 8 puzzles do GDD ainda não existem.

| Passo | Conteúdo | Conceito | Tipo de conceito |
|---|---|---|---|
| 1 | Boas-vindas da CISSA | Nenhum | Narrativa |
| 2 | O que é IA | Inteligência artificial | Conceito de IA, não é lógica |
| 3 | O que é treinar um modelo | Treinamento | Conceito de IA, não é lógica |
| 4 | O que é viés | Viés de dados | Conceito de IA, não é lógica |
| 5 | Dados pessoais e privacidade | Privacidade | Conceito de IA, não é lógica |
| 6 | Primeiro chamado e norma da CISSA | Condicional (`P -> Q`) e modus ponens | **Lógica proposicional** |
| 7 | Encerramento do dia | Nenhum | Narrativa |

Forma do passo 6: `P -> Q` (se é urgente, vai para o TI) e `P` (está marcado como urgente), logo `Q`.

| Alternativa | Papel | Erro que revela |
|---|---|---|
| A) Encaminho para o TI | Correta | Nenhum: aplica modus ponens |
| B) Nada: a norma não diz o que fazer | Distratora | Não reconhece que a condicional se aplica |
| C) Arquivo: só o TI decide o que é urgente | Distratora | Troca a premissa dada por uma suposição externa |

## Puzzles previstos

Conceitos da seção 6.3 do GDD, ainda sem código.

| Dia | Puzzle | Conceito formal | Situação |
|---|---|---|---|
| 1 | Quiz de integração + notação lógica | Condicional simples, modus ponens | Implementado, falta a colinha |
| 3 | Triagem de chamados de TI | Conectivos, tabela-verdade | Não implementado |
| 12 | Conciliação financeira + decisão | De Morgan, dupla negação, tautologia | Não implementado |
| 27 | Transcrição da ata | Cavaleiros e Escudeiros: dedução por consistência | Não implementado |
| 41 | Arquivo morto + treino da IARA | Quantificadores, grade lógica | Não implementado |
| 58 | Restaurar crachás | Modus ponens/tollens, silogismo hipotético | Não implementado |
| 74 | Auditoria de logs da IARA | Satisfazibilidade (SAT) | Não implementado |
| 89 | Preparar a sala | Silogismo categórico, diagrama de Venn | Não implementado |
| 100 | Confronto final | Prova por contradição | Não implementado |

## Auditoria de telas de teoria

**Resultado: nenhuma tela de teoria isolada.** Foram revisados o roteiro inteiro do quiz, o
cartão do dia e o escritório.

| Tela | Explica conceito lógico fora da tarefa? | Observação |
|---|---|---|
| Cartão do dia | Não | Só narrativa |
| Escritório | Não | O bilhete do mural é opcional e faz parte do cenário |
| Passo 1 do quiz | Não | Apresenta a empresa, não ensina conteúdo |
| Passos 2 a 5 | Não | São perguntas diretas, sem texto explicativo antes |
| Passo 6 | Não | A regra vem dentro do próprio chamado |

Antes desta revisão, o Dia 1 não tinha nenhum conteúdo de lógica, embora o GDD peça condicional
e modus ponens. O passo 6 corrige isso. O bilhete do mural é a dica por exploração da US04.

## Pontos para a equipe

- **Revisão pendente (PI2-218):** o enunciado e as distratoras do passo 6 são uma proposta.
- **Notação:** a fonte VT323 não tem `→`, `↔`, `∧`, `∨` nem `¬`, por isso o jogo usa `P -> Q`. A colinha do RF07 vai precisar de outra fonte ou de ícones.
- **Colinha:** o post-it persistente com os símbolos (US06) não existe. O bilhete do mural é uma dica de um único puzzle, não a colinha.
- **Passos 2 a 5:** são conteúdo de IA, sem conceito lógico. O GDD prevê isso no quiz de integração, então foram mantidos.
- **Tableaux:** o GDD ainda pede confirmar com o professor se removê-los atende à ementa de LMC.
- **Regras formalizadas:** não há formalização de LMC versionada no repositório, o que limita a verificação regra × código do PI2-32.
