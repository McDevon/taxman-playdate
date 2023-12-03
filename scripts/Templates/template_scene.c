#include "template_scene.h"

typedef struct {
    SCENE;
    int score;
} TemplateScene;

void template_scene_destroy(void *scene)
{
    TemplateScene *self = (TemplateScene *)scene;
    scene_destroy(self);
}

char *template_scene_describe(void *scene)
{
    TemplateScene *self = (TemplateScene *)scene;
    return sb_string_with_format("Scene with score %d", self->score);
}

void template_scene_initialize(GameObject *scene)
{
    TemplateScene *self = (TemplateScene *)scene;
}

void template_scene_start(GameObject *scene)
{
    TemplateScene *self = (TemplateScene *)scene;
}

void template_scene_update(GameObject *scene, Float dt)
{
    TemplateScene *self = (TemplateScene *)scene;
}

void template_scene_fixed_update(GameObject *scene, Float dt_s)
{
    TemplateScene *self = (TemplateScene *)scene;
}

void template_scene_render(GameObject *scene, RenderContext *ctx)
{
    TemplateScene *self = (TemplateScene *)scene;
    context_clear_white(ctx);
}

// Functions which are not used can be removed and set to NULL in the type list
static SceneType TemplateSceneType =
    scene_type(
        "TemplateScene",
        &template_scene_destroy,
        &template_scene_describe,
        &template_scene_initialize,
        NULL, // A scene will never be removed from parent so the function is not set
        &template_scene_start,
        &template_scene_update,
        &template_scene_fixed_update,
        &template_scene_render
);

Scene *template_scene_create()
{
    Scene *scene = scene_alloc(sizeof(TemplateScene));
    scene->w_type = &TemplateSceneType;
    
    TemplateScene *self = (TemplateScene *)scene

    // List assets which will be loaded automatically before scene starts
    //scene_set_required_image_asset_names(scene, list_of_strings("demo_image"));
    //scene_set_required_grid_atlas_infos(scene, list_of_grid_atlas_infos(grid_atlas_info("demo_font", (Size2DInt){ 8, 14 })));
    //scene_set_required_audio_effects(scene, list_of_strings("demo_audio"));
    
    return scene;
}
