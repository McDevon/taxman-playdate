#include "game_data.h"
#include <string.h>

void game_data_destroy(void *object)
{
    GameData *base = (GameData *)object;
    destroy(base->tile_dictionary);
    destroy(base->mechanics_random);
    destroy(base->visual_random);
}

char *game_data_describe(void *object)
{
    return platform_strdup("with tile dictionary");
}

BaseType GameDataType = { "GameData", &game_data_destroy, &game_data_describe };

GameData *game_data_create()
{
    GameData *base = platform_calloc(1, sizeof(GameData));
    base->w_type = &GameDataType;
    base->tile_dictionary = hashtable_create();
    base->mechanics_random = random_create(6608090405212458590LL, 4588768532683232228LL);
    base->visual_random = random_create(4608090406132658590LL, 5588768554981732228LL);

    return base;
}
