#ifndef bug_hopper_h
#define bug_hopper_h

#include "engine.h"

typedef struct BugHopper BugHopper;

BugHopper *bug_hopper_create(GameObject *gecko_head, float angle, context_callback_t *eaten_callback, void *eaten_callback_context);

#endif /* bug_hopper_h */
