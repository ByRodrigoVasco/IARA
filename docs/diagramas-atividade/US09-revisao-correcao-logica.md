# US09: Revisão de correção lógica

**User Story:** Como Wagner, quero ter confiança de que cada puzzle está logicamente correto e alinhado ao conteúdo da disciplina, para poder recomendar o jogo em sala de aula sem medo de ensinar algo errado.

## Diagrama de atividades

```plantuml
@startuml
start
:Equipe documenta o conceito lógico formal do puzzle;
while (Solução e pistas alinhadas ao conceito formal?) is (Não)
  :Puzzle é ajustado/corrigido;
endwhile (Sim)
:Puzzle aprovado para publicação;
stop
@enduml
```

**Critérios cobertos:** conceito lógico formal documentado internamente; conferência pelo material da disciplina antes de finalizar; nenhum puzzle publicado sem revisão.
