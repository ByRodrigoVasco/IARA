# Testes de sistema

Relatório da execução dos testes de sistema do IARA: o jogo testado como um todo,
simulando o uso real de um jogador.

| | |
|---|---|
| **Data da execução** | 22/10/2026 |
| **Versão testada** | branch `develop`, commit `b366e7c` |
| **Ambiente** | Windows 11, MSYS2 UCRT64, gcc 16.2.0, raylib |
| **Resultado** | **32 de 35 casos aprovados** — 3 falhas, todas registradas como bug |
| **Veredito** | **Aprovado com ressalvas**: nenhuma falha impede jogar |
| **Atualização de 02/11/2026** | CT-06, CT-24 e CT-35 ampliados e CT-36 a CT-75 incluídos: **75 de 75 aprovados**. Os BUG-01 a BUG-08 estão corrigidos na branch. [Log da execução](evidencias/execucao-2026-11-02.txt) e [matriz de rastreabilidade](rastreabilidade.md) |
| **Atualização de 03/11/2026** | Branch `test/campanha-100-dias`, com o CT-76 incluído: **76 de 76 aprovados**. BUG-09 a BUG-11 corrigidos e BUG-12 em aberto. [Log da execução](evidencias/execucao-2026-11-03.txt) |
| **Screencast da Sprint 02** | Execução de 03/11/2026 sobre a `develop` (`63c2d01`), gravada: **76 de 76 aprovados**. [Vídeo](../sprint-02/screencast-testes.mp4) e [log](../sprint-02/execucao-screencast-2026-11-03.txt) |
| **Atualização de 07/11/2026** | Branch `test/us03-modo-automatico`, com o CT-77 e o CT-78 incluídos (US03: PI2-226 a PI2-228). BUG-13 e BUG-14 corrigidos |

## Como executar

