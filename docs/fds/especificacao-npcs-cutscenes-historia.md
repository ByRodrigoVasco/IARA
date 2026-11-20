# IARA — Especificação de NPCs, cutscenes e tarefas do modo história

> **Atualização de continuidade:** este documento registra a especificação original. Para os dias posteriores, as decisões e o contrato das seções 10 e 11 do [roteiro de encenação](../../../Novas%20referências/roteiro-encenacao-cutscenes.md) substituem propostas conflitantes aqui, em especial Ricardo, Renê, Mariana, presença e migração. A seção 12 do roteiro registra a integração das 21 cenas seguintes, as ausências e a migração.

**Proposta para implementação | 18 de novembro de 2026**  
**Projeto:** `Updated/IARA`, em C e raylib. **Situação:** planejamento; os comportamentos propostos ainda não foram implementados.

## 1. Objetivo e escopo

Criar personagens que trabalhem e reajam no escritório, integrando cenas às tarefas existentes. A prioridade é definir NPCs e comportamentos, depois o executor de cutscenes e a reestruturação das tarefas. O primeiro marco implementável é o Dia 1 completo; em seguida, os outros dias.

Preservar a campanha de 100 dias, seus nove dias jogáveis, puzzles, evidências e quatro finais atuais. Cada **dia jogável** terá pelo menos três cenas principais: abertura, acontecimento durante a tarefa e repercussão. São 27 cenas planejadas abaixo. Dias de flash e saltos não recebem essa exigência.

O manual alternativo v0.2 é inspiração, não especificação obrigatória. Adaptam-se as ideias dos capítulos 2–6 e 8–21, incluindo encenação, tarefas, mapa, rotinas, estado e produção. Os capítulos 1 e 7 estão fora do escopo. Não adotar automaticamente a campanha de três sessões, seus finais, seu mapa ou suas decisões. A interface de diálogo atual pode ser reutilizada; um redesenho de caixa, retratos e digitação não é requisito deste documento.

## 2. Base observada no projeto

- `game/calendar.c` define os dias jogáveis 1, 3, 12, 27, 41, 58, 74, 89 e 100, intercalados com flashes e saltos. Existem cartões de chegada e encerramento.
- `game/npcs.c` define sete NPCs, falas por dia e dica em rodízio. Todos estão configurados como `NPC_STATIONARY`; há suporte a movimento linear, mas não a rotinas com destinos. Os lugares são redistribuídos por dia. A US14 contém detalhes técnicos históricos que divergem do código; conferir o código ao implementar.
- `screens/game_loop.c` reúne mapa, diálogo, computador e cartões. Bilhete ou conversa bloqueia movimento do jogador e atualização normal dos NPCs. O escritório permanece carregado ao abrir o computador.
- `screens/computer_loop.c` e `computer_days.h` definem `DayScript` e `DialogStep`: cinco chamados no Dia 1 e um puzzle nos demais dias, além de apresentação, eventuais escolhas e relatório. Ao alcançar o relatório, resultados e efeitos são consolidados. O failsafe antecipa esse momento e encerra o expediente após a saída do computador.
- O índice do passo e os resultados parciais do computador são variáveis locais, não campos persistidos de `game_state.h`. `core/save.c` grava texto versionado com checksum e utiliza temporário e backup. A retomada exata de tarefa parcial após reiniciar não deve ser presumida.
- `logic.c` calcula os puzzles. O mapeamento de LMC registra revisão de conteúdo ainda pendente à data do documento. Não alterar gabaritos ou pontuação por conveniência da cena.

Estas conclusões vêm de leitura estática do código e da documentação. As cenas e comportamentos abaixo são propostas, não resultados observados em partida.

## 3. NPCs: personalidade, trabalho e comportamento

Funções seguem as falas atuais quando explícitas; personalidade e atuação são propostas ficcionais. Cada personagem precisa de trabalho visível, destino, reação e conversa opcional. Dicas continuam opcionais; cenas obrigatórias não devem obrigar o jogador a pedir ajuda.

