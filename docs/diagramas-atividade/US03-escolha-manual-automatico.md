# US03: Escolha manual ou automático

**User Story:** Como Lívia, quero escolher entre resolver manualmente ou passar a tarefa para a IARA decidir, para eu controlar quando vale gastar meu tempo praticando ou avançar rápido.

## Diagrama de atividades

```plantuml
@startuml
start
:Tarefa do dia apresentada;
if (Jogador escolhe o modo?) then (Manual)
  repeat
    :Jogador tenta resolver a tarefa;
  repeat while (Entrada é válida?) is (Não) not (Sim)
  if (Jogador acertou?) then (Sim)
    :Resultado positivo aplicado;
  else (Não)
    :Resultado negativo aplicado;
  endif
else (Automático)
  :IARA resolve a tarefa;
  :Sem recompensa narrativa;
endif
stop
@enduml
```

**Critérios cobertos:** as duas opções sempre disponíveis; automático sem recompensa; resultado manual depende do acerto; entradas inválidas rejeitadas sem travar o jogo.
