#include "gecko_scene.h"
#include "gecko.h"
#include "game_data.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "debug_draw.h"

typedef struct {
    SCENE;
    Label *w_info_label;
    Label *w_camera_label;
    Label *w_attach_label;
    GameObject *w_head;
    DebugDraw *w_debug;
    Vector2D camera_position;
    Controls previous_controls;
    bool follow;
} GeckoScene;

void gecko_scene_update(GameObject *scene, Float dt)
{
    GeckoScene *self = (GeckoScene *)scene;
    
    Controls controls = go_get_scene_manager(self)->controls;
    
    if (self->follow) {
        get_main_render_context()->render_camera->position = self->w_head->position;
        self->camera_position = self->w_head->position;
    } else {
        float camera_speed = 200.f;
        float translate = dt * camera_speed;
        if (controls.button_up) {
            self->camera_position = vec_vec_add(self->camera_position, vec(0.f, -translate));
        }
        if (controls.button_down) {
            self->camera_position = vec_vec_add(self->camera_position, vec(0.f, translate));
        }
        if (controls.button_left) {
            self->camera_position = vec_vec_add(self->camera_position, vec(-translate, 0.f));
        }
        if (controls.button_right) {
            self->camera_position = vec_vec_add(self->camera_position, vec(translate, 0.f));
        }
        get_main_render_context()->render_camera->position = self->camera_position;
    }

    if (controls.button_b && !self->previous_controls.button_b) {
        self->follow = !self->follow;
        label_set_text(self->w_camera_label, self->follow ? "Camera: follow" : "Camera: D-pad");
    }

    self->previous_controls = controls;
}

void gecko_scene_fixed_update(GameObject *scene, Float dt_s)
{
    GeckoScene *self = (GeckoScene *)scene;
    GameData *data = (GameData*)go_get_scene_manager(self)->data;

}

typedef struct {
    int distance_x;
    int distance_y;
    int offset_x;
    int offset_y;
    int compensate_x;
    int compensate_y;
    char *image_name;
    Image *w_image;
} GeckoBackgroundElement;

GeckoBackgroundElement gecko_bg_elements[] = {
    { 479, 400, 0, 0, -50, -50, "Ground-6.png", NULL },
    { 587, 400, 0, 150, -150, -50, "Ground-5.png", NULL },
    { 643, 400, 0, 300, -150, -50, "Ground-8.png", NULL },
    { 643, 400, 643 / 2, 212, -150, -50, "Ground-9.png", NULL },
    { 587, 400, 587 / 2, 84, -150, -50, "Ground-10.png", NULL },
};

void gecko_scene_render(GameObject *scene, RenderContext *ctx)
{
    GeckoScene *self = (GeckoScene *)scene;

    context_clear_white(ctx);
    
    Vector2D position = ctx->render_camera->position;
    Vector2DInt intPosition = (Vector2DInt){ (int32_t)roundf(position.x), (int32_t)roundf(position.y) };
    
    const int count = sizeof gecko_bg_elements / sizeof gecko_bg_elements[0];
    for (int i = 0; i < count; ++i) {
        GeckoBackgroundElement element = gecko_bg_elements[i];
        context_render_rect_image(ctx,
                                  element.w_image,
                                  (Vector2DInt){ element.distance_x - mod(intPosition.x + element.offset_x, element.distance_x) + element.compensate_x,
                                                 element.distance_y - mod(intPosition.y + element.offset_y, element.distance_y) + element.compensate_y },
                                  render_options_make(((intPosition.x + element.offset_x) / element.distance_x) & 1,
                                                      false,
                                                      false)
                                  );
    }
}

void gecko_scene_initialize(GameObject *scene)
{
    GeckoScene *self = (GeckoScene *)scene;

    LOG("Enter gecko scene");
    
    self->w_camera_label = ({
        Label *label = label_create("font4", "Camera: D-pad");
        label->position.x = SCREEN_WIDTH - 2;
        label->position.y = SCREEN_HEIGHT - 2;
        label->anchor.x = 1.f;
        label->anchor.y = 1.f;
        label->ignore_camera = true;
        label->invert = false;
        label;
    });
        
    go_add_child(self, self->w_camera_label);
    
    self->w_debug = debugdraw_create();
    go_set_z_order(self->w_debug, 1000);
    
    self->camera_position = vec(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f);
    get_main_render_context()->render_camera->position = self->camera_position;
        
    self->w_head = ({
        Sprite *sprite = sprite_create("Gecko_Head-5.png");
        
        Animator *head_animation = animator_create();
        ArrayList *anim_idle = list_create();
        list_add(anim_idle, anim_frame_create("Gecko_Head-1.png", 0.4f));
        list_add(anim_idle, anim_frame_create("Gecko_Head-2.png", 0.2f));
        list_add(anim_idle, anim_frame_create("Gecko_Head-3.png", 0.3f));
        list_add(anim_idle, anim_frame_create("Gecko_Head-4.png", 0.35f));
        list_add(anim_idle, anim_frame_create("Gecko_Head-5.png", 0.45f));
        list_add(anim_idle, anim_frame_create("Gecko_Head-6.png", 1.5f));
        animator_add_animation(head_animation, "idle", anim_idle);
        
        go_add_component(sprite, head_animation);
        
        animator_set_animation(head_animation, "idle");
        
        sprite->anchor.x = 0.f;
        sprite->anchor.y = 0.5f;
        sprite->position.x = 200.f;
        sprite->position.y = 120.f;
        sprite->draw_mode = drawmode_rotate;

        go_set_z_order(sprite, 5);
                
        GeckoCharacter *gecko_comp = gecko_char_create(self->w_debug);
        
        go_add_component(sprite, gecko_comp);
        
        (GameObject *)sprite;
    });

    go_add_child(self, self->w_head);
    go_add_child(self, self->w_debug);
    
    const int count = sizeof gecko_bg_elements / sizeof gecko_bg_elements[0];
    for (int i = 0; i < count; ++i) {
        gecko_bg_elements[i].w_image = get_image(gecko_bg_elements[i].image_name);
    }
        
    set_screen_dither(get_image_data("dither_blue.png"));
}

void gecko_scene_start(GameObject *scene)
{
}

void gecko_scene_destroy(void *scene)
{
    go_destroy(scene);
}

char *gecko_scene_describe(void *scene)
{
    return platform_strdup("[]");
}

static SceneType GeckoSceneType =
    scene_type("GeckoScene",
               &gecko_scene_destroy,
               &gecko_scene_describe,
               &gecko_scene_initialize,
               NULL,
               &gecko_scene_start,
               &gecko_scene_update,
               &gecko_scene_fixed_update,
               &gecko_scene_render
               );

Scene *gecko_scene_create()
{
    Scene *p_scene = scene_alloc(sizeof(GeckoScene));
    p_scene->w_type = &GeckoSceneType;
    
    scene_set_required_image_asset_names(p_scene, list_of_strings("gecko", "backgrounds", "dithers"));
    scene_set_required_grid_atlas_infos(p_scene, list_of_grid_atlas_infos(grid_atlas_info("font4", (Size2DInt){ 8, 14 })));
    
    return p_scene;
}
