# IARA — Roteiro de encenação e plano de cutscenes

**Proposta narrativa | 18 de novembro de 2026**  
**Alvo:** modo história de `Updated/IARA`  
**Estado:** roteiro para revisão; cenas, gestos e adaptações ainda não implementados.

Este documento complementa [a especificação de NPCs, cutscenes e tarefas](especificacao-npcs-cutscenes-historia.md). A especificação descreve arquitetura e requisitos; este roteiro define a progressão dramática, a presença proposta dos personagens e a encenação. Em caso de divergência, registrar e decidir a alteração nos dois documentos: nenhuma proposta aqui altera automaticamente o código, os puzzles ou os finais.

## 1. Primeira leva: três cenas

Implementar primeiro **três cutscenes do Dia 1**, identificadas como 01A, 01M e 01F. Elas cobrem chegada, uma interrupção entre chamados e repercussão, inclusive falha antecipada. São suficientes para testar o ciclo completo de uma jornada sem produzir 27 cenas antes de conhecer os limites do mapa, da movimentação e do computador.

Esta é uma entrega vertical, não só três diálogos: inclui a correção mínima da movimentação dos atores envolvidos, integração com as tarefas, retomada, gravação segura e testes necessários para as cenas funcionarem. Os sete NPCs continuam disponíveis conforme o jogo atual; a rotina móvel completa de todos eles pode vir em lotes posteriores. Não simular ações com recursos inexistentes: usar movimento e orientação quando forem legíveis, ou uma encenação estática assumida como provisória.

**Fora da primeira leva:** os outros oito dias encenados, novas salas, finais adicionais, nova caixa de diálogo, retratos e poses dedicadas. A meta posterior continua sendo três cenas por dia jogável, sujeita a avaliação do ritmo; flashes e saltos não passam a exigir cenas.

## 2. Intenção narrativa e continuidade

O jogador chega à CISSA para trabalhar com a IARA. Ela é útil e acessível; a responsabilidade pelo que se envia continua humana. A empresa parece organizada, mas as pessoas sentem o peso de uma rotina cada vez mais automatizada. O horror cresce por lacunas em registros, pessoas ausentes e água onde não deveria estar. As cenas devem mostrar esse desgaste em ações e reações, sem antecipar explicações finais.

O código atual define nove dias jogáveis: 1, 3, 12, 27, 41, 58, 74, 89 e 100. Os dias intermediários usam flashes e saltos. O quadro a seguir é uma **proposta de continuidade**, não um novo cânone confirmado. A coluna de presença descreve o que as cenas planejadas precisam; não declara que os demais personagens desapareceram. O jogo atual mantém os sete NPCs no escritório em todos esses dias, portanto qualquer ausência física exige implementação e revisão de roteiro.

| Dia | Situação já representada na Updated | Foco e presença sugeridos para as cenas | A / M / F — intenção futura |
|---|---|---|---|
| **1** | Integração; chamados de IA, treino, viés, privacidade e triagem. | Laura acolhe; JV acompanha a conferência; Vasco cobra o encerramento. | A: chegada e mesa. M: reação ao primeiro chamado ou à tomada pela IARA. F: responsabilidade pelo relatório. |
| **3** | Fila de Tiago, norma de conectivos e convite de Júlia. | Vasco e Zito organizam a fila; Tiago é citado, não aparece como NPC. | A: fila sem responsável. M: norma precisa ser lida. F: consequência do envio e sinal de ausência. Preservar a escolha de almoço e seu efeito existente. |
| **12** | Conciliação de Sônia; escolha entre assinar e pedir revisão. | Anderson observa o fechamento; Laura percebe falta de alguém. Sônia continua remetente. | A: pressa do fechamento. M: duas formulações. F: reação específica à escolha; revisão pode gerar evidência de rescisão sem carta. |
| **27** | Ata da diretoria e gravação com água. | Anderson lida com a ata; Nicollas nota um sinal do prédio. | A: gravação. M: interrupção e silêncio. F: marca de água. Não revelar uma explicação sobrenatural completa. |
| **41** | Seleção de dados para treinar a IARA; arquivo úmido. | JV confronta o conjunto; Nicollas cuida do arquivo; Laura percebe ausências. | A: material para treino. M: qualidade e privacidade dos registros. F: efeito da seleção efetivamente feita. |
| **58** | Falha de crachás; registro de Diego; mural pode fornecer evidência. | Zito investiga; Laura reconhece nomes. Diego é referência de registro. | A: falha. M: nome inesperado. F: manter acessível a investigação opcional; não conceder evidência na cena. |
| **74** | Auditoria, logs e bilhete sobre acesso sem autor humano. | Julio e Zito examinam registros; Vasco lida com ausência de chefia. | A: auditoria. M: lacuna de autoria. F: apontar mural sem forçar leitura. |
| **89** | Lista para o Dia 100; mural/mapa e crachá de Mariana. | Laura e Vasco reagem à lista; Nicollas indica a pista. | A: convidados. M: implicações da lista. F: mural permanece opcional; no código, examiná-lo ativa `found_mariana`, sem comprovar área física do Fundo. |
| **100** | Renê propõe prova por contradição; o resultado e escolhas acumuladas determinam o final. | Renê está no texto do computador; outros atores e sua presença física dependem de produção. | A: preparar confronto. M: tensionar premissas sem dar gabarito. F: repercussão da prova, seguida do final já calculado. |

