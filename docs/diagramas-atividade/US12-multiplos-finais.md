# US12: Múltiplos finais por escolhas

**User Story:** Como Matheus, quero que o jogo tenha múltiplos finais determinados pelas minhas escolhas, e não por sorte, para poder mostrar pro público que cada decisão teve peso real na história.

## Diagrama de atividades

```plantuml
@startuml
start
:Sistema avalia o padrão de escolhas acumuladas;
switch (Qual foi o padrão predominante?)
case (Tudo manual, sem IA)
  :Final A: "Ileso";
case (Delegou quase tudo à IARA)
  :Final B: "Pamonha";
case (Errou quase tudo)
  :Final C: "Passe no RH mais tarde";
case (Outro padrão)
  :Final D;
endswitch
stop
@enduml
```

**Critérios cobertos:** pelo menos 4 finais distintos; final depende do padrão de escolhas; mesmo padrão repetido leva ao mesmo final (sem aleatoriedade).
