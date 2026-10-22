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

**Fale a frase 1**, com o VS Code na tela:

> "Esse é o projeto IARA. Vou rodar a suíte de testes de sistema, que testa o jogo
> inteiro simulando o uso de um jogador."

**Cole o comando no terminal e dê Enter:**

```
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

**Fale a frase 2** enquanto os testes rolam (uns 40 segundos):

> "São 31 casos de teste. Ele confere os arquivos de mídia, faz o personagem andar
> vinte mil passos pelo escritório e procura um caminho até o computador."

**Quando a janela do jogo abrir sozinha, aponte e fale a frase 3:**

> "Essa é a partida de verdade rodando dentro do teste: o cartão do dia e o
> escritório."

**No resumo final, fale a frase 4:**

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
