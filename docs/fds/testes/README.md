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

## Como executar

```powershell
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

O script compila e roda os cinco conjuntos de teste e devolve um resumo. O código
de saída é `0` quando tudo passa e `1` quando há falha, o que permite ligar a
suíte a um CI mais adiante.

O parâmetro `-Pausado` espera uma tecla entre um conjunto e o outro, para dar tempo
de narrar durante a gravação do screencast.

## O que foi testado

A suíte monta o jogo de verdade — os mesmos módulos, os mesmos arquivos de mídia,
a mesma máquina de estados — e o conduz como um jogador faria. Não há simulação de
fachada: `test_sistema` (CT-01 a CT-16) inclui `game_loop.c` para alcançar o estado
interno real da partida, `test_conteudo` (CT-17 a CT-24) percorre o quiz e o menu, e
`test_fluxo` (CT-25 a CT-35) monta o jogo inteiro menos a `main.c` e o percorre do
começo ao fim com a janela à vista: abertura, menu, CRÉDITOS e a volta ao menu,
INICIAR, cartão do dia, caminhada pelo escritório e as quatro perguntas do quiz. Junto rodam os dois testes de unidade que já existiam,
`test_collisions` e `test_characters`.

## Casos de teste

| ID | Caso | Resultado |
|---|---|---|
| CT-01 | Janela do jogo inicializa | OK |
| CT-02 | Fonte do jogo cobre os acentos do português (16 de 16) | OK |
| CT-03 | Todos os arquivos de mídia usados pelo jogo existem (25 de 25) | OK |
| CT-04 | Mapa do escritório carregado na placa de vídeo | OK |
| CT-05 | Sprites das 8 direções do jogador carregados | OK |
| CT-06 | Frames do andar (leste e oeste) carregados | OK |
| CT-07 | Animação do andar percorre os 4 frames em ciclo | OK |
| CT-08 | Jogador consegue ir do spawn até o computador (72 passos) | OK |
| CT-09 | Jogador nasce livre, fora de qualquer obstáculo | OK |
| CT-10 | Jogador nunca entra em um obstáculo ao andar (20.000 passos) | OK |
| CT-11 | Jogador nunca sai dos limites do mapa | OK |
| CT-12 | Empurrar contra as 4 bordas não atravessa a parede | OK |
| CT-13 | Zona do computador só acende com o jogador em cima | OK |
| CT-14 | Redimensionar a janela mantém o jogador no mesmo ponto do mapa | OK |
| CT-15 | Fade in chega à opacidade total (alpha 255) | **FALHA — [BUG-01](bugs.md#bug-01)** |
| CT-16 | Fade out chega à transparência total (alpha 0) | OK |
| CT-17 | Roteiro do computador íntegro (textos, opções e gabarito) | OK |
| CT-18 | Roteiro abre e fecha com uma tela de texto | OK |
| CT-19 | Jogador percorre o roteiro do início ao fim (4 perguntas) | OK |
| CT-20 | Fim do roteiro encerra a tela do computador | OK |
| CT-21 | Menu abre em INICIAR, destravado | OK |
| CT-22 | Seleção do menu circula nos dois sentidos | OK |
| CT-23 | Confirmar guarda a opção escolhida e fecha o menu | OK |
| CT-24 | Abrir e fechar o menu várias vezes não acumula memória | **FALHA — [BUG-02](bugs.md#bug-02)** |
| CT-25 | Partida começa no dia 1, em um dia jogável | OK |
| CT-26 | Estatísticas ocultas começam no valor base | OK |
| CT-27 | Tela de abertura termina sozinha e entrega o menu (283 quadros) | OK |
| CT-28 | Escolher CRÉDITOS abre a tela de créditos e ela permanece aberta | OK |
| CT-29 | Voltar dos créditos e tela desconhecida devolvem o jogador ao menu | OK |
| CT-30 | Escolher INICIAR no menu começa a partida | OK |
| CT-31 | Cartão do dia termina e o escritório aparece (10,1 s) | OK |
| CT-32 | Personagem atravessa o escritório até o computador (53 passos) | OK |
| CT-33 | Interagir com o computador abre a tela do quiz | OK |
| CT-34 | Jogador responde as 4 perguntas e conclui o quiz | OK |
| CT-35 | Texto do computador cabe dentro da caixa e dos botões | **FALHA — [BUG-08](bugs.md#bug-08)** |

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

O bloco segue até o computador e responde as quatro perguntas. Foi nesta captura
que apareceu o [BUG-08](bugs.md#bug-08): repare no enunciado vazando para fora da
caixa cinza, cortado nas duas bordas.

![Tela do quiz com a primeira pergunta respondida e o ícone de acerto](evidencias/quiz-respondido.png)

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
| [BUG-08](bugs.md#bug-08) | Texto do quiz vaza para fora da caixa e dos botões | Média | CT-35 |

Nenhum deles impede jogar a versão atual: o jogo abre, o menu funciona, o
personagem anda, colide e chega ao computador, e o quiz vai até o fim.

## O que a suíte ainda não cobre

**Entrada de teclado e mouse de verdade**: a raylib não permite injetar teclas, então
os testes chamam as mesmas funções de movimento que `handle_player_input()` chama,
substituindo só a leitura do teclado — o mapeamento tecla → direção continua sendo
verificado à mão. **Aparência**: os testes provam que as texturas carregam e que as
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
