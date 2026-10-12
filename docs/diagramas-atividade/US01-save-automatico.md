# US01: Save automático ao fechar

**User Story:** Como Lívia, quero que meu progresso salve automaticamente ao fechar o jogo, para retomar de onde parei nas minhas horas vagas curtas.

## Diagrama de atividades

```plantuml
@startuml
start
:Jogador fecha o jogo;
:Progresso salvo automaticamente em arquivo local;
:Jogador reabre o jogo depois;
if (Arquivo de save existe e não está corrompido?) then (Sim)
  :Retoma no mesmo dia em que parou;
else (Não)
  :Inicia nova partida sem travar;
endif
stop
@enduml
```

**Critérios cobertos:** retomada no mesmo dia; sem conta/internet; salvo em arquivo local; save ausente ou corrompido não trava o jogo.
