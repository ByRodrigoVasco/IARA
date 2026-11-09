# Usabilidade: as 10 heurísticas de Nielsen no IARA

Auditoria de 09/11/2026, feita lendo o código de todas as telas e conferindo por captura de tela em
1600x900, 1280x720 e 960x540. Não houve teste com jogadores: o que está aqui é o que o código faz e o que a
suíte confere. Os casos de teste novos têm o código `UX-nn` (`code/tests/test_ux.c`, mais o `test_conteudo.c`
e o `test_fluxo.c`).

## Resumo

| # | Heurística | Já cumpria | Faltava | Agora |
|---|---|---|---|---|
| 1 | Visibilidade do status | Barra de carregamento com porcentagem, "Dia X de Y" no jogo, mensagens de compra no Plano de Carreira | Salvar era silencioso, e uma falha ao salvar só ia para o log | Aviso "Progresso salvo." e aviso de erro com o que conferir |
| 2 | Mundo real | Textos em português, sem jargão | Nada relevante | Sem mudança |
| 3 | Controle e liberdade | ESC volta um passo nas telas de escolha, nos créditos e nas configurações; ESC sai do computador | **No menu e no escritório o ESC fechava o jogo na hora**, e a campanha não tinha pausa nem como voltar ao menu | Pausa na campanha (ESC ou P) com CONTINUAR, COMO JOGAR, VOLTAR AO MENU e SAIR DO JOGO |
| 4 | Consistência | Estilo comum nas telas de modo, de save e de carregamento | Menu, créditos e configurações sem dica de teclas, cada tela com seu jeito | Mesmas peças (`ui_theme.h`) e dicas de tecla no rodapé de todas as telas |
| 5 | Prevenção de erros | Apagar save pede confirmação | **SAIR e ESC fechavam o jogo sem perguntar; DESISTIR da partida não perguntava** | Pergunta antes de sair (menu e pausa) e antes de desistir; o botão que cancela já vem escolhido |
| 6 | Reconhecimento | Dicas de movimento e de ação na tela do escritório, "BLOQUEADO: como liberar" nos personagens | Teclas do menu e das configurações só na memória | Dicas de tecla sempre à vista |
| 7 | Flexibilidade | Teclado e mouse em todas as telas, F11 para tela cheia | Sem atalho de pausa na campanha | ESC e P pausam, nas duas modalidades |
| 8 | Minimalismo | Telas enxutas | Nada relevante | Sem mudança |
| 9 | Erros claros | Nomes de save inválidos explicam o motivo | Falha ao salvar sem mensagem | "Não consegui salvar o progresso. Confira se há espaço livre no disco e se a pasta do jogo permite gravar arquivos." |
| 10 | Ajuda | Nada | **Não havia ajuda em lugar nenhum** | AJUDA no menu, COMO JOGAR na pausa e no início do Sobrevivência |

## O que foi feito, onde

**Peças comuns** (`code/src/ui`)
- `ui_theme.h` e `ui_widgets.c`: cores, painel, botão, texto com quebra e dica de rodapé.
- `notice.c`: aviso curto no alto da tela, desenhado por `end_game_drawing()`, então vale em qualquer tela. O de erro
  fica 7 s, os outros 2,6 s.
- `confirm_dialog.c`: janela de confirmação. O botão de cancelar já vem escolhido, ESC cancela, e a tecla que abriu a
  janela não vale como resposta.
- `pause_menu.c`: pausa da campanha. Guarda a imagem do jogo como fundo e não atualiza nada enquanto está aberta.

**Campanha**
- Menu: opção AJUDA, dicas de tecla no rodapé, ESC e SAIR perguntam antes de fechar.
- Escritório: ESC ou P pausam. Dentro do computador o ESC continua só saindo do computador.
- Voltar ao menu pela pausa salva e mostra "Progresso salvo.".
- `help.c`: a ajuda, com uma página por modo.

**Modo Sobrevivência** (branch `feature/modo-sobrevivencia`, que segue separada da `develop`)
- Início: opção COMO JOGAR.
- Pausa: COMO JOGAR e DESISTIR com confirmação; dica de teclas no rodapé.
- Falha ao gravar o progresso (`sv_meta_last_save_ok()`) mostra o aviso de erro.

## O que ficou de fora

- Desfazer uma resposta do quiz do computador, ou restaurar os padrões nas configurações: não existem hoje e pediriam
  uma decisão de design (a resposta errada tem consequência na história).
- Passar das dicas de tecla textuais para ícones.
- A abertura (pré-menu) ainda não pode ser pulada com uma tecla.
- Teste com jogadores de verdade: as notas acima vêm da leitura do código e das capturas.

## Como conferir

```powershell
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

Casos: UX-01 a UX-03 (aviso), UX-04 a UX-06 (confirmação), UX-07 e UX-08 (ajuda), UX-09 a UX-12 (pausa),
UX-13 e UX-14 (menu), UX-15 e UX-16 (ESC e ajuda no fluxo). Na branch do Sobrevivência, SV-94 e SV-95 (aviso de falha ao salvar).