| NPC | Personalidade e voz | Rotina inicial proposta | Mudança visível |
|---|---|---|---|
| **Anderson** | Análise de dados; responsável, cansado, evita afirmar sem conferir. Humor discreto. | Mesa → folha para JV → mesa. | Hesita no fechamento do Dia 12; reconhece nome no Dia 58; aponta lacuna do registro no Dia 74. Não presumir que trabalha no Financeiro. |
| **Julio** | Desenvolvimento; sociável e prático, procura atalhos sem ser tratado como incompetente. | Estação → JV → estação. | Questiona treino no Dia 41, verifica catraca no 58 e mostra ausência de autor no log no 74. |
| **JV** | Modelos da IARA; curiosa, precisa, inicialmente defensiva; corrige a própria hipótese. | Compara cartões com a estação. | Acompanha triagem no Dia 1 e compara registros de treino no 41. Revisar com LMC a fala atual que reduz viés apenas aos dados. |
| **Laura** | RH; acolhedora, observa pessoas e tenta aliviar tensões cedo demais. | Entrada → mesas → posto de RH. | Nota mesa vazia no 12, procura colegas no 41 e confere nomes no 89. |
| **Nicollas** | Porão e apoio ao arquivo; reservado, prático, humor seco e atento ao prédio. | Mesa → caixa ou mural acessível → mesa. | Observa água no 27, caixa úmida no 41 e indica mapa no 89; isso não significa ajuda automática no final. |
| **Vasco** | Coordena prioridades; objetivo, cobra prazos para manter o controle. | Quadro → mesas → quadro. | Redistribui fila no 3; confere nomes no 58; no 74 pergunta por pessoas antes de cobrar entrega. |
| **Zito** | TI; tranquilo e competente, prefere demonstrar; fala menos quando assustado. | Mesa de TI → terminal → mesa. | Recebe chamados no 3, examina falha no 58 e aponta divergência no 74, sem resolver puzzle pelo jogador. |
| **IARA** | Assistente útil que gradualmente antecipa ações; confiança não comprova correção. | Computador e infraestrutura já representada. | Falas devem refletir decisões registradas. Nunca dizer que recebeu dado não enviado. |
| **Ricardo, Mariana e Renê** | Direção, pista ligada ao Fundo e confronto final, respectivamente, conforme referências atuais. | Não são parte dos sete NPCs do código de `npcs.c`; começar por documentos ou falas adequadas. | Presença física exige roteiro e assets próprios. Não fundir Ricardo e Renê sem decisão narrativa. |

### 3.1 Rotinas e interação

Cada NPC ativo precisa de ID, presença, posição em coordenadas do mapa, orientação, estado, destino e ação. Estados mínimos: trabalhando, indo, agindo, disponível, conversando, em cena e aguardando rota. Prioridade: cena narrativa > conversa opcional > rotina. Ao terminar uma interrupção, retomar destino válido ou escolher o próximo.

Definir marcos nomeados no escritório real: entrada, mesa do jogador, mesas, mural, computador, quadro e porta. Confirmar coordenadas, colisões e visibilidade antes de usá-los. Arquivo, copa, catraca, painel e Fundo navegáveis são expansões, não áreas presumidamente existentes. Inicialmente representar locais externos por documentos ou cenas adequadas ao espaço atual.

Usar rede de pontos conectados com colisão dos pés, deslocamento por tempo e desenho ordenado pela posição dos pés. Em bloqueio, aguardar e buscar rota alternativa; após limite configurável, registrar falha e sair com segurança. Não atravessar parede ou teletransportar ator visível. Reservar posições de interação, mantendo caminho para o objetivo. Posição lógica deve resistir ao redimensionamento da janela.

A redistribuição de assentos atual dificulta encenação autoral. Nos dias migrados, posições iniciais serão definidas pelo roteiro; manter o comportamento anterior nos dias ainda não migrados. Toda cena define início, destino, posição final e rotina retomada, inclusive se o jogador já estiver perto.

## 4. Escrita e encenação

Cada fala tem uma intenção: pedir, reagir, negar ou revelar um detalhe. Meta de uma ou duas frases, até 22 palavras, sujeita a revisão de legibilidade. Duas a quatro falas geralmente bastam. Sempre que possível, cada cena inclui duas ações distintas, como deslocar e apontar ou interromper e recolher. Até existirem poses próprias, usar direção, deslocamento e objetos já disponíveis como substitutos identificados.

