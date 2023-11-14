#ifndef bug_fly_h
#define bug_fly_h

#include "engine.h"

typedef struct BugFly BugFly;

BugFly *bug_fly_create(GameObject *gecko_head, float angle, context_callback_t *eaten_callback, void *eaten_callback_context);

#endif /* bug_fly_h */
