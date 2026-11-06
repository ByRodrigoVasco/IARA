#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sv_meta.h"

SvMeta sv_meta;

static const char *upgrade_keys[UPG_COUNT] = {"vida", "dano", "velocidade", "recarga", "area", "ima", "xp", "armadura", "recomeco"};

void sv_meta_reset(void)
{
    memset(&sv_meta, 0, sizeof(sv_meta));
}

bool sv_meta_load(const char *path)
{
    FILE *file = fopen(path, "r");
    char line[128];

    sv_meta_reset();
    if (!file) return false;

    while (fgets(line, sizeof(line), file))
    {
        char *equals = strchr(line, '=');
        int value;

        if (!equals) continue;
        *equals = '\0';
        value = atoi(equals + 1);
        if (value < 0) value = 0;

        if (!strcmp(line, "coins")) sv_meta.coins = value;
        else if (!strcmp(line, "runs")) sv_meta.runs = value;
        else if (!strcmp(line, "wins")) sv_meta.wins = value;
        else if (!strcmp(line, "best_seconds")) sv_meta.best_seconds = value;
        else if (!strcmp(line, "kills")) sv_meta.kills = value;
        else if (!strcmp(line, "character")) sv_meta.character = value < CH_COUNT ? value : 0;
        else
        {
            for (int i = 0; i < UPG_COUNT; i++)
            {
                if (!strcmp(line, upgrade_keys[i])) sv_meta.upgrades[i] = value > sv_upgrade_max(i) ? sv_upgrade_max(i) : value;
            }
        }
    }

    fclose(file);
    return true;
}

bool sv_meta_save(const char *path)
{
    char temp_path[260];
    FILE *file;

    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    file = fopen(temp_path, "w");
    if (!file) return false;

    fprintf(file, "coins=%d\nruns=%d\nwins=%d\nbest_seconds=%d\nkills=%d\ncharacter=%d\n", sv_meta.coins, sv_meta.runs, sv_meta.wins, sv_meta.best_seconds, sv_meta.kills, sv_meta.character);
    for (int i = 0; i < UPG_COUNT; i++) fprintf(file, "%s=%d\n", upgrade_keys[i], sv_meta.upgrades[i]);

    if (fclose(file) != 0) return false;

    // Só troca o arquivo antigo quando o novo está completo
    remove(path);
    return rename(temp_path, path) == 0;
}

bool sv_meta_buy(SvUpgradeId id)
{
    int cost;

    if (id < 0 || id >= UPG_COUNT) return false;
    cost = sv_upgrade_cost(id, sv_meta.upgrades[id]);
    if (cost < 0 || sv_meta.coins < cost) return false;

    sv_meta.coins -= cost;
    sv_meta.upgrades[id]++;
    return true;
}