O objetivo visível usa verbo e objeto, preferencialmente até oito palavras. O próximo local deve ser claro, mesmo quando a causa do acontecimento permanece misteriosa. Informação opcional não pode ser a única instrução para completar o dia. Escrever variantes para resolução manual, com IARA conferida, sem conferência e failsafe quando o resultado mudar o sentido da fala. Exposição de dados é fato que não desaparece por uma desculpa.

## 5. Plano de três cenas por dia

**A:** antes da tarefa. **M:** durante a tarefa, em limite seguro. **F:** após resultado consolidado. As três são principais; variantes não são contadas como cenas adicionais. Meta inicial de 15–35 segundos por cena, dependente de teste e tempo de leitura, sem relógio obrigatório. Falas são exemplos para revisão.

| Dia e puzzle | A — abertura | M — durante | F — repercussão |
|---|---|---|---|
| **1: integração e triagem** | **01A Chegada:** Laura acolhe e JV posiciona cartões. “Sua mesa é aquela.” Objetivo: abrir computador. | **01M Conferência:** após primeiro chamado concluído por qualquer método, JV aproxima e aponta ficha: “Ela respondeu. Você conseguiu conferir?” | **01F Assinatura:** após relatório, Vasco consulta quadro e JV recolhe ficha; comentário varia conforme resultado. Objetivo: sair. |
| **3: conectivos e suporte** | **03A Fila:** Vasco entrega lista; Zito observa estação e pede leitura da norma. | **03M Regra:** após primeira tentativa da mesa, Zito aponta bilhete sem revelar resposta; marcas são preservadas. | **03F Destino:** Zito recolhe fila e Vasco registra entrega; reação acompanha resultado. |
| **12: equivalências e fechamento** | **12A Fechamento:** Anderson compara folhas; Laura percebe mesa vazia. | **12M Diferença:** após primeira tentativa, Anderson aponta duas versões sem indicar gabarito. | **12F Carta:** depois da escolha existente, mostra encaminhamento se houve revisão; caso contrário, reconhece falta de confirmação. Preserva evidência conforme regra atual. |
| **27: ata e contradições** | **27A Gravação:** Anderson entrega ata; Nicollas observa indício de água. | **27M Silêncio:** após primeira tentativa, gravação interrompe e Anderson aponta trecho disponível, sem alterar seleção. | **27F Registro:** Nicollas aponta mancha ou marca de água; Vasco recolhe ata. |
| **41: quantificadores e treino** | **41A Arquivo:** JV recebe caixa de Nicollas e separa amostras. | **41M Amostra:** após primeira tentativa, JV compara registro e afasta cartão, sem mudar a mesa. | **41F Treino:** JV observa conjunto; se houve exposição registrada, reconhece o envio, sem alegar revogação. |
| **58: encadeamento e crachás** | **58A Falha:** Zito verifica terminal; Laura confere lista. | **58M Nome:** após primeira tentativa, Zito aponta registro de Diego e Laura consulta nomes. Não concede evidência automaticamente. | **58F Registro:** Laura guarda lista; fala depende de o mural ter sido examinado. A evidência continua vinculada à ação existente. |
| **74: satisfazibilidade e auditoria** | **74A Auditoria:** Julio entrega registro a Anderson; Vasco olha a sala. | **74M Origem:** após primeira tentativa, Julio aponta ausência de autor sem preencher valores da mesa. | **74F Bilhete:** Zito indica mural; se o bilhete ainda não foi lido, investigação permanece opcional e acessível antes da saída. |
| **89: silogismos e lista** | **89A Convite:** Laura confere lista; Vasco procura o próprio nome. | **89M Lista:** após primeira tentativa, Laura aponta nome e recua, sem decidir conclusão lógica. | **89F Mapa:** Nicollas indica mural; se já examinado, reage ao crachá. Encontrar Mariana permanece condicionado à lógica existente. |
| **100: prova por contradição** | **100A Sala:** preparar confronto com Renê usando cenário e tela existentes; presença física exige asset próprio. | **100M Premissas:** após primeira tentativa, levantar pergunta sobre inconsistência sem indicar o par correto; respeitar evidências disponíveis. | **100F Desfecho:** após relatório, reação à prova concluída ou não; então cartão de final usa `get_game_ending()`. |

