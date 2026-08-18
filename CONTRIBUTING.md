# Guia de Contribuição - Projeto IARA

Este documento é para definir como deve ser o trabalho no repositório. Fluxo de branches, padrão de commits e processo de Pull Request.

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