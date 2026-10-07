# US02: Rodar sem instalação/conta

**User Story:** Como Wagner, quero que o jogo rode em qualquer máquina do laboratório sem instalação, conta ou internet, para usá-lo numa aula sem depender do suporte de TI para instalar.

## Diagrama de atividades

```plantuml
@startuml
start
:Usuário executa o arquivo do jogo;
if (Requer instalação?) then (Não)
  :Jogo abre diretamente, sem privilégio de administrador;
  if (Requer login/internet?) then (Não)
    :Jogo roda offline, sem conta de usuário;
    if (Máquina é padrão de laboratório/Windows?) then (Sim)
      :Jogo roda sem dependências externas;
    endif
  endif
endif
stop
@enduml
```

**Critérios cobertos:** sem instalação; sem login/internet; sem privilégio de admin; executável autocontido; roda sem travamentos em máquina padrão de laboratório.
