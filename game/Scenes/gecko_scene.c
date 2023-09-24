#include "gecko_scene.h"
#include "gecko.h"
#include "game_data.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "debug_draw.h"

typedef struct {
    GAME_OBJECT;
    Label *w_info_label;
    Label *w_camera_label;
    Label *w_attach_label;
    GameObject *w_head;
    DebugDraw *w_debug;
    Vector2D camera_position;
    Controls previous_controls;
    bool follow;
} GeckoScene;

Number timer = 0;

void gecko_scene_update(GameObject *scene, Number dt_ms)
{
    GeckoScene *self = (GeckoScene *)scene;
    timer += dt_ms;
    /*if (timer > nb_from_int(10000)) {
        profiler_schedule_end();
    }*/
    
    Controls controls = go_get_scene_manager(self)->controls;
    
    if (self->follow) {
        get_main_render_context()->render_camera->position = self->w_head->position;
        self->camera_position = self->w_head->position;
    } else {
        Number camera_speed = nb_from_int(200);
        Number translate = nb_mul(dt_ms / 1000, camera_speed);
        if (controls.button_up) {
            self->camera_position = vec_vec_add(self->camera_position, vec(nb_zero, -translate));
        }
        if (controls.button_down) {
            self->camera_position = vec_vec_add(self->camera_position, vec(nb_zero, translate));
        }
        if (controls.button_left) {
            self->camera_position = vec_vec_add(self->camera_position, vec(-translate, nb_zero));
        }
        if (controls.button_right) {
            self->camera_position = vec_vec_add(self->camera_position, vec(translate, nb_zero));
        }
        get_main_render_context()->render_camera->position = self->camera_position;
    }

    if (controls.button_b && !self->previous_controls.button_b) {
        self->follow = !self->follow;
        label_set_text(self->w_camera_label, self->follow ? "Camera: follow" : "Camera: D-pad");
    }

    self->previous_controls = controls;
}

void gecko_scene_fixed_update(GameObject *scene, Number dt_ms)
{
    GeckoScene *self = (GeckoScene *)scene;
    GameData *data = (GameData*)go_get_scene_manager(self)->data;

}

void gecko_scene_render(GameObject *scene, RenderContext *ctx)
{
    GeckoScene *self = (GeckoScene *)scene;

    context_clear_white(ctx);
}

void gecko_scene_initialize(GameObject *scene)
{
    GeckoScene *self = (GeckoScene *)scene;

    LOG("Enter gecko scene");
    
    self->w_camera_label = ({
        Label *label = label_create("font4", "Camera: D-pad");
        label->position.x = nb_from_int(SCREEN_WIDTH - 2);
        label->position.y = nb_from_int(SCREEN_HEIGHT - 2);
        label->anchor.x = nb_from_int(1);
        label->anchor.y = nb_from_int(1);
        label->ignore_camera = true;
        label->invert = false;
        label;
    });
        
    go_add_child(self, self->w_camera_label);
    
    self->w_debug = debugdraw_create();
    go_set_z_order(self->w_debug, 1000);
    
    self->camera_position = vec(nb_from_int(SCREEN_WIDTH / 2), nb_from_int(SCREEN_HEIGHT / 2));
    get_main_render_context()->render_camera->position = self->camera_position;
        
    self->w_head = ({
        Sprite *sprite = sprite_create("Gecko_Head-5.png");
        
        Animator *head_animation = animator_create();
        ArrayList *anim_idle = list_create();
        list_add(anim_idle, anim_frame_create("Gecko_Head-1.png", nb_from_int(400)));
        list_add(anim_idle, anim_frame_create("Gecko_Head-2.png", nb_from_int(200)));
        list_add(anim_idle, anim_frame_create("Gecko_Head-3.png", nb_from_int(300)));
        list_add(anim_idle, anim_frame_create("Gecko_Head-4.png", nb_from_int(350)));
        list_add(anim_idle, anim_frame_create("Gecko_Head-5.png", nb_from_int(450)));
        list_add(anim_idle, anim_frame_create("Gecko_Head-6.png", nb_from_int(1500)));
        animator_add_animation(head_animation, "idle", anim_idle);
        
        go_add_component(sprite, head_animation);
        
        animator_set_animation(head_animation, "idle");
        
        sprite->anchor.x = nb_zero;
        sprite->anchor.y = nb_half;
        sprite->position.x = nb_from_int(200);
        sprite->position.y = nb_from_int(120);
        sprite->draw_mode = drawmode_rotate;

        go_set_z_order(sprite, 5);
                
        GeckoCharacter *gecko_comp = gecko_char_create(self->w_debug);
        
        go_add_component(sprite, gecko_comp);
        
        (GameObject *)sprite;
    });

    go_add_child(self, self->w_head);
    go_add_child(self, self->w_debug);
        
    set_screen_dither(get_image_data("dither_blue.png"));
}

void gecko_scene_start(GameObject *scene)
{
    //profiler_schedule_start();
}

void gecko_scene_destroy(void *scene)
{
    go_destroy(scene);
}

char *gecko_scene_describe(void *scene)
{
    return platform_strdup("[]");
}

static GameObjectType GeckoSceneType = {
    { { "GeckoScene", &gecko_scene_destroy, &gecko_scene_describe } },
    &gecko_scene_initialize,
    NULL,
    &gecko_scene_start,
    &gecko_scene_update,
    &gecko_scene_fixed_update,
    &gecko_scene_render
};

GameObject *gecko_scene_create()
{
    GameObject *p_scene = go_alloc(sizeof(GeckoScene));
    p_scene->w_type = &GeckoSceneType;
    
    return p_scene;
}
