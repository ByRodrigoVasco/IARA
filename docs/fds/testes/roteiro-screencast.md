# Roteiro do screencast dos testes

Vídeo de **2 minutos** mostrando a suíte de testes rodando, para anexar ao card
*"Gravar e anexar screencast dos testes"*.

A suíte faz o trabalho sozinha: você só aperta gravar, roda **um comando** e fala
quatro frases.

---

## 1. Antes de gravar (2 minutos)

1. Abra o VS Code na pasta `IARA`.
2. Abra o terminal integrado: `Ctrl + '`
3. Aumente a fonte do terminal com `Ctrl + Scroll` até o texto ficar bem legível.
4. Feche abas e notificações que não interessam.

Pronto. Não precisa preparar mais nada.

---

## 2. Gravar (2 minutos)

**Comece a gravação:** `Win + G` abre a Xbox Game Bar → botão de gravar.
(Ou use o OBS, se preferir.)

**Cole o comando no terminal e dê Enter.** Repare no `-Pausado` no fim — é ele que
segura a saída entre um conjunto e o outro, esperando você apertar uma tecla:

```
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1 -Pausado
```

A partir daqui **você controla o ritmo**. A cada parada aparece
`---- pressione qualquer tecla para continuar ----`: fale, e só então aperte.

**Pausa 1, depois de "Compilando":**

> "Esse é o projeto IARA. Acabei de compilar a suíte de testes de sistema, que
> testa o jogo inteiro simulando o uso de um jogador."

**Pausas 2 e 3** (os dois testes de unidade, que passam rápido): pode só apertar a
tecla, ou dizer "esses dois são os testes de unidade que já existiam".

**Pausa 4, depois do bloco 1** — é o bloco mais rico, vale a pena rolar a tela para
cima enquanto fala:

> "Dezesseis casos aqui: ele conferiu os vinte e cinco arquivos de mídia, fez o
> personagem andar vinte mil passos pelo escritório sem atravessar nada, e achou um
> caminho do spawn até o computador em setenta e dois passos."

**Pausa 5, depois do bloco 2:**

> "Aqui ele percorreu o quiz inteiro e testou o menu. Esse CT-24 que falhou abriu e
> fechou o menu vinte vezes medindo a memória."

**Durante o bloco 3 a janela do jogo abre sozinha e fica uns 7 segundos na tela.**
Aponte para ela e fale:

> "Essa é a partida de verdade rodando dentro do teste: o cartão do dia e o
> escritório."

**Pausa 6, no resumo final:**

> "Vinte e nove dos trinta e um casos passaram. Os dois que falharam são bugs reais,
> já documentados: o fade in que não chega a cem por cento, e o menu que vaza
> memória cada vez que é reaberto."

**Pare a gravação.** Fim.

---

## 3. Depois de gravar (2 minutos)

1. Suba o vídeo no YouTube como **não listado** (é o que a squad já usa nas outras
   entregas).
2. Cole o link no card do Trello *"Gravar e anexar screencast dos testes"*.
3. Acrescente o link na tabela de entregas em [`docs/fds/README.md`](../README.md).

---

## Se quiser mostrar mais

Opcional, só se sobrar vontade: depois do resumo, abra a imagem
`docs/fds/testes/evidencias/percurso-e-colisoes.png` e diga que ela é gerada pelo
próprio teste — verde é onde o jogador nasce, azul é o caminho encontrado até o
computador, laranja são os obstáculos do escritório.

O relatório completo dos testes está em [README.md](README.md) e os bugs em
[bugs.md](bugs.md), caso o professor peça.
