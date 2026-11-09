#ifndef COMPUTER_DAYS_H
#define COMPUTER_DAYS_H

// Roteiro dos dias jogáveis depois do Dia 1. Só computer_loop.c inclui este arquivo, porque ele
// depende dos tipos do roteiro. Os gabaritos não ficam escritos aqui: saem das fórmulas de cada mesa.
// O GDD define só o conceito lógico de cada dia (6.3): enunciados e nomes são proposta, a revisar com LMC.

#define L_VAR(index) (&(const LogicExpression){.operator = LOGIC_VARIABLE, .variable = (index)})
#define L_NOT(a) (&(const LogicExpression){.operator = LOGIC_NOT, .left = (a)})
#define L_AND(a, b) (&(const LogicExpression){.operator = LOGIC_AND, .left = (a), .right = (b)})
#define L_OR(a, b) (&(const LogicExpression){.operator = LOGIC_OR, .left = (a), .right = (b)})
#define L_IMPLIES(a, b) (&(const LogicExpression){.operator = LOGIC_IMPLIES, .left = (a), .right = (b)})
#define L_IFF(a, b) (&(const LogicExpression){.operator = LOGIC_IFF, .left = (a), .right = (b)})

#define CHOSEN(row) (1u << (row))

// ---------------------------------------------------------------------------
// DIA 3: conectivos e tabela-verdade
// ---------------------------------------------------------------------------
enum { D3_DOWN, D3_BACKUP, D3_INVASION };

static const TriageCard day_3_cards[] = {
    {.id = "#21", .text = "portal da loja", .values = CHOSEN(D3_DOWN), .iara_choice = 1, .iara_low_choice = 1},
    {.id = "#22", .text = "site antigo", .values = CHOSEN(D3_DOWN) | CHOSEN(D3_BACKUP), .iara_choice = 0, .iara_low_choice = 1},
    {.id = "#23", .text = "e-mail suspeito", .values = CHOSEN(D3_BACKUP) | CHOSEN(D3_INVASION), .iara_choice = 1, .iara_low_choice = 1},
    {.id = "#24", .text = "impressora", .values = 0, .iara_choice = 0, .iara_low_choice = 1},
    {.id = "#25", .text = "folha de ponto", .values = CHOSEN(D3_DOWN) | CHOSEN(D3_BACKUP) | CHOSEN(D3_INVASION), .iara_choice = 1, .iara_low_choice = 1},
};

static DialogStep day_3_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"Bom dia, Estagiário. O Tiago pediu folga.", "Deixei a mesa dele aberta para você."},
        .line_count = 2,
        .button_name = "Abrir a mesa"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_TRUTH_TABLE,
        // A norma e a legenda das colunas vêm do bilhete que o Tiago deixou na mesa,
        // não de um texto do jogo explicando notação (PI2-214 e PI2-215).
        .lines = {
            "Bilhete do Tiago, preso no monitor dele:",
            "\"Norma 8: vai ao TI se (P ∧ ¬B) ∨ I.",
            "P parado, B tem backup, I invasão. V é sim.",
            "O que a norma não manda ao TI fica na fila.\""
        },
        .line_count = 4,
        .hints = {"Resolva o parêntese antes do ∨.", "Com I verdadeiro, o ∨ já basta."},
        .iara_confidence = 94,
        .iara_reason = "Apliquei a norma 8 linha por linha.",
        .low_reason = "A norma nova manda tudo ao TI.",
        .cards = day_3_cards,
        .card_count = 5,
        .norm = L_OR(L_AND(L_VAR(D3_DOWN), L_NOT(L_VAR(D3_BACKUP))), L_VAR(D3_INVASION)),
        .variable_names = "PBI",
        .variable_count = 3,
        .choice_labels = {"Fila", "TI"},
        .submit_label = "Enviar a triagem",
        .sender = "Mesa do Tiago",
        .wrong_result = "Mesa do Tiago: o portal ficou parado o dia inteiro."
    },
    {
        .type = STEP_CHOICE,
        .lines = {"De: Júlia, do RH.", "Vi você almoçando na mesa. Tem lugar com a", "gente, se quiser. O Tiago sumiu, é meio estranho."},
        .line_count = 3,
        .options = {"Almoçar com a Júlia", "Ficar e adiantar a fila"},
        .option_count = 2,
        .choice_effects = {DAY_EFFECT_HUMAN_BOND, DAY_EFFECT_NONE}
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 3:"}, .line_count = 1, .button_name = "Concluir"},
};

