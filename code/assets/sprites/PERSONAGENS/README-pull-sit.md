# Animação de puxar a cadeira e sentar

Cada personagem tem uma sequência, escolhida pelo lado para o qual a cadeira está voltada.

| Personagem | Pasta | Direção ao sentar | Pés no quadro 0 | Base final da cadeira | Quadro final |
| --- | --- | --- | --- | --- | --- |
| Estagiário | `Estagiarios/pull-sit-nort/` | Norte | `(0, 17)` | `(0, 4)` | 25 |
| Anderson, Julio, JV, Laura, Lucas, Nicollas, Vasco e Zito | `pull-sit-south/` na pasta de cada NPC | Sul | `(0, -17)` | `(0, 0)` | 29 |

## Quadros

Os arquivos são PNGs RGBA de 80 × 72 px, numerados a partir de `frame_000.png`. O personagem e a cadeira já estão compostos em cada quadro. Use 85 ms por quadro e mantenha o último enquanto o personagem estiver sentado.

- **0–3**. Aproximação com os sprites de caminhada.
- **4–8**. Deslocamento da cadeira. No sul, as mãos acompanham o encosto. No norte, os braços conservam a pose original.
- **9–19**. O personagem contorna a cadeira e vira.
- **20–25**. O personagem recua, flexiona as pernas e se acomoda. Nos quadros 22–25 do estagiário, a cadeira fica à frente do corpo, com cerca de metade do cabelo visível acima do encosto.
- **26–29, NPCs**. O personagem e a cadeira avançam juntos até a posição inicial da cadeira. A transparência a partir da linha 46 do quadro permite que o tampo da mesa encubra as pernas e a base da cadeira.

## Alinhamento no jogo

Na campanha, pressione **Espaço** perto do computador. O estagiário anda até o ponto de entrada da cadeira, senta voltado para o norte e abre o computador. Ao sair da tarefa, a sequência é reproduzida em sentido inverso para devolver o controle. Os sete NPCs caminham até as cadeiras do dia e se sentam voltados para o sul. No Dia 1, Laura, JV e Vasco mantêm as posições da recepção até a cena terminar.

O pixel `(40, 49)` de cada quadro corresponde à posição inicial da base da cadeira. Aplique à animação a mesma escala de desenho do personagem. Os deslocamentos na tabela são relativos à base inicial da cadeira, antes de aplicar a escala.

A aproximação usa a caminhada do personagem, inclusive ao chegar pelo leste. A direção final segue a cadeira. O primeiro quadro usa a pose inicial de caminhada nessa direção.

A âncora fica fixa durante a sequência. A sobreposição de piso em `environment/office-chair-floor.png` cobre as cadeiras pintadas em `office.png`. As cadeiras avulsas aparecem durante a aproximação e dão lugar à cadeira animada. Nas mesas dos NPCs, a borda do tampo coincide com a linha 46 do quadro, três pixels acima da âncora antes da escala. O jogo recorta o desenho nessa linha para que o tampo cubra as pernas.

