#include "loading_scene.h"
#include "engine.h"
#include "gecko_scene.h"
#include "tilemap.h"
#include "game_data.h"
#include <stdio.h>
#include <math.h>

#include "bezier.h"

typedef struct {
    SCENE;
    bool initialized;
} LoadingScene;

void loading_scene_render(GameObject *scene, RenderContext *ctx)
{
    context_clear_white(ctx);
}

void endcall(void *go, void *unused)
{
    scene_change(go_get_scene_manager(go), gecko_scene_create(), st_fade_black, 0.8f);
}

void loading_scene_set_draw_mode(void *obj, void *target_obj)
{
    Sprite *target = (Sprite *)target_obj;
    target->draw_mode = drawmode_rotate;
}

void loading_scene_start(GameObject *obj)
{
    LoadingScene *self = (LoadingScene *)obj;
    const Float time_fall = 0.4f;
    const Float time_start = 0.25f;
    const Float time_flatten = 0.29f;
    const Float time_jump = 0.7f;
    const Float time_wait = 1.f;
    
    go_get_scene_manager(obj)->data = game_data_create();
    
    get_main_render_context()->render_camera->position = vec(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f);

    go_add_child(self, ({
        Sprite *sprite = sprite_create("logo_engine.png");
        sprite->position.x = SCREEN_WIDTH / 2.f;
        sprite->position.y = 160.f;
        sprite->anchor.x = 0.5f;
        sprite->anchor.y = 1.f;
        sprite->scale.x = 1.f;
        sprite->scale.y = 1.f;
        sprite->rotation = 0.f;

        sprite->draw_mode = drawmode_scale;
        
        go_set_z_order(sprite, 1);
        
        go_add_component(sprite, act_create(action_sequence_create(({
            ArrayList *list = list_create();
            list_add(list, action_delay_create(time_start + time_fall));
            list_add(list, action_ease_out_create(action_scale_to_create(vec(1.5f, 0.5f), time_flatten)));
            list_add(list, action_ease_in_create(action_scale_to_create(vec(1.f, 1.f), time_flatten)));
            list;
        }))));

        sprite;
    }));
    
    go_add_child(self, ({
        Float start_scale = 0.9f;
        
        Sprite *sprite = sprite_create("logo_taxman.png");
        sprite->position.x = SCREEN_WIDTH / 2.f;
        sprite->position.y = -1.f;
        sprite->anchor.x = 0.5f;
        sprite->anchor.y = 1.f;
        sprite->scale.x = start_scale;
        sprite->scale.y = start_scale;
        sprite->rotation = 0.f;

        sprite->draw_mode = drawmode_scale;

        go_set_z_order(sprite, 1);
        
        go_add_component(sprite, act_create(action_sequence_create(({
            //Float jump_bezier[4] = { .54f, 4.97f, 1.f, 1.31f };
            Float jump_bezier_table[] = { 0.000000f, 0.459692f, 0.871031f, 1.235496f, 1.554405f, 1.828919f, 2.060041f, 2.248616f, 2.395327f, 2.500677f, 2.564972f, 2.588284f, 2.570396f, 2.510713f, 2.408102f, 2.260611f, 2.064869f, 1.814598f, 1.495427f, 1.000000f };
            
            ArrayList *list = list_create();
            list_add(list, action_delay_create(time_start));
            list_add(list, action_ease_in_create(action_move_to_create(vec(SCREEN_WIDTH / 2.f, 137.f), time_fall)));
            list_add(list, action_ease_out_create(action_scale_to_create(vec(1.5f, 0.5f), time_flatten)));
            list_add(list, action_ease_in_create(action_scale_to_create(vec(1.f, 1.f), time_flatten)));
            list_add(list, action_ease_bezier_prec_table_create(action_move_by_create(vec(0.f, -15.f), time_jump), jump_bezier_table, 20));
            list_add(list, action_delay_create(time_wait));
            list_add(list, action_callback_create(endcall, NULL));
            list;
        }))));
        
        go_add_component(sprite, act_create(action_sequence_create(({
            ArrayList *list = list_create();
            list_add(list, action_delay_create(time_start + time_fall));
            list_add(list, action_ease_out_create(action_move_by_create(vec(0.f, 11.f), time_flatten)));
            list_add(list, action_ease_in_create(action_move_by_create(vec(0.f, -10.f), time_flatten)));
            list;
        }))));

        go_add_component(sprite, act_create(action_sequence_create(({
            ArrayList *list = list_create();
            list_add(list, action_delay_create(time_start + time_fall + 2 * time_flatten));
            list_add(list, action_callback_create(&loading_scene_set_draw_mode, sprite));
            list_add(list, action_ease_out_create(action_rotate_to_create((float)M_PI * 0.022f, time_jump)));
            list;
        }))));

        sprite;
    }));
}


void loading_scene_destroy(void *scene)
{
    scene_destroy(scene);
}

char *loading_scene_describe(void *scene)
{
    return go_describe(scene);
}

static SceneType LoadingSceneType =
    scene_type(
               "LoadingScene",
               &loading_scene_destroy,
               &loading_scene_describe,
               NULL,
               NULL,
               &loading_scene_start,
               NULL,
               NULL,
               &loading_scene_render
               );

Scene *loading_scene_create()
{
    LoadingScene *p_scene = (LoadingScene*)scene_alloc(sizeof(LoadingScene));
    
    p_scene->w_type = &LoadingSceneType;
    
    scene_set_required_image_asset_names(p_scene, list_of_strings("demo_sprites", "dithers"));
        
    return (Scene *)p_scene;
}
