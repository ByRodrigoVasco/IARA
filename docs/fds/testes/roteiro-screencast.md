# Roteiro do screencast dos testes

Vídeo curto mostrando a suíte de testes sendo executada, para anexar ao card
*"Gravar e anexar screencast dos testes"*.

O card pede apenas **"um vídeo curto mostrando os testes sendo executados"**.
Narrar é opcional. O caminho mais simples é gravar mudo, e ele já entrega o card.

---

## Caminho simples: vídeo mudo (5 minutos no total)

### Antes

1. Abra o VS Code na pasta `IARA`.
2. Abra o terminal: `Ctrl + '`
3. Aumente a fonte do terminal com `Ctrl + Scroll`.
4. Feche abas e notificações que não interessam.

### Gravar

1. Aperte `Win + G` e clique no botão de gravar da Xbox Game Bar.
2. Clique no terminal, cole o comando e dê Enter:

```
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

3. Não faça nada. Deixe rodar **cerca de 40 segundos**.
4. Quando aparecer o `RESUMO DA SUITE`, espere 2 segundos e pare: `Win + Alt + R`.

O vídeo fica em `Vídeos\Capturas`.

### Depois

1. Suba no YouTube como **Não listado**.
2. Cole o link no card do Trello.
3. Acrescente o link na tabela de entregas em [`docs/fds/README.md`](../README.md).

**Acabou.**

---

## O que aparece no vídeo

Vale saber para não achar que travou:

| Momento | O que aparece |
|---|---|
| 0s a 3s | Compilação dos cinco conjuntos |
| 3s a 11s | Blocos 1 e 2 passando no terminal |
| 11s a 16s | A tela de abertura do jogo, com o nome IARA |
| 16s a 19s | O menu principal |
| 19s a 29s | O cartão **"Dia 1"**. Fica parado ~10s, e **isso é normal**: é o tempo real da animação do jogo |
| 29s a 34s | O escritório, com o personagem andando e esbarrando nos móveis |
| 34s a 38s | O personagem chega no computador e a tela do quiz abre |
| 38s em diante | O resumo final no terminal |

A parte do jogo não é gravação de tela colada depois: é a partida de verdade,
conduzida pelo próprio teste.

---

## Se quiser narrar (opcional)

Só faça isso **depois** de já ter o vídeo mudo salvo, como garantia.

Use o parâmetro `-Pausado`, que segura a saída entre um bloco e outro esperando
você apertar uma tecla:

```
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1 -Pausado
```

A cada parada aparece `---- pressione qualquer tecla para continuar ----`. Fale, e
só então aperte. São seis paradas. Sugestões do que dizer:

**Depois de "Compilando":**

> "Esse é o projeto IARA. Acabei de compilar a suíte de testes de sistema."

**Depois do bloco 1:**

> "Dezesseis casos: conferiu os vinte e cinco arquivos de mídia, fez o personagem
> andar vinte mil passos pelo escritório sem atravessar nada, e achou um caminho do
> spawn até o computador."

**Depois do bloco 2:**

> "Aqui ele percorreu o quiz inteiro e testou o menu. Esse CT-24 que falhou abriu e
> fechou o menu vinte vezes medindo a memória."

**Durante o bloco 3**, aponte para a janela do jogo:

> "Essa é a partida de verdade rodando dentro do teste: a abertura, o menu, o
> cartão do dia, o personagem andando pelo escritório e a tela do computador."

**No resumo final:**

> "Trinta e um dos trinta e três casos passaram. Os dois que falharam são bugs
> reais, já documentados."

---

## Ferramenta de gravação

A **Xbox Game Bar** (`Win + G`) já vem no Windows e basta. O **OBS Studio** dá mais
controle, se você quiser mostrar terminal e jogo lado a lado.
