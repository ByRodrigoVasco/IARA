# US11: Pontuação varia por método

**User Story:** Como Lívia, quero que a forma como resolvo cada tarefa influencie no meu resultado final que recebo, para sentir que minhas escolhas ao longo do jogo realmente importam.

## Diagrama de atividades

```plantuml
@startuml
start
while (Fim da partida?) is (Não)
  switch (Como a tarefa foi resolvida?)
  case (Manual + dica)
    :+10 pontos;
  case (Manual sem ajuda)
    :+10 pontos;
  case (Automático)
    :-5 pontos;
  case (Failsafe)
    :-10 pontos;
  endswitch
  :Peso somado à pontuação acumulada da partida;
endwhile (Sim)
:Final reflete o padrão acumulado, não a última tarefa;
stop
@enduml
```

**Critérios cobertos:** pesos diferentes por método (manual = manual+dica > automático); valores 10/-5/-10; peso acumulado ao longo da partida; final reflete o padrão geral.
