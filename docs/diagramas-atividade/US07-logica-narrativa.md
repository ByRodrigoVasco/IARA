# US07: Lógica ensinada pela narrativa

**User Story:** Como Lívia, quero aprender os conceitos de lógica resolvendo as tarefas do protagonista dentro da história, sem telas de teoria separadas, para não sentir que estou fazendo uma aula disfarçada de jogo.

## Diagrama de atividades

```plantuml
@startuml
start
:Jogador encontra um novo conceito lógico no contexto da tarefa;
while (Entendeu o suficiente pelo contexto?) is (Não)
  :Explicação embutida (diálogo, e-mail, documento ou colinha visual);
  :Jogador busca dica adicional via NPC/exploração;
endwhile (Sim)
:Puzzle resolvido sem tela de teoria isolada;
stop
@enduml
```

**Critérios cobertos:** nenhum puzzle precedido de tela de teoria separada; jogador consegue entender o suficiente pelo contexto da tarefa.
