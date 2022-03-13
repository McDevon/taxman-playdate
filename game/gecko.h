#ifndef gecko_h
#define gecko_h

#include "engine.h"
#include "debug_draw.h"

typedef struct GeckoCharacter GeckoCharacter;

GeckoCharacter *gecko_char_create(DebugDraw *debug);

#endif /* gecko_h */
