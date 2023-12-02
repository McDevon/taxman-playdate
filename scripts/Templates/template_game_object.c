#include "template_game_object.h"

typedef struct TemplateGameObject {
    GAME_OBJECT;
    int score;
} TemplateGameObject;

void template_game_object_destroy(void *obj)
{
    TemplateGameObject *self = (TemplateGameObject *)obj;
    go_destroy(obj);
}

char *template_game_object_describe(void *obj)
{
    return go_describe(obj);
}

void template_game_object_added_to_parent(GameObject *obj)
{
    TemplateGameObject *self = (TemplateGameObject *)obj;
}

void template_game_object_will_be_removed_from_parent(GameObject *obj)
{
    TemplateGameObject *self = (TemplateGameObject *)obj;
}

void template_game_object_start(GameObject *obj)
{
    TemplateGameObject *self = (TemplateGameObject *)obj;
}

void template_game_object_update(GameObject *obj, float dt)
{
    TemplateGameObject *self = (TemplateGameObject *)obj;
}

void template_game_object_fixed_update(GameObject *obj, float dt)
{
    TemplateGameObject *self = (TemplateGameObject *)obj;
}

void template_game_object_render(GameObject *obj, RenderContext *ctx)
{
    TemplateGameObject *self = (TemplateGameObject *)obj;
}

// Functions which are not used can be removed and set to NULL in the type list
GameObjectType TemplateGameObjectType =
    game_object_type(
        "TemplateGameObject",
        &template_game_object_destroy,
        &template_game_object_describe,
        &template_game_object_added_to_parent,
        &template_game_object_will_be_removed_from_parent,
        &template_game_object_start,
        &template_game_object_update,
        &template_game_object_fixed_update,
        &template_game_object_render
);

TemplateGameObject *template_game_object_create()
{
    TemplateGameObject *obj = (TemplateGameObject *)go_alloc(sizeof(TemplateGameObject));
    obj->w_type = &TemplateGameObjectType;

    return obj;
}
