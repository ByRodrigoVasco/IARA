#ifndef SV_AUDIO_H
#define SV_AUDIO_H

#include <stdbool.h>

/*
 * Áudio do Modo Sobrevivência, com as regras de mistura aprovadas no catálogo:
 *  - cada som tem uma faixa de prioridade (alerta > ataque > evento > menu > poder > inimigo > frequente > discreto);
 *  - cada som tem intervalo mínimo e número máximo de vozes;
 *  - sons pequenos somam no máximo 10 por segundo;
 *  - um alerta, ataque de chefe ou evento abaixa a trilha, o ambiente e os sons pequenos por um instante.
 * Os arquivos já foram masterizados em volumes relativos, então o jogo toca todos com volume 1.
 */

#define SV_AUDIO_PATH "code/assets/audio/"

typedef enum { TIER_ALERTA, TIER_ATAQUE, TIER_EVENTO, TIER_UI, TIER_PODER, TIER_INIMIGO, TIER_FREQUENTE, TIER_MICRO, TIER_PASSO } SvTier;

// nome, faixa, intervalo mínimo (s), vozes
#define SV_SFX_LIST(X)                                                                                                                     \
    X(dano, TIER_ALERTA, 0.25f, 1) X(morte, TIER_ALERTA, 1.0f, 1) X(chefeSurge, TIER_ALERTA, 1.0f, 1) X(chefeMorre, TIER_ALERTA, 1.0f, 1) \
    X(diegoPorta, TIER_ALERTA, 0.5f, 1) X(impressoraJustaCausa, TIER_ALERTA, 0.5f, 1) X(reneDecisao, TIER_ALERTA, 0.5f, 1)                \
    X(reneEscudoCai, TIER_ALERTA, 0.5f, 1) X(iaraOnda, TIER_ALERTA, 0.5f, 1) X(iaraCanto, TIER_ALERTA, 0.5f, 1)                          \
    X(iaraMentira, TIER_ALERTA, 0.5f, 1) X(horaExtra, TIER_ALERTA, 1.0f, 1) X(afogou, TIER_ALERTA, 1.0f, 1)                              \
    X(impressoraLeque, TIER_ATAQUE, 0.25f, 2) X(impressoraPapelada, TIER_ATAQUE, 0.25f, 2) X(modeloMarca, TIER_ATAQUE, 0.25f, 2)         \
    X(modeloExplode, TIER_ATAQUE, 0.25f, 2) X(modeloOlhar, TIER_ATAQUE, 0.25f, 2) X(modeloClones, TIER_ATAQUE, 0.25f, 2)                \
    X(diegoNegado, TIER_ATAQUE, 0.25f, 2) X(diegoGarras, TIER_ATAQUE, 0.25f, 2) X(diegoFantasmas, TIER_ATAQUE, 0.25f, 2)                \
    X(reneCarimbo, TIER_ATAQUE, 0.25f, 2) X(reneCorrente, TIER_ATAQUE, 0.25f, 2) X(reneSombra, TIER_ATAQUE, 0.25f, 2)                   \
    X(iaraMaos, TIER_ATAQUE, 0.25f, 2) X(iaraRiso, TIER_ATAQUE, 0.25f, 2)                                                                \
    X(subirNivel, TIER_EVENTO, 0.25f, 1) X(bau, TIER_EVENTO, 0.25f, 1) X(evolucao, TIER_EVENTO, 0.25f, 1) X(novoAto, TIER_EVENTO, 0.25f, 1) \
    X(compra, TIER_EVENTO, 0.25f, 1) X(marmita, TIER_EVENTO, 0.25f, 1) X(aguaSobe, TIER_EVENTO, 0.25f, 1)                                \
    X(novaconversa, TIER_EVENTO, 0.25f, 1) X(prompt, TIER_EVENTO, 0.25f, 1)                                                              \
    X(confirma, TIER_UI, 0.1f, 1) X(semSaldo, TIER_UI, 0.1f, 1) X(escolha, TIER_UI, 0.06f, 1)                                           \
    X(ponteiro, TIER_PODER, 0.25f, 2) X(caneca, TIER_PODER, 0.25f, 2) X(deploy, TIER_PODER, 0.25f, 2) X(tarja, TIER_PODER, 0.25f, 2)   \
    X(modus, TIER_PODER, 0.25f, 2) X(ecoSussurro, TIER_PODER, 0.25f, 2) X(crachaAtravessa, TIER_PODER, 0.25f, 2)                        \
    X(auditorPassos, TIER_PODER, 0.25f, 2) X(glitch, TIER_PODER, 0.25f, 2) X(coletaMoeda, TIER_PODER, 0.25f, 2)                         \
    X(notificacaoPing, TIER_INIMIGO, 0.5f, 1) X(esvaziadoGeme, TIER_INIMIGO, 0.25f, 2) X(dadoQuica, TIER_INIMIGO, 0.4f, 1)             \
    X(alucinacaoSome, TIER_INIMIGO, 0.25f, 2) X(autoRespostaDivide, TIER_INIMIGO, 0.25f, 2) X(afogadoCospe, TIER_INIMIGO, 0.25f, 2)    \
    X(grampeador, TIER_FREQUENTE, 0.12f, 2) X(naoperturbe, TIER_FREQUENTE, 1.0f, 1) X(tabela, TIER_FREQUENTE, 0.25f, 2)                 \
    X(colega, TIER_FREQUENTE, 0.25f, 2) X(postits, TIER_FREQUENTE, 0.4f, 1) X(chamadoMorre, TIER_FREQUENTE, 0.3f, 1)                    \
    X(coletaXp, TIER_FREQUENTE, 0.12f, 2) X(acertaInimigo, TIER_MICRO, 0.14f, 2) X(novoDia, TIER_MICRO, 0.25f, 2) X(passo, TIER_PASSO, 0.3f, 1)

typedef enum
{
#define X(name, tier, gap, voices) SND_##name,
    SV_SFX_LIST(X)
#undef X
    SND_COUNT
} SvSound;

// Trilhas. A ordem acompanha os nomes em sv_audio.c.
typedef enum
{
    MUS_MENU, MUS_LOJA, MUS_GAMEOVER, MUS_VITORIA,
    MUS_ATO1, MUS_ATO2, MUS_ATO3, MUS_ATO4, MUS_ATO5,
    MUS_IMPRESSORA, MUS_MODELO, MUS_DIEGO, MUS_RENE, MUS_IARA, MUS_HORAEXTRA,
    MUS_COUNT, MUS_NONE = -1
} SvMusic;

// Ambientes por área (índice do mapa).
#define SV_AMBIENT_COUNT 13

// Desliga o áudio de vez (os testes automáticos não devem tocar nada no alto-falante).
void sv_audio_disable(bool disabled);
void sv_audio_init(void);

/** @brief Abre o áudio e carrega o próximo som a cada chamada. Devolve o andamento, de 0 a 1. Para a tela de carregamento. */
float sv_audio_init_step(void);
void sv_audio_close(void);
bool sv_audio_ready(void);

void sv_sfx(SvSound sound);

// Troca a trilha (com entrada suave). MUS_NONE silencia.
void sv_music_play(SvMusic music);
SvMusic sv_music_current(void);

// Troca o ambiente. -1 silencia.
void sv_ambient_play(int ambient);

// Chamar uma vez por quadro: alimenta os streams e aplica o abaixamento.
void sv_audio_update(float dt);

// Quantos sons pequenos foram tocados no último segundo (para os testes e a depuração).
int sv_audio_small_per_second(void);

#endif // SV_AUDIO_H