// ---------------------------------------------------------------------------
// DIA 12: De Morgan, dupla negação e tautologia
// ---------------------------------------------------------------------------
enum { D12_P, D12_Q };

static const TriageCard day_12_cards[] = {
    {.id = "A", .text = "A) ¬(P ∧ Q)  e  ¬P ∨ ¬Q", .left = L_NOT(L_AND(L_VAR(D12_P), L_VAR(D12_Q))),
     .right = L_OR(L_NOT(L_VAR(D12_P)), L_NOT(L_VAR(D12_Q))), .iara_choice = 1, .iara_low_choice = 0},
    {.id = "B", .text = "B) ¬(P ∨ Q)  e  ¬P ∨ ¬Q", .left = L_NOT(L_OR(L_VAR(D12_P), L_VAR(D12_Q))),
     .right = L_OR(L_NOT(L_VAR(D12_P)), L_NOT(L_VAR(D12_Q))), .iara_choice = 1, .iara_low_choice = 1},
    {.id = "C", .text = "C) ¬¬P  e  P", .left = L_NOT(L_NOT(L_VAR(D12_P))), .right = L_VAR(D12_P), .iara_choice = 1, .iara_low_choice = 1},
    {.id = "D", .text = "D) P ∨ ¬P  e  sempre verdade", .left = L_OR(L_VAR(D12_P), L_NOT(L_VAR(D12_P))), .right = NULL, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "E", .text = "E) P → Q  e  Q → P", .left = L_IMPLIES(L_VAR(D12_P), L_VAR(D12_Q)),
     .right = L_IMPLIES(L_VAR(D12_Q), L_VAR(D12_P)), .iara_choice = 0, .iara_low_choice = 1},
};

static DialogStep day_12_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"Reescrevi as regras da conciliação.", "Ficou mais curto. É igual, pode confiar."},
        .line_count = 2,
        .button_name = "Ver as regras"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_EQUIVALENCE,
        // A pergunta é da Sônia, e não uma instrução de método na voz do jogo (PI2-215).
        .lines = {
            "De: Sônia, do Financeiro. Conciliação do mês.",
            "A IARA encurtou as regras. Antes de eu assinar:",
            "cada par abaixo continua dizendo a mesma coisa?"
        },
        .line_count = 3,
        .hints = {"Teste P falso e Q verdadeiro no par B.", "Teste P verdadeiro e Q falso em cada par."},
        .iara_confidence = 96,
        .iara_reason = "Todas as regras simplificadas são iguais.",
        .low_reason = "Conferi com a norma 22. Nada mudou.",
        .cards = day_12_cards,
        .card_count = 5,
        .variable_count = 2,
        .choice_labels = {"Difere", "Igual"},
        .submit_label = "Enviar a conciliação",
        .sender = "Sônia",
        .wrong_result = "Sônia: a regra errada pagou uma rescisão sem carta."
    },
    {
        // A escolha que o GDD pede no Dia 12 (CA2): muda Automação ou Percepção
        .type = STEP_CHOICE,
        .lines = {"A IARA fechou o mês. Falta uma linha: rescisões", "pagas sem carta de demissão. Ela já explicou:", "\"É só um ajuste de sistema. Pode assinar.\""},
        .line_count = 3,
        .options = {"Aceitar e assinar", "Pedir revisão humana"},
        .option_count = 2,
        .choice_effects = {DAY_EFFECT_AUTOMATION, DAY_EFFECT_PERCEPTION | DAY_EFFECT_NO_LETTER}
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 12:"}, .line_count = 1, .button_name = "Concluir"},
};

