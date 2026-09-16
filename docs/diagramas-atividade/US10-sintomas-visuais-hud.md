# US10: Feedback visual, sem HUD numérico

**User Story:** Como Lívia, quero perceber o estado do jogo por sintomas visuais e narrativos como mudanças de cenário, em vez de números na tela, para viver a tensão da história sem ficar calculando estatísticas.

## Diagrama de atividades

```plantuml
@startuml
start
while (Chegou ao fim do jogo?) is (Não)
  :Jogador realiza ações/tarefas;
  :Pontuação interna é atualizada, sem exibir números na tela;
  if (Mudança de estado é perceptível?) then (Sim)
    :Cenário e tom da narração mudam visualmente;
    :Eventos no ambiente refletem a pontuação atual;
  endif
endwhile (Sim)
:Tom da narração reflete todo o percurso;
stop
@enduml
```

**Critérios cobertos:** nenhum número de status visível; tom da narração muda perceptivelmente do início ao fim; eventos do ambiente seguem a pontuação atual.
