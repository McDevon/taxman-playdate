#include "template_component.h"

typedef struct TemplateComponent {
    GAME_OBJECT_COMPONENT;
    int score;
} TemplateComponent;

void template_component_update(GameObjectComponent *comp, float dt)
{
    TemplateComponent *self = (TemplateComponent *)comp;
}

void template_component_fixed_update(GameObjectComponent *comp, float dt)
{
    TemplateComponent *self = (TemplateComponent *)comp;
    GameObject *parent = comp_get_parent(self);
    Controls controls = comp_get_scene_manager(self)->controls;
}

void template_component_destroy(void *obj)
{
    TemplateComponent *self = (TemplateComponent *)obj;
    comp_destroy(obj);
}

char *template_component_describe(void *obj)
{
    TemplateComponent *self = (TemplateComponent *)obj;
    return comp_describe(self);
}

void template_component_start(GameObjectComponent *comp)
{
    TemplateComponent *self = (TemplateComponent *)comp;
}

void template_component_added(GameObjectComponent *comp)
{
    TemplateComponent *self = (TemplateComponent *)comp;
}

void template_component_object_will_be_removed_from_parent(GameObjectComponent *comp)
{
    // Game object to which this component is attached will be removed from its parent
    TemplateComponent *self = (TemplateComponent *)comp;
}

// Functions which are not used can be removed and set to NULL in the type list
static GameObjectComponentType TemplateComponentComponentType =
    go_component_type(
        "TemplateComponent",
        &template_component_destroy,
        &template_component_describe,
        &template_component_added,
        &template_component_object_will_be_removed_from_parent,
        &template_component_start,
        &template_component_update,
        &template_component_fixed_update
);

TemplateComponent *template_component_create()
{
    TemplateComponent *comp = (TemplateComponent *)comp_alloc(sizeof(TemplateComponent));
    comp->w_type = &TemplateComponentComponentType;
    
    return comp;
}
