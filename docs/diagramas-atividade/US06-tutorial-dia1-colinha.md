# US06: Tutorial Dia 1 + colinha visual

**User Story:** Como Lívia, quero um tutorial de integração claro no Dia 1 que apresente os símbolos lógicos básicos e como funcionam os dois modos de jogo, para entender Manual e Automático antes de puzzles mais difíceis.

## Diagrama de atividades

```plantuml
@startuml
start
:Tutorial narrativo apresenta os dois modos de jogo;
:Colinha visual (post-it) com símbolos lógicos é disponibilizada;
if (Jogador escolhe o modo?) then (Manual)
  :Jogador usa a colinha para resolver a tarefa do Dia 1;
else (Automático)
  :IARA resolve a tarefa do Dia 1;
endif
:Tarefa do Dia 1 concluída sem penalidades;
stop
@enduml
```

**Critérios cobertos:** Dia 1 jogável do início ao fim sem penalidades; colinha visual (¬, ∧, ∨, →, ↔) acessível nos dois modos; jogador entende a diferença entre os modos ao final.
