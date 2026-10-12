# US04b: Failsafe após 3 erros

**User Story:** Como Lívia, quero que o jogo sempre me deixe avançar mesmo se eu não conseguir resolver um puzzle, para não ficar travada sem conseguir continuar a história.

## Diagrama de atividades

```plantuml
@startuml
start
:Contador de erros = 0;
while (Jogador acertou o puzzle?) is (Não)
  :Jogador tenta resolver o puzzle manualmente;
  :Contador de erros +1;
  if (3 erros seguidos?) then (Sim)
    :Dia jogável avança automaticamente;
    :IARA resolve a tarefa silenciosamente;
    :Tarefa pontua -10, como feita pela IARA;
    stop
  endif
endwhile (Sim)
:Tarefa concluída pelo jogador;
stop
@enduml
```

**Critérios cobertos:** 3 erros seguidos avançam o dia automaticamente; tarefa pontua como feita pela IARA; jogador nunca fica bloqueado.
