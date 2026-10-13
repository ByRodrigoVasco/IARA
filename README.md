# IARA
 
Você investiga um caso de horror corporativo envolvendo IARA, uma inteligência
artificial fictícia. Quanto mais questiona as respostas dela em vez de
aceitá-las automaticamente, mais precisa recorrer à lógica para provar
as inconsistências. Assim, a mecânica ensina ao mesmo tempo ética em IA e lógica clássica.


## Ferramentas tecnológicas
 ![C](https://img.shields.io/badge/c-%23A8B9CC.svg?style=for-the-badge&logo=c&logoColor=black)
![raylib](https://img.shields.io/badge/raylib-000000.svg?style=for-the-badge)
![Haskell](https://img.shields.io/badge/haskell-%235D4F85.svg?style=for-the-badge&logo=haskell&logoColor=white)
![GitHub](https://img.shields.io/badge/github-%23181717.svg?style=for-the-badge&logo=github&logoColor=white)
- **Motor e gameplay:** C, para o game loop, menus, gerenciamento de estado e persistência.
- **Renderização:** [raylib](https://www.raylib.com/), biblioteca gráfica em C, 2D.
- **Regras puras:** Haskell, para os módulos de lógica, validação de respostas e seleção de conteúdo.
- **Versionamento:** GitHub, com fluxo de branches e Pull Requests descrito no [`CONTRIBUTING.md`](CONTRIBUTING.md).

## Como compilar e executar

Todos os comandos devem ser executados **na raiz do repositório**, porque o jogo carrega os assets por caminhos relativos (`code/assets/...`).

### Requisitos

- Compilador `gcc` (no Windows, via [MSYS2](https://www.msys2.org/) UCRT64).
- Biblioteca [raylib](https://www.raylib.com/): no MSYS2, `pacman -S mingw-w64-ucrt-x86_64-raylib`; no Linux, o pacote `raylib` da sua distribuição.

### Windows (MSYS2 UCRT64)

```bash
gcc code/src/*.c -o iara.exe -I C:/msys64/ucrt64/include -L C:/msys64/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
./iara.exe
```

### Linux

```bash
gcc code/src/*.c -o iara -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./iara
```

No VS Code, o mesmo processo está na tarefa `run raylib` (`.vscode/tasks.json`).

> **Versão para terminal (Unidade 1 de PIF):** a versão em texto, exigida na primeira entrega parcial, ainda está em desenvolvimento. Esta seção será atualizada com os comandos de compilação e execução dela.

## Backlog e board

![Backlog](docs/assets/entregas/backlog.jpeg)

[Link Trello](https://trello.com/b/ifJwhbVe/iara-projeto-integrador-squad-8)

![Board](docs/assets/entregas/board.jpeg)

## Screencast do Protótipo de Baixa Fidelidade

<a href="https://youtu.be/qE2Gn79bmqw">
   <img src="docs/screencast/Thumbnail da Screencast.png" width="800" alt="Watch the video" />
</a>

 
## Artefatos das disciplinas

| Disciplina / frente | Artefato |
|---|---|
| Gestão ágil | [Board e backlog no Trello](https://trello.com/b/ifJwhbVe/iara-projeto-integrador-squad-8) |
| Design | [Protótipos lo-fi no Figma](https://www.figma.com/design/DpnmgBif3vJcn336pefbpa/IARA---PROTOTIPO-FDS?node-id=0-1&t=vm69jod3fxAtJGjN-1) e [documentação dos protótipos](docs/prototipos-lofi/README.md) |
| Engenharia de software | [Diagramas de atividade por história de usuário](docs/diagramas-atividade/README.md) e [guia de contribuição](CONTRIBUTING.md) |
| Programação imperativa (C) | Código-fonte em [`code/src`](code/src) |
| Programação funcional (Haskell) | A definir: o módulo de regras ainda não foi adicionado ao repositório |

## Documentação

- **Diagramas de atividade:** um diagrama UML por história de usuário, traduzindo os critérios de aceitação em fluxo. Ver [`docs/diagramas-atividade/README.md`](docs/diagramas-atividade/README.md).
- **Protótipos lo-fi:** telas de baixa fidelidade das histórias de usuário, elaboradas no [Figma](https://www.figma.com/design/DpnmgBif3vJcn336pefbpa/IARA---PROTOTIPO-FDS?node-id=0-1&t=vm69jod3fxAtJGjN-1). Ver [`docs/prototipos-lofi/README.md`](docs/prototipos-lofi/README.md).

## Squad
 
Fotos e sprites ficam em [`docs/assets/`](docs/assets/README.md).
 
| Foto | Sprite | Nome | Função | GitHub | LinkedIn | Email |
|---|---|---|---|---|---|---|
| ![](docs/assets/team/anderson-santos.png) | ![](docs/assets/sprites/anderson-santos.png) | Anderson Santos | Lógica matemática | [acampospsantos](https://github.com/acampospsantos) | [andersoncampospsantos](https://www.linkedin.com/in/andersoncampospsantos) | acps@cesar.school |
| ![](docs/assets/team/joao-pedro.png) | ![](docs/assets/sprites/joao-pedro.png) | João Pedro | Desenvolvimento das regras em Haskell | [jpsouza25](https://github.com/jpsouza25) | [joão-pedro-souza-costa](https://www.linkedin.com/in/jo%C3%A3o-pedro-souza-costa-a0456430b/) | jpsc@cesar.school |
| ![](docs/assets/team/joao-vitor-medeiros.png) | ![](docs/assets/sprites/joao-vitor-medeiros.png) | João Vitor Medeiros | Desenvolvimento do motor em C | [JvMedeiros7](https://github.com/JvMedeiros7) | [jvmedeiros7](https://www.linkedin.com/in/jvmedeiros7/) | jvm7@cesar.school |
| ![](docs/assets/team/julio-cesar-lins.png) | ![](docs/assets/sprites/julio-cesar-lins.png) | Julio Cesar Lins | Desenvolvimento do motor em C | [Lins-ju](https://github.com/Lins-ju) | [júlio-lins](https://www.linkedin.com/in/j%C3%BAlio-lins-123905182/) | jcsl@cesar.school |
| ![](docs/assets/team/laura-brayner.png) | ![](docs/assets/sprites/laura-brayner.png) | Laura Brayner | Design e integração | [laurabrayner](https://github.com/laurabrayner) | [laurabrayner](https://www.linkedin.com/in/laurabrayner) | lcbc@cesar.school |
| ![](docs/assets/team/luan-arruda.png) | ![](docs/assets/sprites/luan-arruda.png) | Luan Arruda | Design e integração | [luan-arruda](https://github.com/luan-arruda) | [luan-arruda-548a9534b](https://www.linkedin.com/in/luan-arruda-548a9534b) | las9@cesar.school |
| ![](docs/assets/team/nicolas-mauricio.png) | ![](docs/assets/sprites/nicolas-mauricio.png) | Nicolas Mauricio | Gestão ágil | [nicollasfr](https://github.com/nicollasfr) | [nicollasmfranca](https://www.linkedin.com/in/nicollasmfranca/) | nmcf@cesar.school |
| ![](docs/assets/team/rodrigo-vasco.png) | ![](docs/assets/sprites/rodrigo-vasco.png) | Rodrigo Vasco | Engenharia de software | [ByRodrigoVasco](https://github.com/ByRodrigoVasco) | [rodrigo-vlm](https://www.linkedin.com/in/rodrigo-vlm/) | rvlm@cesar.school |