### Presença e arcos: proposta para revisão

- **Laura:** referência humana da chegada; percebe cadeiras vazias e nomes antes de ter uma explicação. Nos dias finais, sua disponibilidade precisa ser definida antes de criar uma cena que a exija.
- **JV:** começa confiante na ferramenta e passa a querer testar sua origem e seus resultados. A cautela não implica que toda falha de IA seja causada apenas pelos dados.
- **Anderson:** analista responsável que reconhece o custo de aceitar registros sem conferência. Seu arco não o transforma automaticamente em funcionário do Financeiro.
- **Julio:** desenvolvedor inclinado a resolver rápido; sua competência permanece visível quando os registros começam a falhar.
- **Nicollas:** conhece o prédio e reconhece sinais materiais. Suas observações não equivalem a conhecer toda a história do Fundo.
- **Vasco:** organiza prazos e prioridades; a pressão do trabalho pode ceder espaço à preocupação com as pessoas.
- **Zito:** suporte técnico, fala enquanto verifica; a assistência não substitui o raciocínio do jogador.
- **IARA:** começa prestativa, depois demonstra conhecimento ou iniciativa inquietante. Só pode se referir a ações ou dados que o estado da partida realmente sustente.
- **Ricardo, Mariana e Renê:** preservar as menções existentes; suas relações e eventual presença em cena precisam de definição narrativa. Não presumir que Ricardo e Renê são a mesma pessoa.

Cada dia futuro precisa de uma matriz aprovada de **presente / ausente / apenas mencionado** antes da implementação. Primeiro reconciliar essa matriz com cartões, diálogos opcionais, dicas e tarefas; depois configurar a presença física. Não retirar o único NPC que oferece uma dica sem prever substituto opcional.

## 3. Regras de escrita e encenação

Cada cena deve ter intenção, gatilho verificável, ação no cenário, diálogo curto, saída e consequência explícita. Cada fala realiza uma ação dramática; a meta é até duas frases e 22 palavras, validada na tela. Sempre que viável, incluir duas ações físicas distintas. A fala não explica uma escolha que ainda não ocorreu nem oferece a solução de um puzzle.

O jogador avança a cena; escolhas continuam na exploração ou nos formulários das tarefas. A entrada que iniciou uma cena não avança sua primeira fala. Pausa suspende encenação e tarefa. A cena não passa sozinha pelo texto. O objetivo posterior deve indicar uma ação concreta, sem exigir conversa ou bilhete opcionais.

**Marcos espaciais usados abaixo são nomes de produção**, não coordenadas validadas: `entrada`, `mesa_jogador`, `aproximacao_mesa`, `posto_jv`, `posto_laura`, `quadro_vasco` e `saida`. Antes de implementar, conferir se esses pontos existem e são alcançáveis no mapa atual, reservar espaço para o jogador e ajustar o percurso sem atravessar obstáculos. A câmera atual e as camadas do computador precisam ser testadas; não prometer enquadramento móvel ou objeto manipulável sem implementação.

## 4. Dia 1 — contexto jogável

### Situação e alterações necessárias

Atualmente, o cartão de chegada diz que ninguém recebe o jogador. Isso contradiz 01A. Proposta de substituição, mantendo a mesa afastada:

> “Sua mesa fica na salinha ao lado do corredor, um pouco afastada do resto.”  
> “O computador já está ligado. Alguém espera perto da entrada.”