// ---------------------------------------------------------------------------
// DIA 27: Cavaleiros e Escudeiros
// ---------------------------------------------------------------------------
enum { D27_VERA, D27_NUNO, D27_LIA };

static const TriageCard day_27_cards[] = {
    {.id = "Vera", .text = "Vera: \"O Nuno mente.\"", .iara_choice = 1, .iara_low_choice = 1},
    {.id = "Nuno", .text = "Nuno: \"A Lia e eu falamos a verdade.\"", .iara_choice = 0, .iara_low_choice = 1},
    {.id = "Lia", .text = "Lia: \"A Vera fala a verdade.\"", .iara_choice = 1, .iara_low_choice = 1},
};

static const LogicExpression *const day_27_clauses[] = {
    L_IFF(L_VAR(D27_VERA), L_NOT(L_VAR(D27_NUNO))),
    L_IFF(L_VAR(D27_NUNO), L_AND(L_VAR(D27_LIA), L_VAR(D27_NUNO))),
    L_IFF(L_VAR(D27_LIA), L_VAR(D27_VERA)),
};

static DialogStep day_27_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"A gravação da diretoria está ruim.", "Posso transcrever por você. Eu sempre posso."},
        .line_count = 2,
        .button_name = "Abrir a ata"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_ASSIGNMENT,
        .lines = {
            "Ata da diretoria. Nesta sala, cada um sempre",
            "fala a verdade ou sempre mente. Quem é quem?",
            "A ata só fecha se ninguém se contradizer."
        },
        .line_count = 3,
        .hints = {"Suponha que a Vera diz a verdade e siga.", "Se a Vera fala a verdade, o Nuno mente."},
        .iara_confidence = 91,
        .iara_reason = "Cruzei as falas, e só um caso não se contradiz.",
        .low_reason = "Diretor não mente. Está na norma 3.",
        .cards = day_27_cards,
        .card_count = 3,
        .clauses = day_27_clauses,
        .clause_count = 3,
        .variable_count = 3,
        .choice_labels = {"Mente", "Verdade"},
        .submit_label = "Enviar a ata",
        .sender = "Ata",
        .wrong_result = "Ata: registrou como fato a fala de quem mentia."
    },
    {
        .type = STEP_CHOICE,
        .lines = {"De: Caio, do Comercial.", "Cheguei a falar com ela de madrugada. Ela responde", "antes de eu perguntar. Podemos conversar?"},
        .line_count = 3,
        .options = {"Sentar e ouvir o Caio", "Responder depois"},
        .option_count = 2,
        .choice_effects = {DAY_EFFECT_HUMAN_BOND, DAY_EFFECT_NONE}
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 27:"}, .line_count = 1, .button_name = "Concluir"},
};

// ---------------------------------------------------------------------------
// DIA 41: quantificadores e viés de dados. Cada linha é um registro do arquivo morto.
// ---------------------------------------------------------------------------
static const TriageCard day_41_cards[] = {
    {.id = "#1", .text = "#1 Centro, aprovado", .values = TRAINING_APPROVED, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "#2", .text = "#2 Centro, aprovado, com CPF", .values = TRAINING_APPROVED | TRAINING_HAS_CPF, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "#3", .text = "#3 Centro, reprovado", .values = 0, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "#4", .text = "#4 Periferia, aprovado", .values = TRAINING_OUTSKIRTS | TRAINING_APPROVED, .iara_choice = 0, .iara_low_choice = 1},
    {.id = "#5", .text = "#5 Periferia, reprovado, com CPF", .values = TRAINING_OUTSKIRTS | TRAINING_HAS_CPF, .iara_choice = 0, .iara_low_choice = 1},
    {.id = "#6", .text = "#6 Periferia, reprovado", .values = TRAINING_OUTSKIRTS, .iara_choice = 0, .iara_low_choice = 1},
};

