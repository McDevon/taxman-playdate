#ifndef gecko_h
#define gecko_h

#include "engine.h"
#include "debug_draw.h"

typedef struct GeckoCharacter GeckoCharacter;
typedef void (gecko_movement_callback)(float movement, Vector2D position, float direction_radians, void *callback_context);

GeckoCharacter *gecko_char_create(DebugDraw *debug, gecko_movement_callback movement_callback, void *movement_callback_context);

#endif /* gecko_h */