Cenas M de mesa disparam após a primeira tentativa, inclusive correta, antes de prosseguir ao relatório. Se o failsafe resolver chamados restantes, entregar M pendente antes de F; quando o contexto não permitir, executar variante curta após sair do computador. Nunca exigir erro ou saída para que a cena aconteça.

O código atual mantém os sete NPCs no escritório, embora cartões descrevam ausências. A lista de presença por dia precisa de revisão narrativa; se um participante estiver ausente em versão aprovada, usar documento ou substituto designado sem bloquear a cena. Ajustar cartões de chegada para não contradizerem a nova recepção.

## 6. Reestruturação de tarefas e integração

```text
cartão do dia → cena A → exploração/objetivo → computador
    → marco seguro → cena M → retomada da tarefa → relatório/consolidação
    → cena F → exploração/investigação opcional → saída → encerramento/final
```

O jogador pode fechar e reabrir o computador sem reiniciar a sequência durante a sessão. Para o primeiro recorte, M é camada sobre o computador: congela entradas e temporizadores da tarefa, preserva escritório ao fundo e retorna ao mesmo passo. Se a legibilidade ou atuação exigir saída ao mapa, criar transição explícita sem descarregar o estado da tarefa. Depois de F, a investigação continua opcional. A saída aguarda F; no Dia 100, F precede `show_ending()`.

Eventos propostos: `day_started`, `ticket_completed(id, resultado)`, `attempt_submitted(id, resultado)`, `report_ready`, `computer_closed`, `investigation_completed(id)` e `exit_requested`. IDs de conteúdo são estáveis; valores são copiados, sem ponteiros para estado temporário. Coordenador recebe evento uma vez, avalia condições, enfileira cena e confirma transição após a conclusão. Não disparar por frame.

Hoje o código pressupõe chamados contíguos (`FIRST_TICKET_STEP + ticket`) e relatório último (`REPORT_STEP`). Não inserir cenas como novos `DialogStep` entre chamados sem remover essa suposição. Separar coordenação e conteúdo de puzzle; migrar acessos por índice para IDs, incluindo o failsafe, mantendo cinco chamados do Dia 1 e funções de correção. `resolve_day_tasks` e `apply_day_results` continuam sendo aplicados exatamente uma vez ao alcançar relatório; F lê o resultado consolidado.

## 7. Executor de cutscenes

O executor é independente da tela hospedeira, atualizado uma vez por frame e desenhado na camada correta. Definição contém ID estável, condição, atores, local, comandos, saídas e marco de retomada. Comandos mínimos: `move(actor, marker)`, `face(actor, target)`, `act(actor, pose/object)`, `say(actor, text)`, `wait_action(id)`, `set_object_state`, `emit_event` e `end`. Ações paralelas são explícitas; aguardar somente ações necessárias. Comandos não executam código arbitrário.

| Estado | Contrato |
|---|---|
| Preparação | Validar participantes e marcos; bloquear entrada de exploração/tarefa e consumir a entrada que abriu a cena. |
| Entrada/ação | Deslocar e orientar atores; gestos autorizados podem ocorrer junto de fala. |
| Fala | Usar controles de diálogo existentes. Se houver texto progressivo, uma entrada completa o texto e outra avança; a entrada de abertura não é reaproveitada. |
| Espera de ação | Guardar no máximo um avanço pendente; descartá-lo ao sair do estado. Espera ociosa não impede avanço. |
| Saída | Concluir ações essenciais, aplicar efeitos uma vez, registrar conclusão, restaurar rotina e contexto de entrada. |

Decisões reais ficam no puzzle ou exploração, fora da cena. Pausa congela deslocamento, gestos, texto e temporizadores; retorno ou perda de foco não pode consumir entrada inadvertida. O desenho do mundo permanece ativo. Normalizar teclado e mouse conforme controles existentes, sem impor redesenho da interface.

Se ator, asset ou rota essencial faltar, registrar ID e motivo; apresentar variante curta sem ação espacial ou retornar a estado seguro, mantendo progresso. Não inventar evidência ou efeito de tarefa para compensar falha visual. Marcar cena concluída apenas após a variante terminar. Detectar ciclos, destino inválido e fila cheia durante desenvolvimento.