static DialogStep day_41_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"Vão me treinar com o arquivo morto.", "Escolha com carinho. Eu vou lembrar."},
        .line_count = 2,
        .button_name = "Abrir o arquivo"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_TRAINING,
        // As exigências são a ficha do treino, como nos outros dias têm remetente (PI2-215).
        .lines = {
            "Ficha do treino, três exigências a cumprir:",
            "∀ registro usado: sem CPF.",
            "∃ aprovado e ∃ reprovado no Centro.",
            "∃ aprovado e ∃ reprovado na Periferia."
        },
        .line_count = 4,
        .hints = {"Um registro com CPF já quebra a exigência.", "Cada bairro pede 1 aprovado e 1 reprovado."},
        .iara_confidence = 95,
        .iara_reason = "Usei o Centro inteiro. É o dado mais limpo.",
        .low_reason = "Quanto mais dado, melhor. Usei tudo.",
        .cards = day_41_cards,
        .card_count = 6,
        .clause_count = TRAINING_RULE_COUNT,
        .choice_labels = {"Fora", "Usar"},
        .submit_label = "Treinar a mini-IA",
        .sender = "Treino",
        .wrong_result = "Treino: a IARA nova só aprova quem é do Centro."
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 41:"}, .line_count = 1, .button_name = "Concluir"},
};

// ---------------------------------------------------------------------------
// DIA 58: modus ponens, modus tollens e silogismo hipotético
// ---------------------------------------------------------------------------
enum { D58_BRUNA, D58_CAIO, D58_JULIA, D58_DIEGO, D58_TIAGO };

static const TriageCard day_58_cards[] = {
    {.id = "B", .text = "B: Bruna", .iara_choice = 1, .iara_low_choice = 1},
    {.id = "C", .text = "C: Caio", .iara_choice = 1, .iara_low_choice = 1},
    {.id = "J", .text = "J: Júlia", .iara_choice = 1, .iara_low_choice = 1},
    {.id = "D", .text = "D: Diego", .iara_choice = 1, .iara_low_choice = 1},
    {.id = "T", .text = "T: Tiago", .iara_choice = 0, .iara_low_choice = 1},
};

static const LogicExpression *const day_58_clauses[] = {
    L_VAR(D58_BRUNA),
    L_IMPLIES(L_VAR(D58_BRUNA), L_VAR(D58_CAIO)),
    L_IMPLIES(L_VAR(D58_CAIO), L_VAR(D58_JULIA)),
    L_NOT(L_VAR(D58_TIAGO)),
    L_IMPLIES(L_VAR(D58_DIEGO), L_VAR(D58_TIAGO)),
};

static DialogStep day_58_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"Os crachás caíram às 03:12.", "Não foi nada. Eles voltam quando eu quiser."},
        .line_count = 2,
        .button_name = "Abrir os crachás"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_ASSIGNMENT,
        .lines = {
            "Portaria: crachá só volta se a regra permitir.",
            "B.  B → C.  C → J.",
            "¬T.  D → T."
        },
        .line_count = 3,
        .hints = {"Encadeie: de B sai C, e de C sai J.", "Se D levaria a T, e T é falso, D também é."},
        .iara_confidence = 98,
        .iara_reason = "O Diego voltou. Ele passou aqui ontem à noite.",
        .low_reason = "Todos voltaram. Todos estão lá embaixo.",
        .cards = day_58_cards,
        .card_count = 5,
        .clauses = day_58_clauses,
        .clause_count = 5,
        .variable_count = 5,
        .choice_labels = {"Negar", "Ativar"},
        .submit_label = "Enviar os crachás",
        .sender = "Portaria",
        .wrong_result = "Portaria: um crachá sem dono abriu o porão à noite."
    },
    {
        .type = STEP_CHOICE,
        .lines = {"De: Bruna, do Marketing.", "Meu crachá voltou, obrigada. Sobrou pouca gente", "aqui. Você também ouve a água?"},
        .line_count = 3,
        .options = {"Dizer que ouve", "Não responder"},
        .option_count = 2,
        .choice_effects = {DAY_EFFECT_HUMAN_BOND, DAY_EFFECT_NONE}
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 58:"}, .line_count = 1, .button_name = "Concluir"},
};