A IARA já apresenta os chamados e as três formas de resolução no computador; manter esse tutorial. O roteiro atual traz, nesta ordem, Bruna (definição de IA), Caio (treinamento), Júlia (viés), Diego (privacidade) e Tiago (triagem). A cena 01M ocorre após o resultado do chamado de **Bruna**, antes de liberar Caio; não descreve uma sugestão da IARA caso o jogador tenha respondido sozinho.

Há três erros consecutivos até a IARA assumir o chamado atual e os restantes. Se isso ocorrer no primeiro chamado, 01M deve ser executada em uma variante própria após o relatório e a saída controlada do computador, antes de 01F. 01F continua obrigatória antes do encerramento do dia. Não apresentar Caio como pendente após o failsafe.

### 01A — Uma mesa afastada

**Objetivo dramático:** apresentar Laura como atenta às pessoas e JV como colega responsável pelos modelos, sem eliminar a sensação de isolamento da mesa.  
**Gatilho:** após o cartão do Dia 1, antes de liberar movimento; uma vez por partida.  
**Participantes:** jogador, Laura e JV.  
**Ponto de partida proposto:** jogador em `entrada`; Laura em posição segura próxima; JV junto ao seu posto. Marcas e alcance são validados no mapa.  
**Objetivo ao sair:** “Vá até seu computador”.

| Ordem | Ação e diálogo | Condição de avanço |
|---|---|---|
| 1 | Laura vira para o jogador e dá um passo para abrir caminho. **Laura:** “Você deve ser o estagiário. Sou Laura, do RH.” | Aguardar fala. |
| 2 | Laura aponta a direção da mesa pela orientação do corpo. **Laura:** “Sua mesa fica ali, na salinha. Qualquer coisa, me chama.” | Aguardar fala; não depender de gesto de mão novo. |
| 3 | JV se aproxima até marca livre ou vira a partir de uma posição visível. **JV:** “Sou a JV. A IARA vai receber os chamados com você.” | Aguardar fala e chegada somente se o deslocamento estiver ativo. |
| 4 | JV olha para o computador; Laura recua em direção ao seu posto. **JV:** “Ela sugere. A decisão continua sendo sua.” | Concluir ações essenciais sem esperar animação ociosa. |
| 5 | Encerrar cena; jogador recebe controle. JV e Laura retomam rotina segura. | Marcar 01A concluída uma vez. |

**Alternativa de produção:** se ainda não houver rota segura, posicionar JV perto da marca antes do início do dia, fora da visão do jogador, e usar mudança de orientação; não teletransportá-la durante a cena. A recepção altera o cartão citado acima, mas não concede dica, pontos ou vínculo.

### 01M — O primeiro chamado

**Objetivo dramático:** tornar visível a relação entre JV, o trabalho e a IARA, reagindo ao método usado sem dar bronca ou revelar o próximo gabarito.  
**Gatilho normal:** resultado de Bruna registrado; antes de abrir Caio. Uma vez, mesmo ao sair e reabrir o computador.  
**Participantes:** jogador e JV.  
**Local:** computador ativo com escritório ao fundo. A primeira implementação propõe sobreposição; validar se a posição de JV e suas ações são visíveis. Caso não sejam, usar uma transição controlada ao escritório e restaurar o passo seguinte sem reinicializar o computador.  
**Saída normal:** retomar o roteiro no chamado de Caio.

| Ordem | Ação e diálogo | Condição |
|---|---|---|
| 1 | Congelar o computador após registrar o resultado. JV se aproxima de `aproximacao_mesa` ou já está visível em posição segura; olha para o jogador. | A cena não altera memória, resposta ou resultado. |
| 2 | **JV:** “Já saiu o primeiro?” | Aguardar fala. |
| 3 | Mostrar **uma** das falas da tabela de variantes abaixo. | Selecionar pelo resultado registrado; não inferir conteúdo de ações não salvas. |
| 4 | JV vira para seu posto. **JV:** “Os próximos chegam aí. Se precisar, estou por perto.” | Aguardar fala. |
| 5 | Retomar a rotina de JV e liberar o chamado de Caio no mesmo estado de computador. | Marcar cena concluída antes de aceitar novo input. |

| Resultado de Bruna | Fala de JV |
|---|---|
| `TASK_SOLVED_ALONE` | “Você resolveu direto. Às vezes é o caminho mais rápido.” |
| `TASK_SOLVED_WITH_IARA` | “Você conferiu a sugestão. Boa: é assim que a gente encontra o que passou.” |
| `TASK_SENT_UNCHECKED` | “Já foi? Depois olha o relatório comigo. Ele mostra o que saiu.” |

