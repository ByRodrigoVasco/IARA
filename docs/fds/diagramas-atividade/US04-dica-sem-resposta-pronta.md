# US04: Dica leve (NPC/exploração)

**User Story:** Como Lívia, quero uma dica leve quando travar em um puzzle, vinda de um NPC ou por exploração, dependendo do puzzle, nunca a resposta pronta, para sentir que continuo sendo eu quem resolve a tarefa.

## Diagrama de atividades

```plantuml
@startuml
start
:Jogador trava em um puzzle;
if (Tipo de dica disponível?) then (NPC)
  :Jogador conversa com o NPC;
  :NPC oferece dica em diálogo;
else (Exploração)
  :Jogador examina cenário/item;
  :Dica é revelada pela exploração;
endif
:Jogador continua tentando resolver com a dica;
:Pontuação da tarefa não é reduzida;
stop
@enduml
```

**Critérios cobertos:** dica vem de NPC ou exploração, nunca da IARA; pedir dica não reduz a pontuação.