```powershell
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

O script compila e roda os sete conjuntos de teste e devolve um resumo. O código
de saída é `0` quando tudo passa e `1` quando há falha, o que permite ligar a
suíte a um CI mais adiante.

O parâmetro `-Pausado` espera uma tecla entre um conjunto e o outro, para dar tempo
de narrar durante a gravação do screencast.

## O que foi testado

A suíte monta o jogo de verdade — os mesmos módulos, os mesmos arquivos de mídia,
a mesma máquina de estados — e o conduz como um jogador faria. Não há simulação de
fachada: `test_sistema` (CT-01 a CT-16) inclui `game_loop.c` para alcançar o estado
interno real da partida, `test_conteudo` (CT-17 a CT-24, CT-37, CT-41, CT-43, CT-47 a CT-52, CT-57, CT-58, CT-61 a CT-70, CT-74, CT-77 e CT-78) percorre os chamados e o menu, e
`test_fluxo` (CT-25 a CT-36, CT-42 e CT-44 a CT-46, CT-53 a CT-56, CT-59, CT-60, CT-71 a CT-73, CT-75 e CT-76) monta o jogo inteiro menos a `main.c` e o percorre do
começo ao fim com a janela à vista: abertura, menu, CRÉDITOS e a volta ao menu,
INICIAR, cartão do dia, caminhada pelo escritório, tutorial, os cinco chamados (um deles conferindo a sugestão da IARA), o relatório do dia, a volta ao mapa e a saída pela porta, que encerra o dia. Depois repete o dia pelos outros caminhos: colinha do mural, tudo enviado sem conferir, failsafe e saída do computador no meio do dia. Junto rodam os testes de unidade
`test_collisions`, `test_characters`, `test_save` e `test_hints`.

## Casos de teste

| ID | Caso | Resultado |
|---|---|---|
| CT-01 | Janela do jogo inicializa | OK |
| CT-02 | Fonte do jogo cobre os acentos do português (16 de 16) | OK |
| CT-03 | Todos os arquivos de mídia usados pelo jogo existem (25 de 25) | OK |
| CT-04 | Mapa do escritório carregado na placa de vídeo | OK |
| CT-05 | Sprites das 8 direções do jogador carregados | OK |
| CT-06 | Frames do andar das 8 direções carregados | OK |
| CT-07 | Animação do andar percorre os 4 frames em ciclo | OK |
| CT-08 | Jogador consegue ir do spawn até o computador (72 passos) | OK |
| CT-09 | Jogador nasce livre, fora de qualquer obstáculo | OK |
| CT-10 | Jogador nunca entra em um obstáculo ao andar (20.000 passos) | OK |
| CT-11 | Jogador nunca sai dos limites do mapa | OK |
| CT-12 | Empurrar contra as 4 bordas não atravessa a parede | OK |
| CT-13 | Zona do computador só acende com o jogador em cima | OK |
| CT-14 | Redimensionar a janela mantém o jogador no mesmo ponto do mapa | OK |
| CT-15 | Fade in chega à opacidade total (alpha 255) | OK |
| CT-16 | Fade out chega à transparência total (alpha 0) | OK |
| CT-17 | Roteiro do computador íntegro (textos, opções e gabarito) | OK |
| CT-18 | Roteiro abre com uma tela de texto e fecha com o relatório do dia | OK |
| CT-19 | Jogador percorre o roteiro do início ao fim (5 chamados) | OK |
| CT-20 | Fim do roteiro encerra a tela do computador | OK |
| CT-21 | Menu abre em INICIAR, destravado | OK |
| CT-22 | Seleção do menu circula nos dois sentidos | OK |
| CT-23 | Confirmar guarda a opção escolhida e fecha o menu | OK |
| CT-24 | Abrir e fechar o menu várias vezes não acumula memória | OK |
| CT-25 | Partida começa no dia 1, em um dia jogável | OK |
| CT-26 | Estatísticas ocultas começam no valor base | OK |
| CT-27 | Tela de abertura termina sozinha e entrega o menu (283 quadros) | OK |
| CT-28 | Escolher CRÉDITOS abre a tela de créditos e ela permanece aberta | OK |
| CT-29 | Voltar dos créditos e tela desconhecida devolvem o jogador ao menu | OK |
| CT-30 | Escolher INICIAR no menu começa a partida | OK |
| CT-31 | Cartão do dia termina e o escritório aparece (10,1 s) | OK |
| CT-32 | Personagem atravessa o escritório até o computador (53 passos) | OK |
| CT-33 | Interagir com o computador abre a tela do quiz | OK |
| CT-34 | Jogador resolve todos os chamados e conclui o dia pelo relatório | OK |
| CT-35 | Texto do computador cabe na caixa, nos botões e na janela em 7 tamanhos | OK |
| CT-36 | Escolher SAIR pede o fechamento e mantém a janela viva para o save | OK |
| CT-37 | Alto contraste liga e desliga na tela de configurações | OK |
| CT-38 | Jogador consegue ir do spawn até o item da dica (246 passos) | OK |
| CT-39 | Examinar a dica abre o bilhete e não altera a pontuação | OK |
| CT-40 | Contornos seguem distinguíveis com protanopia e deuteranopia simuladas | OK |
| CT-41 | Brilho respeita os limites e altera a imagem | OK |
| CT-42 | Escolher CONFIGURAÇÕES abre a tela e ela permanece aberta | OK |
| CT-43 | Preferências voltam do arquivo e arquivo inválido não quebra o jogo | OK |
| CT-44 | Concluir o quiz devolve o jogador ao mapa, andando e com a tarefa resolvida | OK |
| CT-45 | Jogador alcança a porta, encerra o dia e segue para o Flash do Dia 2, com o Dia 1 salvo | OK |
| CT-46 | Flash do Dia 2 só avança ao toque e abre o Dia 3, com a tarefa nova e o jogador na porta | OK |
| CT-47 | Resolver sozinho não exige abrir a IARA e cada chamado vale 10 pontos | OK |
| CT-48 | Conferir a IARA vale o mesmo que resolver sozinho, e pegar erro dela soma 5 e Percepção | OK |
| CT-49 | Enviar sem conferir não trava, vale 0, soma Automação e o erro aparece no relatório | OK |
| CT-50 | Erro mostra dica, abre nova tentativa e o acerto zera a contagem | OK |
| CT-51 | Terceiro erro seguido aciona o failsafe: a IARA conclui o que falta e o jogador não trava | OK |
| CT-52 | Colinha traz os 5 símbolos, começa fora do monitor e não altera a pontuação | OK |
| CT-53 | Contador mostra Dia 1 de 100 e ler o mural dá a colinha e registra a investigação, sem somar pontos ainda | OK |
| CT-54 | Colinha fica colada no canto inferior esquerdo do monitor, sem cobrir a janela da IARA | OK |
| CT-55 | Enviar sem conferir devolve o jogador ao mapa e o fim do dia conta que a IARA resolveu | OK |
| CT-56 | Failsafe passa pelo relatório e encerra o dia direto no cartão, e recomeçar não acumula pontos | OK |
| CT-57 | Chamado com CPF pede a tarja, que vai ao relatório sem mudar os pontos, e a chefia só pressiona | OK |
| CT-58 | Investigar soma 5 para quem resolveu, não conta duas vezes e soma Percepção em todo caso | OK |
| CT-59 | Sair do computador volta ao mapa sem concluir, e voltar retoma o mesmo ponto com os pontos certos | OK |
| CT-60 | Partida nova recomeça os chamados do início | OK |
| CT-61 | Avaliador acerta a tabela-verdade dos 5 conectivos e aplica modus ponens | OK |
| CT-62 | Mesa de triagem aceita uma única combinação, diz só quantos erros há e a IARA erra a isca | OK |
| CT-63 | Memória da IARA cai com o uso, a confiança sobe, e nova conversa recupera sem apagar o que vazou | OK |
| CT-64 | Calendário tem os 9 dias jogáveis do GDD, o Dia 2 é um flash e o Dia 4 abre um bloco de salto | OK |
| CT-65 | Avaliador reconhece equivalência, tautologia, contradição e silogismo válido | OK |
| CT-66 | Cada puzzle dos dias 3 a 100 tem gabarito calculado e único, sem múltipla escolha | OK |
| CT-67 | IARA acerta os dias 3 e 27, erra os outros com memória cheia e erra todos sem memória | OK |
| CT-68 | Treinos diferentes mudam o resultado da mini-IA, e o viés deixa a Periferia sem aprovação (CA3) | OK |
| CT-69 | Decisão do Dia 12 muda Automação ou Percepção (CA2), e dias delegados reduzem a memória inicial da IARA | OK |
| CT-70 | Os 4 finais saem das condições do GDD, sem sorte, em todas as combinações (CA6) | OK |
| CT-71 | Campanha passa pelos 100 dias em ordem: 9 jogáveis, 14 flashes e o resto em salto | OK |
| CT-72 | Investigar, conversar e fechar a prova leva ao Final D, com as 3 evidências e a Mariana | OK |
| CT-73 | Depois do final, o jogo volta ao menu e INICIAR recomeça do Dia 1 sem herdar nada | OK |
| CT-74 | Três erros em qualquer puzzle acionam o failsafe, vão ao relatório e não contam como prova | OK |
| CT-75 | Save retoma no salto, no dia jogável com o roteiro certo e depois de um dia já encerrado (CA4) | OK |
| CT-76 | Botões das mesas de todos os dias cabem, com o texto legível, em 7 tamanhos de janela | OK |
| CT-77 | Todo chamado dos 9 dias jogáveis oferece resolver sozinho ou pedir à IARA, e o manual vale pontos (US03, PI2-226) | OK |
| CT-78 | Modo automático resolve na hora, nunca trava nem aciona o failsafe e vale 0 em todos os dias, com e sem memória e com e sem tarja (US03, PI2-227 e PI2-228) | OK |

## Evidência do percurso

O CT-08 procura um caminho do spawn até o computador dando um passo de jogador por
vez, com as funções de colisão reais. A imagem abaixo é gerada pela própria execução
do teste: verde é o spawn, azul é o caminho encontrado, laranja são os 25 obstáculos
do escritório e amarelo é a zona de interação do computador.

![Percurso do jogador do spawn até o computador, com as hitboxes de colisão](evidencias/percurso-e-colisoes.png)

O bloco 3 vai além e roda a partida na tela: abertura, menu, cartão do dia,
caminhada pelo escritório e entrada no computador. A imagem abaixo é um quadro
capturado pelo próprio teste, com o personagem parado em frente ao computador e a
seta de interação acesa.

![Personagem em frente ao computador no escritório, durante a execução do teste](evidencias/escritorio-em-jogo.png)

O bloco segue até o computador e resolve os cinco chamados, na cena com o
escritório ao fundo, a mesa, o monitor e o desktop. Foi nesta captura que apareceu
o [BUG-08](bugs.md#bug-08), já corrigido: o enunciado vazava para fora da caixa.

![Primeiro chamado respondido, com o ícone de acerto](evidencias/quiz-respondido.png)

Concluídos os chamados, a seta passa para a porta de vidro, e sair por ela mostra o cartão de fim do dia.

![Personagem em frente à porta de vidro, com a seta indicando a saída](evidencias/saida-pela-porta.png)

![Cartão de fim do Dia 1](evidencias/fim-do-dia.png)

Os outros caminhos do dia também geram evidência: a sugestão da IARA, a conferência no manual, a tarja de CPF, o relatório do dia, a dica depois de um erro, a colinha pega no mural e o failsafe do terceiro erro.

![Sugestão da IARA, com os botões Conferir e Enviar sem conferir](evidencias/sugestao-da-iara.png)

![Conferência das afirmações da IARA com o trecho do manual](evidencias/conferindo-a-iara.png)

![Chamado com CPF: tarjar antes de enviar à IARA](evidencias/tarja-de-cpf.png)

![Relatório do dia depois de enviar tudo sem conferir](evidencias/relatorio-do-dia.png)

![Resposta errada com a dica e o botão Tentar de novo](evidencias/erro-com-dica.png)

![Colinha colada no canto inferior esquerdo do monitor](evidencias/colinha-no-monitor.png)

![Terceiro erro seguido: a IARA assume o que falta](evidencias/failsafe-terceiro-erro.png)

![Cartão de fim do dia depois do failsafe](evidencias/fim-do-dia-failsafe.png)

![Flash do Dia 2](evidencias/flash-do-dia-2.png)

![Dia 3: tabela-verdade na mesa de triagem](evidencias/dia-3-mesa.png)

![Dia 12: equivalências lógicas na conciliação](evidencias/dia-12-mesa.png)

![Dia 41: treino da mini-IA com quantificadores](evidencias/dia-41-enviada.png)

![Dia 100: prova por contradição](evidencias/dia-100-mesa.png)

![Escritório alagado no Dia 89](evidencias/escritorio-no-dia-89.png)

![Mesa de triagem do chamado com símbolos lógicos](evidencias/mesa-de-triagem.png)

![Envio da mesa com um destino errado: o jogo diz quantos, não quais](evidencias/triagem-com-erro.png)

![IARA com pouca memória, confiante e errada](evidencias/iara-sem-memoria.png)

![Conferência sem a página citada: o jogador procura no manual](evidencias/procurando-no-manual.png)

![Botão de sair na barra de tarefas, ao lado da cobrança da chefia](evidencias/sair-do-computador.png)

## Bugs encontrados

Oito problemas, três pegos pelos testes automatizados e cinco por inspeção do código
durante a escrita da suíte. O detalhamento e o texto pronto para abrir cada issue
estão em **[bugs.md](bugs.md)**.

| ID | Problema | Gravidade | Como foi achado |
|---|---|---|---|
| [BUG-01](bugs.md#bug-01) | Fade in para em alpha 250 e nunca fica 100% opaco | Baixa | CT-15 |
| [BUG-02](bugs.md#bug-02) | Voltar ao menu vaza memória sem limite (~7 MB por volta) | **Alta** | CT-24 |
| [BUG-03](bugs.md#bug-03) | `unload_office()` libera a textura do jogador duas vezes | Média | Inspeção |
| [BUG-04](bugs.md#bug-04) | `unload_game_loop()` e `unload_computer_loop()` nunca são chamadas | Baixa | Inspeção |
| [BUG-05](bugs.md#bug-05) | `close_pre_menu()` desenha fora do par `BeginDrawing`/`EndDrawing` | Baixa | Inspeção |
| [BUG-06](bugs.md#bug-06) | `PERSONAGENS_PATH` em minúsculo quebra o jogo fora do Windows | Média | Inspeção |
| [BUG-07](bugs.md#bug-07) | Pré-menu nunca descarrega as imagens da memória RAM | Média | Inspeção |
| [BUG-08](bugs.md#bug-08) | Texto do quiz vaza para fora da caixa e dos botões (corrigido) | Média | CT-35 |
| [BUG-09](bugs.md#bug-09) | Rótulos das mesas vazam dos botões (corrigido) | Média | Medição, CT-76 |
| [BUG-10](bugs.md#bug-10) | Falha ao trocar o save pode perder o progresso (corrigido) | Média | Inspeção, `test_save` |
| [BUG-11](bugs.md#bug-11) | Fundo do cartão do dia é carregado e nunca desenhado (corrigido) | Baixa | Inspeção |
| [BUG-12](bugs.md#bug-12) | Menu não tem como começar uma partida nova (aberto) | Média | Inspeção |
| [BUG-13](bugs.md#bug-13) | Suíte de testes deixou de compilar depois do áudio (corrigido) | Alta | Execução da suíte |
| [BUG-14](bugs.md#bug-14) | "Enviar sem conferir" esperava a IARA terminar de escrever (corrigido) | Média | Inspeção, US03 |

Do BUG-01 ao BUG-11 estão corrigidos na `develop`, e o status de cada um fica em [bugs.md](bugs.md). Nenhum deles impedia jogar: o jogo abre, o menu funciona, o
personagem anda, colide e chega ao computador, e o quiz vai até o fim.

## O que a suíte ainda não cobre

**Entrada de teclado e mouse de verdade**: a raylib não permite injetar teclas, então
os testes chamam as mesmas funções de movimento que `handle_player_input()` chama,
substituindo só a leitura do teclado — o mapeamento tecla → direção continua sendo
verificado à mão. Pelo mesmo motivo, o mouse preso à tela do monitor, a tecla F11, a caixa
de tela cheia e o arraste da barra de brilho também são verificados à mão: os testes desligam a captura para não prender o mouse
de quem os executa. Os cliques no quiz e na escolha do modo chamam as mesmas funções dos botões, sem passar pelo mouse. **Aparência**: os testes provam que as texturas carregam e que as
posições batem, não que a tela está legível. **Telas não implementadas**: dia de
flash e dia de salto não têm o que testar. O CT-29 verifica, para os créditos, a
volta ao menu com uma tecla simulada (só nesse ponto a leitura de teclado é
substituída, e o resto da tela é o código real) e que uma tela desconhecida também
devolve o jogador ao menu. **Haskell**: as regras puras de PIF ficam de fora.

O roteiro para gravar o screencast desta execução está em
[roteiro-screencast.md](roteiro-screencast.md).


## PI2-224 — vidro e fachada inferior (02/11/2026)

Os dois painéis inferiores e a porta de entrada permitem caminhar atrás do vidro.
A personagem também entra parcialmente atrás dos trechos opacos da fachada,
respeitando a mesma profundidade máxima dos vidros. O limite físico inferior
coincide com a linha de baixo da porta (y=808 no asset, referência y=369,5), em toda
a largura do mapa. Não é permitido afundar no rodapé ou na placa NEXO. As plantas
e jardineiras têm bloqueio em toda a área visível, incluindo vasos e folhagem;
as jardineiras inferiores ocupam também a parte inferior dos vasos, antes ausente.
As salas da direita e os móveis mantêm seus bloqueios. A porta oferece passagem visual, sem transição para outro mapa.

`collision_tile` distingue `TILE_SOLID` e `TILE_GLASS`; o vidro fornece somente
sua base como retângulo bloqueante. `collides_with_tiles` oferece a consulta
tipada e `collides_with_any`/`move_axis` preservam as assinaturas existentes.
A superfície visível é definida em `glass_surface_reference`, separadamente
da profundidade física em `glass_reference`. F1 mostra os bloqueios efetivos.

A cena e a personagem são desenhadas em uma RenderTexture. Um shader GLSL 330
aplica 12% de azul somente nas superfícies de vidro, sem deslocar pixels e sem
usar tempo ou ondas. Os reflexos fixos reutilizam o asset; molduras, divisória
da porta, rodapés, pilares e parede central são redesenhados à frente. Dicas de
controle e seta ficam fora do efeito. A textura intermediária é recriada ao
redimensionar e, junto do shader, é liberada ao sair do escritório ou encerrar o
jogo. Se o shader não estiver disponível, reflexos e oclusão continuam.

Validação: build completo sem diagnósticos do compilador; testes de unidade de
colisão aprovados; testes gráficos VID-01 a VID-08 em 800×450, 1280×720,
1366×768, 1920×1080 e 1024×768:

| Caso | Verificação |
|---|---|
| VID-01 | Entrar atrás dos painéis, porta e pilares sem colidir com a fachada |
| VID-02 | Movimento bloqueado ao tentar ultrapassar a base inferior |
| VID-03 | Pixels alterados no vidro e preservados fora das superfícies |
| VID-04 | Molduras redesenhadas sobre a parte sobreposta da personagem |
| VID-05 | Quadros idênticos com a personagem parada, mesmo após decorrer tempo |
| VID-06 | Varredura da fachada com passos verticais e diagonais sem ultrapassar a linha inferior da porta |
| VID-07 | Plantas e jardineiras bloqueiam os quatro lados e a parte inferior dos vasos |
| VID-08 | Personagem permanece visível no piso entre pilares e jardineiras, nos dois lados |

A suíte completa manteve as falhas preexistentes CT-15 (fade in), CT-24
(memória do menu) e CT-35 (layout do quiz). Na execução completa anterior, com sete casos do vidro, os testes
de sistema somaram 39/42 aprovados. Após a correção visual, o bloco 1 foi
executado novamente: 23/24 aprovados, com somente CT-15 reprovado; os oito
casos VID passaram nas cinco resoluções. O VID-08 falhou antes da correção e
passou depois, confirmando a regressão visual. Os três conjuntos de unidade passaram.

Os recortes opacos da parede central começam na sua borda real, em y=808 no
asset. A faixa de piso acima dela não é redesenhada sobre a personagem. Essa
correção altera somente a oclusão visual; as colisões aprovadas permanecem iguais.

![Personagem inteira no piso entre a jardineira e o pilar](evidencias/personagem-no-piso.png)

![Personagem atrás do painel inferior direito](evidencias/vidro-inferior.png)

![Personagem atrás do vidro da porta](evidencias/vidro-porta.png)

![Limite de colisão alinhado à base da porta, destacado em vermelho](evidencias/parede-inferior.png)

Para conferir manualmente, caminhe para baixo diante dos painéis, da porta e
dos dois pilares inferiores. A personagem deve aparecer através do vidro e
ficar parcialmente oculta pelas áreas opacas sem ultrapassar a base da porta;
a fachada deve permanecer estática. Nenhuma planta ou jardineira pode ser atravessada. As superfícies,
as bases físicas e a camada de primeiro plano devem ser revistas se `office.png`
mudar. As salas de vidro da direita permanecem fora do escopo desta tarefa.
