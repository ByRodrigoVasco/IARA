#include <stdio.h>

#include "raylib.h"
#include "cloud.h"
#include "sv_ranking_cloud.h"

#define REFRESH_SECONDS 20.0

static bool duo_table = false;
static SvRankEntry rows[SV_RANKING_MAX];
static int row_count = 0;
static bool online = false;
static int read_ticket = 0;
static int send_ticket = 0;
static double requested_at = 0.0;
static SvSendState send_state = SV_SEND_NONE;
static int position = 0;

static const char *table_type(void)
{
    return duo_table ? "dupla" : "solo";
}

static void request_table(void)
{
    read_ticket = cloud_read_ranking(table_type(), SV_RANKING_MAX);
    requested_at = GetTime();
}

void sv_ranking_cloud_new_run(void)
{
    send_ticket = 0;
    send_state = SV_SEND_NONE;
    position = 0;
}

void sv_ranking_cloud_open(bool duo)
{
    // A tabela do outro tipo não serve: some até a resposta chegar
    if (duo != duo_table)
    {
        row_count = 0;
        online = false;
    }
    duo_table = duo;
    sv_ranking_cloud_new_run();
    read_ticket = 0;
    if (cloud_enabled()) request_table();
}

void sv_ranking_cloud_submit(const char *name, int seconds, int kills, int level, int bosses, int character, bool won)
{
    cloud_match match = {0};

    if (!cloud_enabled()) return;
    snprintf(match.name, sizeof(match.name), "%s", name);
    match.seconds = seconds;
    match.kills = kills;
    match.level = level;
    match.bosses = bosses;
    match.character = character;
    match.won = won;
    position = 0;
    send_ticket = cloud_register_match(table_type(), &match);
    send_state = send_ticket != 0 ? SV_SEND_WAITING : SV_SEND_NONE;
}

static void apply_table(const cloud_result *result)
{
    row_count = result->row_count < SV_RANKING_MAX ? result->row_count : SV_RANKING_MAX;
    for (int i = 0; i < row_count; i++)
    {
        const cloud_match *from = &result->rows[i];
        SvRankEntry *to = &rows[i];

        sv_ranking_clean_name(from->name, to->name, sizeof(to->name));
        to->seconds = from->seconds;
        to->kills = from->kills;
        to->level = from->level;
        to->bosses = from->bosses;
        to->character = from->character % SV_CLOUD_CHARACTER_BASE;   // Na dupla, o do J1
        to->won = from->won;
        to->order = 0;
    }
}

void sv_ranking_cloud_update(void)
{
    cloud_result result;

    if (!cloud_enabled()) return;
    if (send_ticket != 0 && cloud_poll(send_ticket, &result))
    {
        send_ticket = 0;
        position = result.position;
        if (result.status == CLOUD_OK) send_state = result.position > 0 ? SV_SEND_DONE : SV_SEND_REFUSED;
        else send_state = result.status == CLOUD_OFFLINE ? SV_SEND_OFFLINE : SV_SEND_REFUSED;
        cloud_result_free(&result);
        request_table();   // Só depois da resposta a tabela já inclui a partida
    }
    if (read_ticket != 0 && cloud_poll(read_ticket, &result))
    {
        read_ticket = 0;
        online = result.status == CLOUD_OK;
        if (online) apply_table(&result);
        cloud_result_free(&result);
    }
    else if (GetTime() - requested_at > REFRESH_SECONDS) request_table();
}

bool sv_ranking_cloud_online(void)
{
    return cloud_enabled() && online;
}

int sv_ranking_cloud_count(void)
{
    return row_count;
}

const SvRankEntry *sv_ranking_cloud_get(int index)
{
    return index >= 0 && index < row_count ? &rows[index] : NULL;
}

SvSendState sv_ranking_cloud_send_state(void)
{
    return send_state;
}

int sv_ranking_cloud_position(void)
{
    return position;
}
