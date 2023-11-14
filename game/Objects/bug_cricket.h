#ifndef bug_cricket_h
#define bug_cricket_h

#include "engine.h"

typedef struct BugCricket BugCricket;

BugCricket *bug_cricket_create(GameObject *gecko_head, float angle, context_callback_t *eaten_callback, void *eaten_callback_context);

#endif /* bug_cricket_h */
