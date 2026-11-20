#ifndef CAMPAIGN_SCENES_H
#define CAMPAIGN_SCENES_H

/* Dados das cenas do Dia 12 em diante. Incluído pelo loop do escritório. */
typedef struct {
    int day;
    const char *objective;
    int arrival_actors[3];
    const char *arrival[3];
    int middle_actor;
    const char *middle[5]; // abertura, erro, acerto, envio sem conferir, tomada
    int finish_actors[3];
} CampaignSceneDay;

static const CampaignSceneDay campaign_scene_days[] = {
    {12, "Abra a conciliação no computador",
     {NPC_ANDERSON, NPC_LAURA, NPC_ANDERSON},
     {"A Sônia mandou duas versões. Ela quer saber se dizem a mesma coisa.",
      "Essa mesa ficou vazia hoje. Não me avisaram de ninguém saindo.",
      "Peraí. Confere o texto antes de assinar."},
     NPC_ANDERSON,
     {"A versão curta tá aí? Quero comparar uma linha.",
      "A mesa devolveu. Mantém as marcas e olha de novo.",
      "A mesa aceitou. Ainda tem o fechamento pra conferir.",
      "Já mandou? Depois eu olho o resultado no relatório.",
      "A IARA fechou a mesa. Vamos ver o que ficou registrado."},
     {NPC_ANDERSON, NPC_ANDERSON, NPC_LAURA}},
    {27, "Abra a ata no computador",
     {NPC_ANDERSON, NPC_NICOLLAS, NPC_ANDERSON},
     {"A ata tem falas de três pessoas. A gravação falha no meio.",
      "Também ouvi água por aqui.",
      "Registra o que as falas permitem provar."},
     NPC_ANDERSON,
     {"O trecho com água ainda tá na gravação?",
      "A ata não fechou. Tenta seguir as falas desde o começo.",
      "A mesa aceitou. Quero conferir o registro depois.",
      "Quando sair o relatório, me mostra o que foi registrado.",
      "A IARA terminou a ata. Quero ver qual versão foi registrada."},
     {NPC_NICOLLAS, NPC_ANDERSON, NPC_ANDERSON}},
    {41, "Abra o arquivo no computador",
     {NPC_NICOLLAS, NPC_JV, NPC_JV},
     {"A caixa veio úmida. Separei o que ainda dá pra ler.",
      "Vou testar o conjunto. Primeiro precisamos escolher o que entra.",
      "A ficha tá no computador."},
     NPC_JV,
     {"Você consegue ver o conjunto inteiro?",
      "A ficha devolveu a seleção. Vamos olhar as exigências.",
      "A mesa aceitou. Depois eu testo o que saiu.",
      "Já enviou? Quero ver o conjunto no relatório.",
      "A IARA assumiu o treino. Vou olhar o resultado."},
     {NPC_JV, NPC_NICOLLAS, NPC_JV}},
    {58, "Confira os crachás no computador",
     {NPC_ZITO, NPC_LAURA, NPC_ZITO},
     {"A lista de acesso caiu. Vou checar a conexão daqui.",
      "Tem nomes que eu lembro do seu primeiro dia. Confere antes de ativar.",
      "A regra da portaria tá na tela."},
     NPC_ZITO,
     {"A mesa ainda tá respondendo?",
      "Ela devolveu. Segue uma regra de cada vez.",
      "A mesa aceitou. Vamos conferir o relatório.",
      "O envio já saiu? A portaria registra o resultado.",
      "A IARA assumiu os crachás. Quero ver o que ela registrou."},
     {NPC_LAURA, NPC_LAURA, NPC_ZITO}},
    {74, "Abra os logs no computador",
     {NPC_JULIO, NPC_VASCO, NPC_JULIO},
     {"O pedido de auditoria chegou. Tem uma linha sem autor.",
      "A chefia não veio. Primeiro, vê o que os registros permitem fechar.",
      "Se a tela travar, me chama antes de reiniciar."},
     NPC_JULIO,
     {"O log ainda tá aberto?",
      "A auditoria devolveu a combinação. Confere o que já está certo.",
      "A mesa aceitou. Vamos olhar o relatório.",
      "Já enviou? Quero ver o que ficou registrado.",
      "A IARA assumiu a auditoria. O relatório ainda precisa ser lido."},
     {NPC_ZITO, NPC_VASCO, NPC_VASCO}},
    {89, "Abra a lista no computador",
     {NPC_LAURA, NPC_VASCO, NPC_LAURA},
     {"Seu nome tá na lista do Dia 100.",
      "A sala precisa ficar pronta. Lê as condições antes de fechar.",
      "Se precisar, me chama."},
     NPC_LAURA,
     {"Essas pessoas todas são convidadas?",
      "A mesa devolveu. Lê as condições outra vez.",
      "A mesa aceitou. Vou esperar o relatório.",
      "Já saiu? Depois lê o que foi registrado.",
      "A IARA fechou a lista. O relatório mostra o que ela enviou."},
     {NPC_NICOLLAS, NPC_LAURA, NPC_LAURA}},
    {100, "Abra a prova no computador",
     {NPC_VASCO, NPC_LAURA, NPC_VASCO},
     {"A reunião começou. A IARA espera a resposta na tela.",
      "A sala já está aberta.",
      "Confere o que estiver disponível."},
     NPC_JV,
     {"Você conseguiu comparar as afirmações?",
      "A prova não fechou. Volta ao que você consegue sustentar.",
      "A mesa aceitou a prova. Quero ver o que ficou registrado.",
      "Já apresentou? Vamos ler o relatório.",
      "A IARA assumiu a prova. Vamos ler o resultado."},
     {NPC_JV, NPC_LAURA, NPC_LAURA}},
};

static const CampaignSceneDay *get_campaign_scene_day(int day)
{
    for (unsigned i = 0; i < sizeof(campaign_scene_days) / sizeof(*campaign_scene_days); i++)
        if (campaign_scene_days[i].day == day) return &campaign_scene_days[i];
    return NULL;
}

#endif