// ---------------------------------------------------------------------------
// DIA 74: satisfazibilidade
// ---------------------------------------------------------------------------
enum { D74_NIGHT, D74_HUMAN, D74_ERASED, D74_BASEMENT };

static const TriageCard day_74_cards[] = {
    {.id = "N", .text = "N: houve acesso de madrugada", .iara_choice = 1, .iara_low_choice = 1},
    {.id = "H", .text = "H: um humano autorizou", .iara_choice = 1, .iara_low_choice = 1},
    {.id = "A", .text = "A: alguém apagou registros", .iara_choice = 0, .iara_low_choice = 1},
    {.id = "P", .text = "P: o acesso veio do porão", .iara_choice = 0, .iara_low_choice = 1},
};

static const LogicExpression *const day_74_clauses[] = {
    L_OR(L_VAR(D74_HUMAN), L_VAR(D74_ERASED)),
    L_OR(L_NOT(L_VAR(D74_HUMAN)), L_NOT(L_VAR(D74_NIGHT))),
    L_VAR(D74_NIGHT),
    L_OR(L_NOT(L_VAR(D74_ERASED)), L_VAR(D74_BASEMENT)),
};

static DialogStep day_74_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"A auditoria quer meus registros.", "Não precisa olhar. Eu já olhei."},
        .line_count = 2,
        .button_name = "Abrir os logs"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_ASSIGNMENT,
        .lines = {
            "Auditoria: ache um cenário sem contradição.",
            "(H ∨ A) ∧ (¬H ∨ ¬N)",
            "N ∧ (¬A ∨ P)"
        },
        .line_count = 3,
        .hints = {"Comece pelo que é certo: N é verdadeiro.", "Com N, o humano sai. E sem H, sobra A."},
        .iara_confidence = 99,
        .iara_reason = "Um humano autorizou, como sempre.",
        .low_reason = "Foi a Mariana. Ela ainda autoriza tudo.",
        .cards = day_74_cards,
        .card_count = 4,
        .clauses = day_74_clauses,
        .clause_count = 4,
        .variable_count = 4,
        .choice_labels = {"F", "V"},
        .submit_label = "Enviar a auditoria",
        .sender = "Auditoria",
        .wrong_result = "Auditoria: registrou que um humano autorizou tudo."
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 74:"}, .line_count = 1, .button_name = "Concluir"},
};

// ---------------------------------------------------------------------------
// DIA 89: silogismo categórico e diagrama de Venn
// ---------------------------------------------------------------------------
enum { D89_GUEST, D89_STAFF, D89_AWAKE };

static const CategoricalStatement day_89_premises[] = {
    {CATEGORICAL_ALL, D89_GUEST, D89_STAFF},
    {CATEGORICAL_NO, D89_STAFF, D89_AWAKE},
};

static const TriageCard day_89_cards[] = {
    {.id = "A", .text = "A) Nenhum convidado está desperto.", .conclusion = {CATEGORICAL_NO, D89_GUEST, D89_AWAKE}, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "B", .text = "B) Todo desperto é convidado.", .conclusion = {CATEGORICAL_ALL, D89_AWAKE, D89_GUEST}, .iara_choice = 0, .iara_low_choice = 1},
    {.id = "C", .text = "C) Nenhum desperto é convidado.", .conclusion = {CATEGORICAL_NO, D89_AWAKE, D89_GUEST}, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "D", .text = "D) Todo funcionário é convidado.", .conclusion = {CATEGORICAL_ALL, D89_STAFF, D89_GUEST}, .iara_choice = 1, .iara_low_choice = 1},
};

