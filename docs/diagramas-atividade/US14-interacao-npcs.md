# US14: Interação opcional com NPCs

**User Story:** Como Lívia, quero poder interagir/conversar com NPCs, para ter uma alternativa que não seja só resolver puzzle sozinha.

## Diagrama de atividades

```plantuml
@startuml
start
if (Jogador quer interagir com um NPC?) then (Sim)
  :Jogador aborda o NPC;
  :Diálogo opcional: contexto, humor ou dica;
endif
:Progresso do jogador segue livre;
stop
@enduml
```

**Critérios cobertos:** NPCs alcançáveis por escolha do jogador; pelo menos 1 interação de diálogo disponível; interação opcional, não bloqueia o progresso.
