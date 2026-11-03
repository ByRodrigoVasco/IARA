# Roteiro de teste em máquina limpa (US02)

Valida que o jogo roda em uma máquina de laboratório, sem o ambiente de
desenvolvimento. A auditoria de rede vale para o código atual; repita o roteiro a
cada entrega.

## Auditoria de rede

- Nenhuma chamada de rede no código-fonte (`socket`, `winsock`, `curl`, `http`, `OpenURL`, `system`, `ShellExecute`).
- Nenhum binário do pacote importa biblioteca de rede (`ws2_32`, `wsock32`, `wininet`, `winhttp`, `urlmon`, `dnsapi`, `iphlpapi`).
- Com o jogo aberto, o processo não tem conexão TCP nem porta UDP (`PKG-03`).

## Na máquina de testes

Use um computador sem MSYS2, gcc ou raylib, com usuário comum (sem administrador).

| # | Passo | Esperado | Resultado |
|---|---|---|---|
| 1 | Desligar o Wi-Fi e o cabo de rede | Sem internet | |
| 2 | Copiar `IARA-windows.zip` por pendrive e extrair na Área de Trabalho | Extrai sem pedir permissão | |
| 3 | Dar dois cliques em `iara.exe` | Abre sem aviso de DLL ausente | |
| 4 | Jogar: menu, INICIAR, escritório, quiz | Sem imagem faltando e sem lentidão perceptível | |
| 5 | Ver os CRÉDITOS | As fotos da equipe aparecem | |
| 6 | Fechar pelo X | `iara.save` aparece ao lado do executável | |
| 7 | Abrir de novo e sair pela opção SAIR | Fecha sem mensagem de erro | |

Máquina usada: ________ Windows: ________ Placa de vídeo: ________ Data: ________ Responsável: ________

## Limites conhecidos

- Requer Windows 10 ou mais recente (usa o runtime UCRT do sistema) e placa de vídeo com OpenGL 3.3.
- O Windows SmartScreen pode avisar na primeira abertura, porque o executável não é assinado.
- Em pasta sem permissão de gravação o jogo funciona, mas o progresso não fica salvo.
