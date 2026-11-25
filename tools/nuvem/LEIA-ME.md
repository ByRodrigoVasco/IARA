# Nuvem do IARA (saves e rankings compartilhados)

Em um evento com vários notebooks, os saves (campanha, Sobrevivência e dupla) e os rankings (solo e
dupla) ficam em um projeto do Supabase. O `iara.ini` e o `iara-desempenho.log` continuam só locais.

O banco do projeto "IARA" (plano gratuito, South America - São Paulo) **já está montado**. Os passos
abaixo servem para montar outro do zero.

## 1. Criar o projeto

Em <https://supabase.com>, crie um projeto no plano gratuito, na região **South America (São Paulo)**.

## 2. Criar as tabelas e funções

No **SQL Editor**, cole o conteúdo inteiro de [supabase.sql](supabase.sql) e clique em **Run**. O
arquivo é idempotente: pode ser rodado de novo depois de uma mudança.

## 3. Definir a senha de apagar saves

Apagar um save pede a senha da organização, conferida no servidor. Só o hash SHA-256 é guardado.
Rode no SQL Editor, trocando `SUA-SENHA`, e **não salve** essa query nem a coloque no repositório:

```sql
insert into iara_config (chave, valor)
values ('senha_apagar', encode(sha256(convert_to('SUA-SENHA', 'UTF8')), 'hex'))
on conflict (chave) do update set valor = excluded.valor;
```

Rodar de novo troca a senha. Sem a senha definida, nenhum save pode ser apagado.

## 4. Configurar o build

Crie o arquivo `tools/nuvem/nuvem.local` (ignorado pelo git) com duas linhas:

```
NUVEM_URL=https://<id-do-projeto>.supabase.co
NUVEM_CHAVE=<chave publishable>
```

- A URL está em **Project Settings > Data API** (Project URL).
- A chave está em **Project Settings > API Keys**: use a **publishable key** (`sb_publishable_...`).
  Ela não é um JWT, e o jogo a envia só no cabeçalho `apikey`. **Nunca** use a chave secreta
  (`sb_secret_...` ou `service_role`).

O arquivo é local: cada cópia do repositório (ou worktree) que for gerar o executável precisa do seu.

O `tools/release/build_release.ps1` lê esse arquivo e embute os dois valores no `iara.exe`. Sem o
arquivo, o executável funciona só local. Não coloque os valores em commits, logs ou testes.

## 5. Testar o banco

Rode esta query no SQL Editor. Ela cria dados de teste; todas as colunas devem dar `true`:

```sql
select
  iara_criar_save('sobrevivencia', 'zz teste')                                   = 'ok'       as criar,
  iara_criar_save('sobrevivencia', 'ZZ TESTE')                                   = 'existe'   as repetido,
  (select status from iara_abrir_save('sobrevivencia', 'zz teste', 'pc-a'))      = 'ok'       as abrir,
  (select status from iara_abrir_save('sobrevivencia', 'zz teste', 'pc-b'))      = 'em_uso'   as trava,
  (select status from iara_gravar_save('sobrevivencia', 'zz teste', 'pc-a', 'x', 0)) = 'ok'   as gravar,
  (select status from iara_gravar_save('sobrevivencia', 'zz teste', 'pc-a', 'y', 0)) = 'conflito' as conflito,
  iara_liberar_save('sobrevivencia', 'zz teste', 'pc-a')                         = 'ok'       as liberar,
  (select status from iara_abrir_save('sobrevivencia', 'zz teste', 'pc-b'))      = 'ok'       as abrir_em_outro,
  iara_registrar_partida('solo', 'zz teste', 300, 900, 10, 2, 0, false)          > 0          as partida_valida,
  iara_registrar_partida('solo', 'zz teste', 9999, 900, 10, 2, 0, false)         = 0          as partida_impossivel,
  iara_apagar_save('sobrevivencia', 'zz teste', 'pc-b', 'senha errada')          = 'senha'    as senha_errada;
```

O ranking é conferido em uma **query separada**: `iara_ler_ranking` é `stable` e não enxerga o que
foi escrito na mesma query (daria `NULL`). Deve dar `true`:

```sql
select (select pontos from iara_ler_ranking('solo', 10) where nome = 'zz teste') = 900 * 10 + 300 * 5 + 2 * 500;
```

Depois apague os dados de teste:

```sql
delete from iara_saves where nome_chave = 'zz teste';
delete from iara_ranking where nome_chave = 'zz teste';
```

## Como o jogo se comporta

| Situação | O que acontece |
|---|---|
| Nos menus | No canto de cima, bolinha verde "online" ou vermelha "offline"; o motivo da queda vai para o `iara-desempenho.log` |
| Escolher o save | A lista vem da nuvem e é atualizada a cada 10 s; um save aberto em outro notebook aparece como "Em uso em outro notebook" |
| Sem conexão na escolha do save | Não dá para abrir, criar nem apagar; a tela oferece "Tentar de novo" |
| A rede cai durante a partida | O jogo segue; a gravação fica na fila e sobe quando a rede voltar |
| Fechar o jogo sem rede | A fila fica no disco e sobe na próxima abertura |
| Outro notebook assumiu o save, ou a nuvem tem gravação mais nova | O jogo avisa e para de subir aquele save; a gravação recusada fica em `nuvem\conflito-*.txt` |
| Modo normal (campanha) | Sobe quando o jogo grava: ao voltar ao menu e ao fechar |
| Partida sem conexão | Vai para a fila do ranking; a tabela mostra o ranking deste computador, marcada como OFFLINE |
| Partida com cheat | Não vai para o ranking |
| Saves que já existiam no computador | Não aparecem na lista da nuvem. Se um save da nuvem tiver o mesmo nome, o progresso local vai para `<arquivo>.anterior` |

A pasta `Documentos\IARA\nuvem` guarda o código deste notebook (`notebook.id`) e a fila. **Não copie
essa pasta de um notebook para outro**: dois notebooks com o mesmo código conseguem abrir o mesmo save
ao mesmo tempo.

## Regras do servidor

| Regra | Valor |
|---|---|
| Nome do save | `[A-Za-z0-9 _-]{1,20}`, único por modo, sem diferenciar maiúsculas |
| Trava de um save aberto | vence em 2 min; o jogo renova a cada 30 s |
| Gravação mais velha que a do servidor | recusada (`conflito`) |
| Conteúdo de um save | até 65536 caracteres |
| Partida impossível (mais de 600 s, mais de 5 chefes, abates demais) | recusada (posição 0) |
| Apagar save | exige a senha e também tira o jogador do ranking |
| Personagem no ranking | solo: o número do personagem; dupla: J1 + 16 × (J2 + 1) |

Os pesos da pontuação (`iara_pontos`) são os de `code/src/survival/sv_ranking.h`: ao mudar um,
mude o outro e rode o `supabase.sql` de novo.

O anon não lê as tabelas direto (`permission denied`): ele só executa as funções.

## Antes do evento

O plano gratuito **pausa o projeto após cerca de 7 dias sem atividade**. Nos dias anteriores, abra o
ranking no jogo (ou use o jogo) para mantê-lo ativo, e confira no painel que ele não está pausado.
