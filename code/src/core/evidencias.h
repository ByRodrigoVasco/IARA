#ifndef EVIDENCIAS_H
#define EVIDENCIAS_H

// Evidências reunidas para a prova do Dia 100.
// Ficam num header próprio porque os dois jogos leem as mesmas evidências: o jogo
// gráfico, por game_state.h, e o jogo de terminal, que não pode incluir raylib.
#define EVIDENCE_NO_RESIGNATION_LETTER (1u << 0) // Dia 12: pediu a revisão da conciliação
#define EVIDENCE_BADGE_AFTER_LEAVING (1u << 1)   // Dia 58: examinou o registro da catraca
#define EVIDENCE_NO_HUMAN_ORDER (1u << 2)        // Dia 74: leu o bilhete da auditoria

#endif // EVIDENCIAS_H
