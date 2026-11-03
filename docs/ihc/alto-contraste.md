# Modo de alto contraste (US19)

Registro do PI2-256 e das subtarefas PI2-274 a PI2-277.

## Decisão a confirmar com a equipe (PI2-274)

O PI2-274 pede que a equipe escolha entre paleta alternativa fixa e filtro em tempo real.
A implementação usa o **filtro em tempo real**, como proposta, e **a decisão ainda precisa ser confirmada**.

| Critério | Filtro em tempo real | Paleta fixa |
|---|---|---|
| Trabalho de arte | Nenhum | Refazer os sprites e cenários |
| Telas futuras | Já funcionam | Cada uma precisa de arte própria |
| Reinício do jogo | Não precisa | Não precisa, mas exige recarregar texturas |
| Controle fino da aparência | Menor | Maior |

## Como funciona

- **Ativação:** caixa **Alto contraste** na tela `CONFIGURAÇÕES` do menu, com efeito imediato. A preferência fica no arquivo `iara.ini`, junto com o brilho e a tela cheia, e volta ao abrir o jogo.
- **Filtro:** aumenta o contraste de luminância em todas as telas ([`accessibility.c`](../../code/src/ui/accessibility.c)).
- **Forma, não só cor:** no escritório, cada tipo de elemento ganha um contorno próprio.

| Elemento | Contorno |
|---|---|
| Objeto interativo (computador, mural) | Linha dupla, clara e escura |
| Vidro (salas e porta) | Linha tracejada |
| Outros obstáculos | Linha fina contínua |

No quiz, acerto e erro já têm ícones diferentes (✓ e ✗) além da cor.

## Teste com daltonismo simulado (PI2-277)

O CT-40 desenha o escritório com o modo ligado, aplica as matrizes de protanopia e deuteranopia
de Machado et al. (2009) e exige razão de contraste de pelo menos 3:1 nos contornos do computador,
do mural e do vidro. O quadro simulado fica em
[`alto-contraste-deuteranopia.png`](../fds/testes/evidencias/alto-contraste-deuteranopia.png).

## Limites

- **NPCs:** ainda não existem no jogo, então esse critério de aceite não pôde ser verificado.
- **Pessoas reais:** o teste é uma simulação; falta avaliar com pessoas com daltonismo.
- **Cenas escuras:** o filtro escurece as sombras da arte do menu. Os textos seguem legíveis, mas detalhes escuros do fundo somem.
