#ifndef game_data_h
#define game_data_h

#include <stdio.h>
#include "engine.h"
#include "random.h"

typedef struct GameData {
    BASE_OBJECT;
    Random *mechanics_random;
    Random *visual_random;
    int latest_score;
} GameData;

GameData *game_data_create(void);

#endif /* game_data_h */
