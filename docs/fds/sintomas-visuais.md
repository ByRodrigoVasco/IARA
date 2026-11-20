# Sintomas visuais das estatísticas ocultas (US10, RF11)

> Como Lívia, quero perceber o estado do jogo por sintomas visuais e narrativos como mudanças de cenário, em vez de números na tela, para viver a tensão da história sem ficar calculando estatísticas.

O jogador nunca vê os campos de `hidden_statistic`. Cada campo, ao passar de um limiar,
muda a iluminação, acende algo no fundo ou põe um elemento no mapa do escritório.
Isso se soma ao sintoma que já existia, o do avanço da campanha: o escritório escurece
e alaga conforme o **dia** (GDD 6.1). Os limiares abaixo fazem o cenário reagir também às
**escolhas** do jogador.

## Regra dos limiares

- **Forte** é o mesmo limiar que decide o final (`AUTOMATION_HIGH_TOTAL`, `PERCEPTION_HIGH_TOTAL` e `HUMAN_BOND_HIGH_TOTAL`, em `core/game_state.h`). O que o jogador vê avisa para qual final ele caminha.
- **Leve** fica na metade desse caminho, arredondada para cima.
- Os valores contam só o que veio depois de `HIDDEN_STATISTIC_START`, como em `get_game_ending()`.
- Se a equipe mudar um limiar de final, o sintoma acompanha sozinho, porque é calculado a partir dele.

## Tabela

| Campo | Leve | Forte | O que muda no cenário |
|---|---|---|---|
| `current_automation_points` (Automação) | 3 | 5 (final Pamonha) | **Leve:** um monitor de cada grupo de mesas acende sozinho, em azul: a IARA trabalhando nas mesas vazias. **Forte:** todos os monitores, a sala de servidores pulsando e a luz falhando de vez em quando. A luz do escritório esfria (menos vermelho) aos poucos, desde o primeiro ponto. |
| `current_perception_points` (Percepção) | 2 | 4 | **Leve:** um X vermelho sobre o ✓ de "PESSOAS" no quadro de valores. **Forte:** também sobre o "DADOS", os bilhetes do mural ligados por linha vermelha e uma luz vermelha por baixo da porta do corredor. |
| `current_human_bond_points` (Vínculo) | 1 | 2 | **Leve:** luz quente na área do sofá. **Forte:** também na copa e no bebedouro. A luz fica mais forte aos poucos, conforme o vínculo cresce. |
| `current_overall_points` (Desempenho) | média abaixo de 10 por dia jogável | média abaixo de 5 por dia jogável | **Leve:** 3 papéis de pendência no chão, perto da mesa do jogador. **Forte:** 7 papéis e a tela da impressora piscando em vermelho. |

O desempenho funciona ao contrário dos outros: o sintoma aparece quando a média **cai**.
Ele usa a média por dia jogável concluído (`symptoms_count_concluded_playable_days()`),
porque os pontos só crescem e um valor absoluto não diria nada sobre o momento do jogador.
Limitação: o Dia 1 tem 5 chamados e os outros têm 1, então quem vai bem no Dia 1 demora
a mostrar o sintoma se piorar depois.

## Quando muda

As estatísticas entram no relatório do dia, no computador. Ao voltar ao escritório, o
jogador já vê o cenário novo: a consequência aparece logo depois da escolha.

## Acessibilidade

- **Fotossensibilidade:** a falha da luz é curta (0,12 s), escurece no máximo 22% e acontece duas vezes a cada 3,7 s, bem abaixo do limite de 3 flashes por segundo. A impressora pisca 1 vez por segundo.
- **Não só cor:** cada sintoma tem forma própria (X no quadro, linhas no mural, papéis no chão), então continua legível no modo de alto contraste.

## Código e testes

| O quê | Onde |
|---|---|
| Limiares e cálculo dos níveis, sem raylib | `code/src/game/symptoms.h` e `symptoms.c` |
| Desenho no mapa (posições em pixels do `office.png`) | `code/src/ui/office_symptoms.c` |
| Chamada no escritório | `render_game_loop_escritorio()` em `code/src/screens/game_loop.c` |
| Testes de unidade | `code/tests/test_symptoms.c` (entra no `run_tests.ps1`) |

`test_symptoms` confere que o nível forte bate com o limiar de cada final, os limites
exatos de cada nível, a média do desempenho e que calcular os sintomas não muda nenhum ponto.

## A validar com a equipe

- Os limiares leves e os elementos escolhidos são uma proposta. O GDD só diz que o estado aparece por sintomas, sem HUD.
- O limiar do Vínculo herda a proposta de 2 conversas, que ainda não foi decidida (RF13).
- Faltam os sintomas no personagem (olheiras e tremor), que continuam pendentes no RF11.