**Failsafe no primeiro chamado:** após a IARA assumir os restantes, exibir e concluir o relatório; quando o computador terminar de fechar, executar a variante 01M antes de 01F. JV olha para o monitor e diz: “A IARA terminou a fila. Vamos olhar o que ficou registrado.” Ela retorna ao posto. Não executar as falas de Caio nem afirmar que o jogador escolheu delegar. A saída automática atual do failsafe precisa aguardar essa fila de cenas.

**Bloqueio ou interrupção:** se JV não alcançar a marca, usar a posição segura visível prevista para a cena, sem atravessar paredes. Pausa congela a ação. Em recarga, restaurar o último marco confirmado e não registrar Bruna duas vezes. Nenhuma variante modifica pontuação ou oferece dica obrigatória.

### 01F — O que foi assinado

**Objetivo dramático:** encerrar o turno com uma consequência compreensível e um primeiro contraste entre pressa e conferência.  
**Gatilho:** após relatório concluído e computador fechado; no failsafe, após a variante 01M. Uma vez por dia. O dia não termina antes desta cena.  
**Participantes:** jogador, JV e Vasco.  
**Posições:** jogador junto à mesa; JV em posto seguro; Vasco junto ao quadro ou em marca visível. Não exigir um objeto físico de papel novo.  
**Saída:** “Saia do escritório”; conversa e mural continuam opcionais no percurso normal. No failsafe, seguir o encerramento automático depois da cena, sem exigir exploração.

| Ordem | Ação e diálogo | Condição |
|---|---|---|
| 1 | Vasco olha para a mesa do jogador e depois para o quadro. **Vasco:** “Primeiro turno entregue?” | Não iniciar antes de fechar o relatório. |
| 2 | JV olha para o jogador. Exibir uma das falas da tabela abaixo. | Selecionar com base no resultado consolidado. |
| 3 | Vasco muda a orientação para o quadro. **Vasco:** “Certo. Amanhã tem mais.” | Aguardar fala. |
| 4 | JV retorna ao posto; devolver controle e indicar saída, ou continuar encerramento automático do failsafe. | Registrar 01F uma vez; não reaplicar pontuação. |

| Condição, nesta ordem de prioridade | Fala de JV |
|---|---|
| Failsafe aconteceu | “Ela terminou a fila. O relatório ainda fica sob nossa responsabilidade.” |
| Maioria dos chamados enviados sem conferir | “O relatório mostra o que foi enviado. Amanhã, vale olhar com calma.” |
| Demais resultados | “O relatório ficou registrado. A gente continua amanhã.” |

Não dizer que todos os chamados estão certos: resolver com conferência não transforma o relatório inteiro em acerto automático. O texto de encerramento atual “Amanhã tem mais” pode ser mantido como eco do diálogo ou revisto para evitar repetição; decisão de edição antes da implementação.

## 5. Condições e testes da primeira leva

- **Sequência normal:** cartão revisado → 01A → exploração opcional → computador → tutorial → Bruna → 01M → Caio, Júlia, Diego e Tiago → relatório → fechar computador → 01F → saída e cartão final do dia.
- **Failsafe em Bruna:** após três erros, IARA assume chamados restantes → relatório → fechamento do computador → variante 01M → 01F → encerramento automático. Se o failsafe ocorrer depois de 01M, não repetir 01M; executar somente 01F após relatório.
- **Métodos:** percorrer Bruna sozinho, com IARA conferida e enviada sem conferir. Verificar fala correspondente, resultado original e memória; não há julgamento novo nem mudança nos cálculos.
- **Exploração:** conversar com NPCs e ler ou ignorar o mural continua possível; dica é opcional. Fechar e reabrir computador não repete 01M nem reinicia chamados durante a sessão.
- **Persistência:** testar fechar o jogo antes/durante/depois de cada cena, após Bruna e após relatório. Retomar no marco seguro, sem duplicar resposta, dados, efeitos ou pontos. Salvar com cena pendente e migrar saves antigos conforme especificação técnica.
- **Movimentação:** NPCs atuais são estacionários; o movimento antigo por eixo não oferece rotas com destino. Testar trajetória dos participantes, colisão entre jogador e todos os NPCs, bloqueio, posição segura e redimensionamento. Carregar e liberar frames necessários; testar fallback estático sem teletransporte visível.
- **Entrada e pausa:** não reutilizar tecla/clique de abertura; uma entrada não revela e avança ao mesmo tempo. Pausa congela tarefa e cena; retorno restaura entrada sem salto.
- **Teste com jogador:** uma pessoa conclui o dia sem orientação externa; registrar se encontra o computador, percebe diferença entre IARA e JV, entende a responsabilidade do relatório e identifica o próximo objetivo. Medir duração e legibilidade em vez de presumir.

