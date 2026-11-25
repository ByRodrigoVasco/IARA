# Arquivo

Material que o jogo **não usa** e que **não entra no executável** nem no pacote de distribuição
(`tools/release/build_release.ps1` só empacota `code/assets` e `docs/assets/team`).

| Pasta | O que é |
|---|---|
| `originais/sprites/` | personagens em pixel art de 32x32 e Colegas de Plantão antigos |
| `originais/docs/` | sprites da equipe e folhas de caminhada antigos, que apareciam na documentação |
| `originais/audio/sobrevivencia/` | músicas e ambientes em WAV; o jogo usa as versões OGG de `code/assets/audio` |
| `originais/audio/campanha/` | músicas em MP3 na qualidade original |

Para voltar um arquivo, copie-o para o lugar de origem (a estrutura de pastas é a mesma).

**Atenção:** `animar_caminhada.py` e os `gerar_*.py` de `tools/sprites` são da arte antiga. Rodá-los
grava quadros de 32x32 por cima dos sprites em alta resolução.