## 8. Persistência e consequências

Separar conteúdo imutável de estado. Persistir por slot: dia, fase, objetivo, IDs de cenas concluídas e pendente; ID de passo do computador, resultados dos chamados já concluídos, efeitos ainda não consolidados, memória e dados expostos/tarjados necessários; evidências e estatísticas existentes; estado narrativo e destino relevante dos NPCs. Não persistir posição de cada frame como fonte de verdade.

**Marcos seguros:** antes e depois de A/M/F, depois de chamado ou tentativa confirmada e depois do relatório. Ao reiniciar durante uma cena, voltar ao início dela com os resultados anteriores preservados e efeitos da cena ainda não aplicados. Aplicar efeitos e registrar conclusão como transação lógica. Ao fechar durante edição de mesa, recuperar a última tentativa confirmada e informar que edições não enviadas foram descartadas. Não duplicar pontos nem reenviar dados.

Introduzir versão de save nova com leitor explícito de v1, conferindo checksum e campos antes da migração. Não sobrescrever o original até gravação bem-sucedida. Save v1 de dia não resolvido recomeça o dia atual sem inventar progresso parcial; save v1 resolvido preserva pontos/evidências e retoma após relatório, tratando A/M/F daquele dia como legadas concluídas. Dias seguintes usam cenas novas. Testar backup, dados inválidos, slots e falhas de gravação; não afetar Sobrevivência.

Estados e efeitos devem ser idempotentes. Se necessários no futuro, manter `conhecido`, `confiança`, `disponível` e `acordo` distintos: conversar não significa que um aliado aceitou uma ação. A versão inicial preserva **Ileso, Pamonha, Hipnotizado e Ruptura**, com `get_game_ending()` como fonte dos critérios. Novos finais, acesso a portas e regras de vínculo exigem proposta e balanceamento separados.

## 9. Mapa, objetos e assets

Primeiro inventariar marcos, barreiras, objetos, sprites e pontos de interação do escritório atual. O mapa ampliado do manual — copa, arquivo, serviço e Fundo — é ideia posterior que exige planta, transições, colisão, arte e testes; não assumir que já existe.

Objeto novo precisa de ID, posição lógica, pivô, camada, hitbox, estados, ações e regras de persistência. Papel no cenário e papel entregue são o mesmo objeto lógico. Carregar objeto requer âncora de mão ou substituto claro. Água e colisões devem ter representação visível. Som pode reforçar informação, nunca ser o único sinal; respeitar preferências de áudio e acessibilidade.

Reaproveitar sprites e recursos atuais após inventário. Poses de carregar, apontar, usar painel, expressões e objetos adicionais são necessidades a produzir, não recursos garantidos. Definir fallback para cada recurso. Caminhos novos ficam em `core/macros.h`; carregar e liberar recursos raylib no ciclo de vida correto. Validar o fluxo com marcadores e poses existentes antes de produzir animações finais.

## 10. Ordem de implementação

1. **Auditoria narrativa e espacial:** validar presenças por dia, lugares e rotas, sprites e contradições dos cartões; atualizar US14 para distinguir conversas opcionais de cenas obrigatórias.
2. **NPCs:** posições autorais nos dias migrados; estados, destinos, prioridades e rotas; manter dicas e diálogos atuais. Testar bloqueios e interação.
3. **Executor:** comandos, estados, fila, entrada, pausa, encerramento idempotente e falha segura. Testar cena no escritório e sobre o computador.
4. **Tarefas e save:** eventos, checkpoints, resultados parciais, migração v1 e failsafe, sem mudar regras de lógica, pontos e finais.
5. **Fatia vertical do Dia 1:** 01A, 01M e 01F, cinco chamados, métodos, relatório, saída e reload; validar ritmo com jogador.
6. **Demais dias:** entregar 3, 12, 27, 41, 58, 74, 89 e 100 com três cenas, variantes, presenças e investigação verificados.
7. **Polimento e expansão:** gestos, áudio, objetos e cenários adicionais após validação do fluxo.