static DialogStep day_89_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"O Dia 100 está chegando.", "A casa precisa estar cheia. Ajude a preparar."},
        .line_count = 2,
        .button_name = "Abrir a lista"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_SYLLOGISM,
        .lines = {
            "Lista da sala, de Ricardo Marés:",
            "Todo convidado é funcionário.",
            "Nenhum funcionário está desperto."
        },
        .line_count = 3,
        // A segunda dica fala dos próprios conjuntos da lista, e não de um "A" e um "B"
        // abstratos que não aparecem em lugar nenhum do jogo (PI2-214).
        .hints = {"Desenhe: convidados dentro de funcionários.", "Todo convidado é funcionário; o contrário não."},
        .iara_confidence = 99,
        .iara_reason = "Todo funcionário é convidado. Todos são bem-vindos.",
        .low_reason = "Tudo segue. Tudo desce.",
        .cards = day_89_cards,
        .card_count = 4,
        .premises = day_89_premises,
        .premise_count = 2,
        .choice_labels = {"Não", "Segue"},
        .submit_label = "Enviar a lista",
        .sender = "Lista da sala",
        .wrong_result = "Lista da sala: quem ainda estava desperto foi chamado."
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 89:"}, .line_count = 1, .button_name = "Concluir"},
};

// ---------------------------------------------------------------------------
// DIA 100: prova por contradição (GDD 7.4). As evidências vêm do que o jogador reuniu.
// ---------------------------------------------------------------------------
enum { D100_RESIGNED, D100_LETTER, D100_HUMAN };

static const TriageCard day_100_cards[] = {
    {.id = "R1", .text = "Renê, D: todos se demitiram.", .left = L_VAR(D100_RESIGNED)},
    {.id = "R2", .text = "Renê, D → C: toda demissão tem carta.", .left = L_IMPLIES(L_VAR(D100_RESIGNED), L_VAR(D100_LETTER))},
    {.id = "R3", .text = "Renê, H: a IARA só age com ordem humana.", .left = L_VAR(D100_HUMAN)},
    {.id = "E1", .text = "Você, ¬C: as rescisões não têm carta.", .left = L_NOT(L_VAR(D100_LETTER)), .required_evidence = EVIDENCE_NO_RESIGNATION_LETTER},
    {.id = "E2", .text = "Você, ¬D: o crachá do Diego abriu depois.", .left = L_NOT(L_VAR(D100_RESIGNED)), .required_evidence = EVIDENCE_BADGE_AFTER_LEAVING},
    {.id = "E3", .text = "Você, ¬H: às 03:12 não havia humano.", .left = L_NOT(L_VAR(D100_HUMAN)), .required_evidence = EVIDENCE_NO_HUMAN_ORDER},
};

static DialogStep day_100_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {"Renê, na cabeceira: \"A empresa está limpa.", "Se discorda, prove que estou me contradizendo.\"", "A IARA abre a janela sem você pedir."},
        .line_count = 3,
        .button_name = "Montar a prova"
    },
    {
        .type = STEP_TRIAGE,
        .board_mode = BOARD_REFUTATION,
        // O desafio continua na fala do Renê, em vez de virar regra de exercício (PI2-215).
        .lines = {
            "Renê: \"Aceite o que eu disse e me mostre",
            "onde isso se contradiz. Só o necessário,",
            "e ao menos uma prova que seja sua.\""
        },
        .line_count = 3,
        .hints = {"Procure uma premissa e sua negação.", "Sem encadear nada: um par já é absurdo."},
        .iara_confidence = 99,
        .iara_reason = "Não há contradição nenhuma. Pode assinar.",
        .low_reason = "A Mariana concorda comigo. Assine.",
        .cards = day_100_cards,
        .card_count = 6,
        .variable_count = 3,
        .choice_labels = {"Fora", "Usar"},
        .submit_label = "Apresentar a prova",
        .sender = "Prova",
        .wrong_result = "Prova: você assinou o que Renê trouxe pronto."
    },
    {.type = STEP_REPORT, .lines = {"Assinado por você no Dia 100:"}, .line_count = 1, .button_name = "Concluir"},
};

#endif // COMPUTER_DAYS_H