## 6. O que fazer depois

1. Revisar o Dia 1 com roteiro, programação e LMC; observar uma partida e corrigir fala, posição, duração e fluxo.
2. Aprovar a matriz de presença dos nove dias, incluindo as ausências sugeridas nos cartões e as conversas e dicas que precisam permanecer acessíveis.
3. Detalhar os demais dias em lotes, cada cena com gatilho, encenação, fala, variantes, efeito e critérios. Priorizar Dia 3 e Dia 12 para exercitar escolha de vínculo e evidência de revisão, antes de expandir para os demais.
4. Avaliar cenas no meio de mesas em uma primeira tentativa correta, errada e em failsafe; confirmar que interrupções não expõem gabarito nem quebram escolhas opcionais.
5. Considerar novas áreas, portas, objetos físicos, acordos de ajuda, efeitos de privacidade, sprites de gesto e eventuais finais adicionais somente com regras e recursos próprios. O manual alternativo pode inspirar essas decisões, mas não define o resultado atual.
6. Validar a identidade e função de Ricardo, Renê e Mariana com as fontes narrativas da equipe antes de escrever uma revelação definitiva. Atualizar cartões, falas e documentação sempre que uma decisão autoral alterar a continuidade.

## 7. Fontes e limites

Base: `code/src/game/calendar.c`, `npcs.c`, `screens/game_loop.c`, `computer_loop.c`, `computer_days.h`, `core/game_state.[ch]`, `core/save.[ch]`, `docs/lmc/mapeamento-puzzles.md` e US14 da Updated; manual alternativo v0.2 como inspiração, com capítulos 1 e 7 fora do escopo de adaptação. O roteiro e a matriz de arcos são propostas; não são comportamento já implementado. A análise é estática: nenhum playtest, build ou teste automatizado foi executado para este documento.


## Registro da implementação — primeira leva

As cenas 01A/01M/01F foram integradas ao ciclo do Dia 1. 01M usa uma apresentação temporária do escritório, mantendo o módulo do computador carregado e congelado, pois o fundo ampliado do computador original desenha somente a imagem do mapa e não os atores. Ao terminar, retorna à mesma tarefa. Na variante de failsafe, 01M ocorre depois do relatório e antes de 01F.

Laura, JV e Vasco recebem marcas autorais no Dia 1, com poses completas e passos curtos sujeitos a colisão. O recorte não implementa rotinas de navegação para os sete NPCs. O bloqueio entre NPCs no movimento antigo foi corrigido para não permitir atravessamento após dois segundos. Os demais dias mantêm sua organização atual.

O save v2 preserva cenas concluídas e o estado do computador do Dia 1, inclusive seleção em edição, memória, erros, resultados e flags de privacidade. Essa retenção de edição é mais completa que o descarte sugerido na especificação inicial. A máquina principal grava checkpoints quando o conteúdo muda e informa falha de gravação; v1 continua sendo aceito. Saves legados resolvidos não repetem cenas. Gestos dedicados e avaliação com jogador permanecem futuros.


### Validação executada

- Build otimizado concluído com o GCC/raylib configurado no projeto; `iara.exe` recompilado.
- `test_story`: consumo da entrada de abertura, pausa, perda de foco, variantes de método, checkpoint com memória/privacidade/seleções e caminho de failsafe passaram.
- `test_save`: round trip, integridade e migração v1 resolvida passaram.
- `test_fluxo`: 56/56 verificações, incluindo os 100 dias, quatro finais, saída/retorno e failsafe passaram. O condutor automatizado agora avança as cenas antes de continuar o percurso.
- `test_survival`: 111 verificações aprovadas. As verificações de colisões, personagens, NPCs, usabilidade, slots, dicas, sistema e conteúdo também foram executadas durante a integração; conteúdo registrou 35/35 casos.
- Inspeção visual por imagem renderizada da recepção: cenário, corpos dos participantes, retrato e fala legíveis a 1280×720.
- O build adicional com `-Wall -Wextra` mostrou avisos nas rotinas existentes de textos e inicializadores; o build padrão passou. Não foi realizado teste de usabilidade com participante humano.

Os arquivos de evidência históricos regenerados pela suíte foram restaurados para evitar alterações sem relação com esta entrega. A captura específica da recepção fica no diretório ignorado `build/scene01a.png`.
