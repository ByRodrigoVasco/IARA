# Guia de Contribuição - Projeto IARA

Este documento é para definir como deve ser o trabalho no repositório. Fluxo de branches, padrão de commits e processo de Pull Request.

## 0. Configurar o ambiente

O jogo é em C com [raylib](https://www.raylib.com/) e roda no Windows. O caminho abaixo usa o MSYS2, o mesmo de quem já desenvolve o projeto.

1. **MSYS2.** Baixe em [msys2.org](https://www.msys2.org/) e instale em `C:\msys64`, sem mudar a pasta.
2. **Terminal.** No menu Iniciar, abra o **MSYS2 UCRT64** (não o MSYS nem o MINGW64: as bibliotecas abaixo são da variante UCRT64).
3. **Dependências.**

   ```bash
   pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
   ```

4. **Clonar e ativar o hook de commits** (uma vez só, logo depois de clonar):

   ```bash
   git clone https://github.com/ByRodrigoVasco/IARA.git
   cd IARA
   git config core.hooksPath .githooks
   git checkout develop
   ```

5. **Compilar** (a partir da raiz do projeto):

   ```bash
   gcc -O2 code/src/*.c code/src/*/*.c -Icode/src -Icode/src/core -Icode/src/screens -Icode/src/ui -Icode/src/game -Icode/src/survival -o iara.exe -lraylib -lopengl32 -lgdi32 -lwinmm
   ```

6. **Executar:**

   ```bash
   ./iara.exe
   ```

   O jogo procura os assets relativos à pasta do executável, então ele abre certo de qualquer terminal ou atalho, desde que o `iara.exe` fique na raiz do projeto (ou no pacote).

### Testes

A suíte tem 11 conjuntos, que abrem uma janela de verdade. Rode no PowerShell, na raiz do projeto, antes de abrir qualquer PR:

```powershell
powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1
```

Ela regrava as imagens em `docs/fds/testes/evidencias/`. Essas imagens **não vão no commit**: depois da suíte, rode `git checkout -- docs/fds/testes/evidencias`. O catálogo dos casos e os bugs conhecidos estão em [docs/fds/testes/](docs/fds/testes/README.md).

### Pacote para outras máquinas

```powershell
powershell -ExecutionPolicy Bypass -File tools/pacote/empacotar.ps1
powershell -ExecutionPolicy Bypass -File tools/pacote/testar_pacote.ps1
```

O primeiro gera `build/IARA-windows.zip`, com um `iara.exe` autocontido (assets e bibliotecas dentro dele, via `tools/release/build_release.ps1`). Quem recebe só abre o `iara.exe`, sem instalar nada. O teste em máquina real segue o [roteiro de máquina limpa](docs/fds/testes/maquina-limpa.md).

### Problemas comuns

| Sintoma | Causa provável |
|---|---|
| `raylib.h: No such file or directory` | O terminal aberto não é o UCRT64, ou o `pacman -S` do passo 3 não terminou |
| O jogo abre sem imagens ou sem som | O `iara.exe` está fora da raiz do projeto, longe da pasta `code/assets` |
| O commit é recusado | A primeira linha não segue `tipo: descrição` (veja a seção 2) |
| A suíte falha em um caso de memória (CT-24) de vez em quando | Variação do ambiente, que também acontece na `develop` limpa: rode de novo antes de investigar |

## 1. Fluxo de branches (GitFlow)

| Branch | Papel | Nasce de | Vai para |
|--------|-------|----------|----------|
| `main` | Sempre estável. Reflete as entregas oficiais (U1, U2). | — | — |
| `develop` | Branch de integração. Onde as features aprovadas se juntam. | `main` | — |
| `feature/<escopo>` | Uma funcionalidade em desenvolvimento. | `develop` | `develop` |
| `release/<versao>` | Preparação de uma entrega (ex:`release/v1.0.1`). Só correções, sem features novas. | `develop` | `main` **e** `develop` |
| `hotfix/<descricao>` | Correção urgente em algo já entregue. | `main` | `main` **e** `develop` |

Regras:
- Ninguém commita direto em `main` ou `develop`. Sempre via Pull Request.
- Uma branch `feature/` corresponde a uma User Story ou item técnico do
  backlog, não a "várias coisas ao mesmo tempo".
- Depois de mesclada, a branch `feature/` é apagada.

### Nomenclatura de branches

```
feature/engine-c-menu-principal
feature/rules-hs-pontuacao-dia12
fix/engine-c-crash-menu
docs/atualiza-readme
```

Na prática o prefixo acompanha o tipo do commit: `feature/` (ou `feat/`) para funcionalidades, `fix/` para correções, `docs/` para documentação, `test/` para testes. Use letras minúsculas e hífens, sem espaços nem acentos.

## 2. Padrões de commits

Baseada em [Conventional Commits](https://www.conventionalcommits.org/) /
[padrões de commit do iuricode](https://github.com/iuricode/padroes-de-commits), com escopo:

```
tipo: descrição curta no imperativo

corpo opcional explicando o quê e por quê
```

### Tipos usados no projeto

| Tipo | Quando usar |
|---|---|
| `feat` | Nova funcionalidade do jogo |
| `fix` | Correção de bug |
| `docs` | Documentação (README, docs/, comentários de escopo) |
| `test` | Criação ou ajuste de testes |
| `refactor` | Reestruturação de código sem mudar comportamento |
| `style` | Formatação, indentação, sem mudança de lógica |
| `chore` | Tarefas de manutenção, configs, dependências |
| `build` | Mudanças no Makefile, Stack/Cabal, scripts de build |
| `ci` | Mudanças nos workflows do GitHub Actions |
| `remove` | Exclusão de arquivos, código ou funcionalidades obsoletas |

### Exemplos


- feat: adiciona menu principal navegável

- fix: corrige cálculo de Pontuação_Dia no cenário de empate

- docs: adiciona matriz RACI da squad

- ci: adiciona build automático no push


### Regras práticas

- Título no imperativo ("adiciona", não "adicionado"/"adicionando").
- Um commit = uma mudança logicamente coesa. Evite commits tipo "várias
  correções" misturando escopos diferentes.

### Validação automática

O repositório tem um hook `commit-msg` em `.githooks/` que recusa o commit se
a primeira linha da mensagem não seguir o padrão acima. Ative uma vez, logo
depois de clonar:

```bash
git config core.hooksPath .githooks
```

A partir daí, todo `git commit` com mensagem fora do padrão é bloqueado
localmente, antes mesmo do push.

## 3. Processo de Pull Request

1. Abra a branch `feature/...` a partir de `develop` atualizada.
2. Ao terminar, abra um PR de `feature/...` para `develop` (nunca para
   `main`).
3. Título do PR segue a mesma convenção do commit (ex:
   `feat: adiciona menu principal navegável`).
4. Descreva no corpo do PR: o que mudou, como testar, o que ficou fora.
5. Pelo menos **1 aprovação** de outro integrante antes do merge.
O PR é o único jeito de uma mudança chegar à `develop`. O hook de commits não vale para o PR: confira o título à mão.

## 4. Issues e bug tracker

Todo problema encontrado vira uma issue no repositório, para a equipe ver o que está aberto e quem cuida do quê.

### Abrir uma issue

- **Título:** `BUG-NN — o que acontece` para bugs (o número segue o último do tracker) ou uma frase curta no imperativo para melhorias.
- **Corpo:** o que acontecia, como reproduzir (passos curtos), o que se esperava e onde fica o código, quando já se sabe.
- **Labels:** `bug` para defeitos, `enhancement` para melhorias, `documentation` para texto, `accessibility` para barreiras de acessibilidade.
- Um problema por issue. Se o bug aparece em duas telas por causas diferentes, são duas issues.

### Bug tracker

Os bugs também ficam em [docs/fds/testes/bugs.md](docs/fds/testes/bugs.md), com ID, gravidade, link da issue e status. Ao abrir ou corrigir um bug, atualize a tabela e o bloco do bug no mesmo PR da correção.

### Fechar a issue

1. O PR da correção cita a issue no corpo (`Closes #N`).
2. **Atenção:** o GitHub só fecha a issue sozinho quando o PR vai para a branch padrão (`main`). Como os PRs vão para a `develop`, feche a issue à mão assim que o PR for mesclado, com um comentário apontando o PR.
3. Atualize o status no `bugs.md` (por exemplo, "Corrigido na `develop`, issue fechada").

### Ligação com o Jira

Cada item do Jira (história ou subtarefa) corresponde a uma branch. Cite a chave no corpo do PR (por exemplo `PI2-231`), e mova o card para **Concluído** quando o trabalho estiver pronto.

## 5. Definição de pronto

Um item (história, subtarefa ou bug) está pronto quando:

1. O código compila sem erro com o comando da seção 0.
2. A suíte de testes passa inteira, e há teste novo para o que foi criado ou corrigido.
3. Docs afetadas estão atualizadas (README, `docs/`, e o `bugs.md` quando for bug).
4. O PR para a `develop` foi aprovado por outra pessoa da equipe e mesclado.
5. A issue ligada foi fechada (à mão, veja a seção 4) e o card do Jira está em **Concluído**.

### Referência ao Jira nos commits e PRs

O hook de commits exige só `tipo: descrição` na primeira linha, então a chave do Jira vai no **corpo** do commit ou do PR:

```
feat: adiciona troca suave de faixa entre telas

PI2-260
```

No título do PR segue valendo a mesma convenção do commit, sem a chave.