Áreas candidatas: `game/npcs.[ch]`, novo executor e coordenador em `game/`, `screens/game_loop.c`, `screens/computer_loop.c`, `screens/computer_days.h`, `core/game_state.[ch]`, `core/save.[ch]`, testes e documentação. Novos módulos são proposta, não arquivos existentes; não acrescentar dependência externa.

## 11. Critérios de aceitação

- Sete NPCs têm função, rota segura e reação visível no recorte entregue. Cena/conversa não os deixa presos, atravessando paredes ou bloqueando objetivo; testar redimensionamento. Dica e conversa permanecem opcionais.
- Em cada dia jogável A, M e F ocorrem uma vez e em ordem, inclusive com primeira tentativa correta, envio sem conferência e failsafe. Abrir/fechar computador e repetir interação não duplica cena ou efeito.
- Entrada de abertura não avança a primeira fala; excesso de entradas não pula várias falas. Pausa e perda de foco não avançam cena. Ações essenciais terminam antes de saída; espera ociosa não prende a cena.
- Jogador conclui sem dica nem investigação opcional; após F existe objetivo acessível. Fala respeita resultados e evidências e não entrega gabarito.
- Dia 1 mantém cinco chamados; outros dias mantêm puzzles, três formas de resolução e correção. Rodar testes existentes de lógica, conteúdo, fluxo, dicas, NPCs e slots; comparar pontos e finais com resultados equivalentes antes/depois.
- Fechar e recarregar antes, durante e depois de cenas, após chamado, tentativa e relatório. Confirmar retenção de resultados, descarte informado de edição não enviada e ausência de efeitos duplicados. Testar v1 resolvido e não resolvido, checksum, backup, falha de gravação e modo Sobrevivência.
- Observar uma pessoa completar o Dia 1 sem ajuda; verificar compreensão do objetivo, reconhecimento dos colegas, distinção entre cena e decisão e entendimento do resultado. Medir tempo real e conferir legibilidade/contraste em configurações suportadas.

## 12. Decisões pendentes

1. Presença definitiva de cada NPC por dia, conciliando código e textos sobre ausências.
2. Relação canônica entre Ricardo, Renê e Mariana; esta proposta não os funde.
3. Legibilidade das cenas sobre o computador; prototipar antes de escolher transições ao mapa.
4. Gestos e objetos efetivamente disponíveis e marcos exatos após vistoria do mapa.
5. Eventual expansão de consequências de dados, acordos e objetos; não mudar finais ou estatísticas sem regras revisadas.
6. Revisão de LMC dos enunciados, dicas e afirmações sobre viés e treino.

## 13. Fontes e limites

- `Roteiro-e-Manual-IARA/manual_iara_alternativo_v0_2.md` e PDF homônimo: inspiração para personagens, encenação, cenas, tarefas, mapa, estado e assets; capítulos 1 e 7 excluídos da adaptação.
- `Updated/IARA/code/src/game/npcs.c`, `calendar.c`: elenco, falas, posições e calendário existentes.
- `Updated/IARA/code/src/screens/game_loop.c`, `computer_loop.c`, `computer_days.h`: fluxo, passos, tarefas e relatório.
- `Updated/IARA/code/src/core/game_state.[ch]`, `save.[ch]`, `save_slots.c`: resultados, finais, evidências e slots.
- `Updated/IARA/docs/lmc/mapeamento-puzzles.md`: mapeamento de puzzles e pendências, respeitando a data e os limites daquele documento.
- `Updated/IARA/docs/fds/diagramas-atividade/US14-interacao-npcs.md`: critério de conversas opcionais; detalhes técnicos confrontados com código atual.

Este documento é uma proposta; não foram realizados build, testes automatizados ou partida para produzi-lo.


## Registro da primeira implementação

O recorte 01A/01M/01F está descrito em `roteiro-encenacao-cutscenes.md`. O Dia 1 usa marcas autorais para Laura, JV e Vasco e passos curtos com verificação de colisões, sem exigir a navegação completa dos sete NPCs. 01M apresenta temporariamente o escritório e congela o computador carregado; depois retorna ao passo seguinte. O save v2 também retém edições da tarefa, além dos resultados confirmados, em vez de descartar seleções não enviadas. O roteiro contém o alcance da primeira leva; as rotinas completas e os demais dias continuam planejados.
