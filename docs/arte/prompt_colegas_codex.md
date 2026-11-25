Você vai criar as folhas de conceito dos 4 Colegas de Plantão do jogo IARA (terror de escritório, pixel art, raylib). Eles são os aliados do poder "Colega de Plantão" no modo sobrevivência.

## Onde trabalhar
Pasta: `C:\Users\Rodrigo\Desktop\Git\GitHub\Projeto 2 - CESAR School\IARA\.claude\worktrees\analise-sprites-branch-c18790`
Salve SOMENTE em `docs/arte/conceito/colegas/`: `seguranca.png`, `suporte.png`, `enfermeira.png`, `entregador.png`.
Não altere código, `code/assets` nem outros arquivos. Não troque de branch, não faça commit nem push.

## Referências (olhe antes de desenhar)
- ESTILO (obrigatório): as folhas dos personagens já aprovados, em `docs/arte/conceito/` (principalmente `anderson.png`, `jv.png`, `estagiarios.png` e `laura.png`). Os colegas têm que parecer do mesmo jogo e do mesmo artista: mesma proporção de cabeça e corpo, mesma espessura de contorno escuro, mesmos 3 a 4 tons por material, mesma direção de luz e mesmo nível de detalhe.
- IDENTIDADE: `C:\Users\Rodrigo\Desktop\Git\GitHub\Projeto 2 - CESAR School\IARA\docs\arte\referencias\colegas_de_plantao.png` (arte antiga e simples; use só para as cores e a ideia).

São personagens novos e fictícios: NÃO copie o rosto, o cabelo ou o corpo de nenhum personagem da equipe com outra cor. Cada um precisa de desenho próprio.

## Os quatro
1. `seguranca`: homem robusto, pele morena, cabelo preto curto, uniforme azul completo (camisa de botão de manga curta, gravata escura, cinto com rádio e lanterna), quepe azul com emblema, distintivo dourado, calça azul-marinho, coturno preto.
2. `suporte`: rapaz magro, cabelo castanho bagunçado, suéter verde, headset com microfone, crachá, calça jeans, tênis.
3. `enfermeira`: mulher adulta, cabelo castanho-acobreado preso em coque, jaleco branco abotoado até o joelho, touca branca com cruz vermelha, cruz no peito, calça azul-clara, sapato branco.
4. `entregador`: homem jovem, pele escura, cabelo curto, jaqueta laranja com faixa refletiva, boné laranja, mochila térmica quadrada de entrega nas costas (bem visível de lado e de costas), calça escura, tênis.

Silhueta e cor principal diferentes entre si, para reconhecer cada um num relance, pequeno e em chão escuro. Vermelho vivo só na cruz da enfermeira.

## Formato de cada folha
- PNG RGBA com fundo 100% transparente (alfa 0), sem sombra no chão, sem halo colorido, sem texto, sem grade e sem moldura.
- Grade de 4 linhas x 4 colunas = 16 figuras inteiras:
  - linha 1: andando de FRENTE;
  - linha 2: andando de LADO, virado para a DIREITA;
  - linha 3: andando de COSTAS;
  - linha 4: GOLPE de lado, virado para a DIREITA: (a) arma o soco, cotovelo para trás; (b) avança com o braço saindo; (c) impacto, braço esticado e corpo inclinado à frente; (d) volta.
- Figuras bem separadas: pelo menos 40 px transparentes entre elas. Nada pode encostar, sair da imagem ou ficar solto da figura (sem faíscas, riscos de movimento ou objetos flutuando).
- Mesma escala nas 16 figuras, cerca de 185 px de altura, pés alinhados na mesma linha em cada fileira. Folha de aproximadamente 1040 x 1000.

## Animação (o ponto mais importante)
- Colunas das linhas 1 a 3: 1 = parado, 2 = passo com uma perna, 3 = parado (igual à coluna 1), 4 = passo com a OUTRA perna.
- De frente e de costas, as colunas 2 e 4 levantam pernas OPOSTAS. Confira isso em cada um.
- De lado, os dois passos mostram as pernas abertas de forma legível, com os braços em contrabalanço.
- Cabeça, cabelo, rosto, acessórios e roupa IDÊNTICOS entre os quadros da mesma linha; só mudam pernas, braços e a subida do corpo no passo. Nada pode tremer, mudar de forma ou de cor (atenção a sapatos e óculos).
- No golpe, o braço faz parte do corpo, com ombro, manga, cotovelo e mão desenhados. Nunca uma barra colada por cima do tronco, e nunca um terceiro braço.

## Processo
1. Faça primeiro SÓ o `seguranca`, me mostre ampliado e espere a minha aprovação. Ele vira o gabarito dos outros três.
2. Antes de mostrar, revise quadro a quadro: recorte limpo, sem rebarbas, furos ou pedaços cortados; pernas alternando; cabeça estável; lado e golpe virados para a direita. Se algo falhar, refaça.
3. Se a geração de imagem não respeitar a grade ou a consistência, corrija por script (recortar, alinhar, remover halo) ou gere de novo. Se não conseguir gerar imagens, diga e pare.
4. Ao final, diga o que ficou bom, o que não ficou e quais arquivos salvou.
