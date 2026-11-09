# Mapeamento de puzzles e auditoria de teoria

Levantamento feito em 02/11/2026 sobre o código da branch `feature/desktop-configuracoes-animacao`
e revisto em 06/11/2026 sobre a `develop` (`d736f64`), quando os puzzles dos dias 3 a 100 já
existiam. Cobre o PI2-217 (puzzle → conceito formal), o PI2-214 (auditoria de telas de teoria) e o
PI2-215 (conceito embutido na narrativa). A revisão com o material de LMC é o PI2-218 e
**ainda não foi feita**: nada aqui substitui a validação de quem cuida da lógica na equipe.

## Puzzles implementados

O Dia 1 traz os chamados de integração, em
[`computer_loop.c`](../../code/src/screens/computer_loop.c). Os puzzles dos outros 8 dias estão em [Puzzles dos dias](#puzzles-dos-dias).

| Passo | Conteúdo | Conceito | Tipo de conceito |
|---|---|---|---|
| 1 | Boas-vindas da CISSA | Nenhum | Narrativa |
| 2 | Tutorial das formas de resolver, na voz da IARA | Nenhum | Narrativa |
| 3 | Bruna: o que é IA | Inteligência artificial | Conceito de IA, não é lógica |
| 4 | Caio: o que é treinar um modelo | Treinamento | Conceito de IA, não é lógica |
| 5 | Júlia: o que é viés | Viés de dados | Conceito de IA, não é lógica |
| 6 | Diego: dados pessoais e privacidade | Privacidade e tarja de dados | Conceito de IA e ética, não é lógica |
| 7 | Tiago: mesa de triagem com a norma da CISSA | Condicional (`P -> Q`) e modus ponens | **Lógica proposicional** |
| 8 | Relatório "Assinado por você" | Responsabilidade pelo que foi enviado | Consequência das escolhas |

Forma do passo 7: `U -> TI` (se é urgente, vai para o TI). Puzzles com símbolos lógicos não usam múltipla
escolha: o jogador aplica a norma a cada chamado da mesa. O gabarito é calculado pelo avaliador de
[`logic.c`](../../code/src/game/logic.c): o TI é obrigatório só quando negá-lo viola a norma.

| Chamado | U | Destino | Erro que revela |
|---|---|---|---|
| #07 servidor fora do ar | Sim | TI | Não aplica modus ponens |
| #08 trocar o mouse | Não | Fila | Trata a condicional como se valesse sem a condição |
| #09 invasão na rede | Sim | TI | Não aplica modus ponens |
| #10 "falem com o TI!" | Não | Fila | Troca a premissa da norma pelo pedido no texto |

A frase "o que a norma não manda ao TI fica na fila" é uma regra à parte: `U -> TI` sozinha nada diz quando U é falso.
Vale confirmar essa redação na revisão. Ao enviar, o jogo diz quantos destinos estão errados, nunca quais.

## Sozinho, com a IARA ou sem conferir

Em cada chamado o jogador decide, sem tela de escolha: a IARA é um botão opcional.

| Forma | Como acontece | Pontos | Automação |
|---|---|---|---|
| Sozinho | Responde direto, podendo usar o manual e a colinha | 10 | Não soma |
| Com a IARA, conferindo | Lê a sugestão, marca o que confere com o trecho do manual e decide | 10, mais 5 se apontar o erro dela | Não soma |
| Sem conferir | Envia a sugestão como veio | 0, e pode sair errado | Soma 1 no dia, se for a maioria dos chamados |

A ideia ética é que usar IA com cuidado vale o mesmo que resolver sozinho. Com memória cheia, a IARA acerta os passos 3, 4 e 5
e erra os passos 6 e 7, sempre com muita confiança. O jogo não dá bronca na hora: o
relatório do fim do dia mostra o que aconteceu com cada chamado. O chamado 6 ainda pergunta se os CPFs vão com tarja à
IARA, e isso aparece no relatório sem mudar os pontos.

A chefia cobra pela barra de tarefas quando um chamado demora, e o botão de enviar sem conferir cresce a cada uso.
Nenhum cronômetro faz o jogador perder um chamado.

### Memória da IARA

Cada uso gasta a memória dela (6 no total): conferir gasta 1, enviar sem conferir gasta 2 e resolver sozinho devolve 1.
A confiança declarada sobe conforme ela piora, porque IA não avisa quando perdeu o contexto.

| Memória | Comportamento | Efeito para o jogador |
|---|---|---|
| Cheia (5 ou 6) | Sugestão direta, com a página do manual | Conferência direta |
| Média (3 ou 4) | Confunde quem pediu o chamado e não cita a página | Uma afirmação falsa a mais, e ele procura a página |
| Baixa (0 a 2) | Responde errado e inventa normas | A sugestão erra, com a confiança mais alta do dia |

O botão **Nova conversa** recupera tudo, mas o chamado precisa ser colado de novo e os dados pessoais voltam a passar pela
tarja; o que já foi enviado sem tarja não volta atrás. Com pouca memória ela cita a Mariana, personagem do GDD que o
jogador ainda não conheceu: é uma proposta de pista para a narrativa, a validar com a equipe.

## Puzzles dos dias

Conceitos da seção 6.3 do GDD. Nenhum puzzle com símbolos usa múltipla escolha: o jogador decide cada linha de uma mesa, e o
jogo diz quanto está errado, nunca onde. Todo gabarito é calculado por [`logic.c`](../../code/src/game/logic.c), e os testes
provam que cada mesa aceita uma só marcação. Os enunciados ficam em [`computer_days.h`](../../code/src/screens/computer_days.h).

| Dia | Puzzle | Conceito formal | Como o jogador resolve | A IARA com memória cheia |
|---|---|---|---|---|
| 1 | Chamados + mesa de triagem | Condicional simples, modus ponens | Aplica `U → TI` a cada chamado | Erra: cai na isca do pedido |
| 3 | Triagem de chamados de TI | Conectivos, tabela-verdade | Calcula `(P ∧ ¬B) ∨ I` em 5 linhas | Acerta |
| 12 | Conciliação + decisão | De Morgan, dupla negação, tautologia | Marca se cada par é igual ou difere | Erra: confunde `¬(P ∨ Q)` com `¬P ∨ ¬Q` |
| 27 | Transcrição da ata | Cavaleiros e Escudeiros | Marca quem mente; a ata só fecha sem contradição | Acerta |
| 41 | Arquivo morto + treino | Quantificadores (`∀`, `∃`) | Monta o conjunto de treino e vê a mini-IA mudar | Erra: treina só com o Centro |
| 58 | Restaurar crachás | Modus ponens e tollens, silogismo hipotético | Encadeia as regras e ativa ou nega cada crachá | Erra: alucina a volta do Diego |
| 74 | Auditoria de logs | Satisfazibilidade (SAT) | Acha os valores que satisfazem as 4 cláusulas | Erra: insiste em um humano |
| 89 | Preparar a sala | Silogismo categórico, Venn | Julga se cada conclusão segue das premissas | Erra: inverte "todo A é B" |
| 100 | Confronto final | Prova por contradição | Escolhe o menor conjunto contraditório, com uma evidência sua | Nega a contradição |

Sem memória, a IARA erra todos. O Dia 89 usa a leitura moderna: "Todo A é B" não garante que exista algum A.
No Dia 100 há 3 provas aceitas: `D` com `¬D`, `H` com `¬H`, ou `D`, `D → C` e `¬C`. Cada evidência vem de uma investigação:

| Evidência | Como conseguir |
|---|---|
| `¬C`: as rescisões não têm carta | Pedir a revisão humana no Dia 12 |
| `¬D`: o crachá do Diego abriu depois | Examinar o mural no Dia 58 |
| `¬H`: às 03:12 não havia humano | Examinar o mural no Dia 74 |

## Calendário e finais

- **Dias:** os 9 jogáveis são os do GDD. A escolha de quais outros dias são flash é proposta: 2, 7, 16, 22, 33, 38, 47, 52, 63, 68, 79, 84, 93 e 97. O resto é salto puro.
- **Vínculo Humano:** cresce nas mensagens opcionais dos dias 3, 27 e 58.
- **Finais:** seguem a seção 6.5. Automação alta é 5 ou mais, Percepção alta é 4 ou mais. O GDD não dá o limiar do Vínculo, então a proposta é 2 das 3 conversas.
- **Mariana:** o jogador a encontra ao examinar o mural no Dia 89, que leva a O Fundo.
- **Memória da IARA:** cada dia abre uma conversa nova, mas cada dia delegado tira 1 da memória inicial, até o limite de 3.

## Auditoria de telas de teoria

### Primeira passagem, 02/11/2026: o Dia 1

**Resultado: nenhuma tela de teoria isolada.** Foram revisados o roteiro inteiro dos chamados, o
cartão do dia e o escritório.

| Tela | Explica conceito lógico fora da tarefa? | Observação |
|---|---|---|
| Cartão do dia | Não | Só narrativa |
| Escritório | Não | O bilhete do mural é opcional e faz parte do cenário |
| Passo 1 do roteiro | Não | Apresenta a empresa, não ensina conteúdo |
| Passo 2 | Não | Explica as formas de resolver, sem conteúdo de lógica |
| Passos 3 a 6 | Não | São chamados diretos, sem texto explicativo antes; o trecho do manual só aparece a quem confere a IARA |
| Passo 7 | Não | A norma vem dentro da própria mesa de triagem |
| Passo 8 | Não | Relatório das consequências, sem conteúdo de lógica |
| Colinha | Não | Post-it de consulta, colado no monitor depois de pegar o bilhete do mural |

Antes desta revisão, o Dia 1 não tinha nenhum conteúdo de lógica, embora o GDD peça condicional
e modus ponens. O passo 7 corrige isso. O bilhete do mural é a dica por exploração da US04.

### Segunda passagem, 06/11/2026: os 8 dias restantes

A primeira passagem foi escrita antes de os puzzles dos dias 3 a 100 existirem: eles entraram em
[`computer_days.h`](../../code/src/screens/computer_days.h) no mesmo dia, pelo `b8063ba`. Esta
passagem cobre esses 8 dias, os enunciados das mesas, as dicas de erro e os bilhetes do mural.

**Nenhuma tela de teoria isolada também aqui.** Nenhum dia abre com uma tela de conceito antes do
puzzle: todos vão do recado da IARA direto para a mesa. O que apareceu foi mais fino — pedaços de
explicação em voz de apostila *dentro* de documentos que deveriam ser de alguém.

| Dia | Achado | O que foi feito |
|---|---|---|
| 3 | `"V é verdadeiro, F é falso."` era legenda de notação na voz do jogo, e o enunciado era o único sem remetente | O enunciado virou o bilhete que o Tiago deixou no monitor, com a norma e a legenda na fala dele |
| 12 | `"Compare cada par na tabela-verdade."` instruía o método; a Sônia, do Financeiro, não falaria assim | A pergunta passou a ser dela: "cada par abaixo continua dizendo a mesma coisa?" |
| 12 | Dica `"Negar um ∨ pede o ∧ do outro lado."` enunciava De Morgan como regra de livro | Virou uma linha a testar no par B, sem nomear a lei |
| 41 | Enunciado eram três linhas de `∀`/`∃` sem remetente nem documento, único dia sem moldura | Passou a ser a ficha do treino, com as mesmas três exigências |
| 41 | Dica `"Um ∀ cai com um só contraexemplo."` era o princípio abstrato | Virou "um registro com CPF já quebra a exigência" |
| 89 | Dica `"Todo A é B não diz que todo B é A."` usava `A` e `B` que não existem no puzzle | Passou a falar dos próprios conjuntos: convidado e funcionário |
| 100 | `"Use só o que a contradição precisa…"` era regra de exercício, embora o Renê estivesse falando na tela anterior | O desafio continua na fala dele, entre aspas |

Ficaram como estavam, por já nascerem de alguém ou de um documento:

| Item | Por quê |
|---|---|
| Bilhete do mural do Dia 1 | Explica `P -> Q`, mas assinado "Estagiário do ano passado" e pregado no mural |
| Colinha | Objeto do cenário, pego junto com o bilhete; é legenda de símbolos, não aula |
| Dias 27, 58, 74, 89 | Enunciados já vêm de "Ata da diretoria", "Portaria", "Auditoria" e "Lista da sala" |
| Dia 1, passo 7 | A norma já vinha no e-mail do Tiago |
| Bilhetes dos dias 12, 41, 58, 74 e 89 | São pistas da narrativa, sem conteúdo de lógica |
| Dicas dos dias 27, 58, 74 e 100 | Falam das variáveis do próprio puzzle, não de letras genéricas |

**Limite desta passagem:** o critério foi *de quem é a voz*, não se o conceito está correto. A
revisão de conteúdo com LMC (PI2-218) continua pendente, e os textos novos entram nela.

## Pontos para a equipe

- **Revisão pendente (PI2-218):** as mesas de todos os dias, as dicas de cada erro, os trechos do manual, as afirmações da IARA e os textos da colinha são uma proposta. A US09 pede essa revisão antes de publicar.
- **Narrativa proposta:** o GDD ainda não preencheu os personagens (2.5). Renê, Sônia, Vera, Nuno, Lia e os textos de cartões, flashes, bilhetes e finais foram escritos para esta versão, e a equipe precisa validar.
- **Notação:** a fonte VT323 não tem `→`, `↔`, `∧`, `∨`, `¬`, `∀` nem `∃`, por isso o jogo os desenha em traços, no texto e na colinha ([`logic_text.c`](../../code/src/ui/logic_text.c)).
- **Colinha:** é pega junto com o bilhete do mural e fica sempre à vista no canto inferior esquerdo do monitor, em qualquer forma de resolver. Isso substitui a tecla prevista no GDD (P03), e quem vai direto ao computador faz o Dia 1 sem ela. A partir do Dia 3 ela já vem no monitor, porque esses puzzles dependem dos símbolos.
- **Passos 3 a 6:** são conteúdo de IA, sem conceito lógico. O GDD prevê isso no quiz de integração, então foram mantidos.
- **Mudança em relação ao GDD:** a memória da IARA é nova, e a escolha Manual ou Automático deixou de ser única por tarefa (GDD 7.3 e US03) e passou a ser por chamado, com uma terceira forma, a de conferir a IARA. A equipe precisa validar e refletir isso no GDD.
- **Áudio:** o jogo não tem som, então a trilha que perde camadas a cada salto (RF06) não foi feita.
- **Tableaux:** o GDD ainda pede confirmar com o professor se removê-los atende à ementa de LMC.
- **Regras formalizadas:** não há formalização de LMC versionada no repositório, o que limita a verificação regra × código do PI2-32.
