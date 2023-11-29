#include "gecko_scene.h"
#include "gecko.h"
#include "game_data.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "debug_draw.h"
#include "bug_fly.h"
#include "bug_cricket.h"
#include "bug_hopper.h"

#define AVERAGE_SPEED_MEASUREMENTS 3

typedef struct {
    float focus;
    float unfocus_multiplier;
    int previous_visible_focus;
    int score;
} GeckoScore;

typedef struct {
    SCENE;
    Label *w_score_label;
    Label *w_focus_label;
    GameObject *w_head;
    DebugDraw *w_debug;
    GameObject *w_fake_position;
    Vector2D camera_position;
    Controls previous_controls;
    GeckoScore score;
    float speed_measurements[AVERAGE_SPEED_MEASUREMENTS];
    float accumulated_movement;
    float latest_speed_measurement;
    float current_gecko_speed;
    float next_bug;
    uint8_t prev_measurement_index;
    bool follow;
    
    int flash_counter;
} GeckoScene;

float timer = 0;

void gecko_scene_update(GameObject *scene, Float dt)
{
    GeckoScene *self = (GeckoScene *)scene;
    timer += dt;
    if (timer > 10.f) {
        //profiler_schedule_end();
    }
    
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

    self->previous_controls = controls;
    
    if (self->flash_counter >= 0) {
        self->flash_counter --;
        
        if (self->flash_counter < 0) {
            set_screen_invert(false);
        }
    }
}

void gecko_scene_fixed_update(GameObject *scene, Float dt_s)
{
    GeckoScene *self = (GeckoScene *)scene;
    GameData *data = (GameData*)go_get_scene_manager(self)->data;

    self->speed_measurements[self->prev_measurement_index + 1 % AVERAGE_SPEED_MEASUREMENTS] = self->latest_speed_measurement;
    self->current_gecko_speed = 0.f;
    for (int i = 0; i < AVERAGE_SPEED_MEASUREMENTS; ++i) {
        self->current_gecko_speed += self->speed_measurements[i];
    }
    self->current_gecko_speed /= AVERAGE_SPEED_MEASUREMENTS;
    self->latest_speed_measurement = 0.f;
}

Pose2D gecko_scene_get_spawn_pose(GeckoScene *self)
{
    GameData *data = (GameData*)go_get_scene_manager(self)->data;
    Random *random = data->mechanics_random;
    
    Float distance = 230.f; // + random_next_float_limit(random, 100.f);
    Float angle = self->w_head->rotation - ((float)M_PI / 10.f) + random_next_float_limit(random, ((float)M_PI / 5.f));
    
    Vector2D offset = vec(cosf(angle) * distance, sinf(angle) * distance);
    Vector2D position = vec_vec_add(self->w_head->position, offset);
        
    return (Pose2D){ position, angle };
}

void gecko_scene_show_message(GeckoScene *self, const char *message, bool screen_space)
{
    go_add_child(self, ({
        Label *label = label_create("font4", message);
        
        label->anchor.x = 0.5f;
        label->anchor.y = 1.f;
                        
        if (screen_space) {
            label->position.x = SCREEN_WIDTH / 2.f;
            label->position.y = SCREEN_HEIGHT / 2.f - 15.f;
            
            label->ignore_camera = true;
        } else {
            label->position.x = self->w_head->position.x;
            label->position.y = self->w_head->position.y - 15.f;
        }
        
        go_set_z_order(label, 20);
                    
        ArrayList *list = list_create();
        list_add(list, action_ease_out_create(action_move_by_create(vec(0.f, -20.f), 0.6f)));
        list_add(list, action_destroy_create());
        
        go_add_component(label, act_create(action_sequence_create(list)));
        
        label;
    }));
}

void gecko_scene_add_score(GeckoScene *self, int score)
{
    if (self->score.focus <= 0.f) {
        return;
    }
    self->score.score += score;
    
    char score_text[16];
    snprintf(score_text, 16, "Score: %d", self->score.score);
    label_set_text(self->w_score_label, score_text);
}

void gecko_scene_add_focus(GeckoScene *self, float focus)
{
    if (self->score.focus <= 0.f) {
        return;
    }
    self->score.focus += focus;
    int visible_focus = (int)ceilf(self->score.focus);
    char focus_text[14];
    snprintf(focus_text, 14, "Focus: %d", visible_focus);
    label_set_text(self->w_focus_label, focus_text);
}

void gecko_scene_fly_eaten(void *context) {
    GeckoScene *self = (GeckoScene*)context;
    self->flash_counter = 1;
    set_screen_invert(true);
    audio_play_file("eat");
    gecko_scene_show_message(self, "Om nom!", true);
    gecko_scene_add_focus(self, 100.f);
    gecko_scene_add_score(self, 100);
    self->score.unfocus_multiplier += 0.01f;
}

void gecko_scene_cricket_eaten(void *context) {
    GeckoScene *self = (GeckoScene*)context;
    audio_stop_file("cricket_walk");
    audio_play_file("eat");
    gecko_scene_show_message(self, "Crunch!", true);
    gecko_scene_add_focus(self, 40.f);
    gecko_scene_add_score(self, 8);
    self->score.unfocus_multiplier += 0.01f;
}

void gecko_scene_hopper_eaten(void *context) {
    GeckoScene *self = (GeckoScene*)context;
    audio_stop_file("hopper_jump");
    audio_play_file("eat");
    gecko_scene_show_message(self, "Crunch!", true);
    gecko_scene_add_focus(self, 70.f);
    gecko_scene_add_score(self, 25);
    self->score.unfocus_multiplier += 0.01f;
}

void gecko_scene_spawn_bug(GeckoScene *self)
{
    GameData *data = (GameData*)go_get_scene_manager(self)->data;
    Random *random = data->mechanics_random;
    
    self->accumulated_movement -= self->next_bug;
    self->next_bug = 300.f + random_next_float_limit(random, 300.f);
    
    Pose2D pose = gecko_scene_get_spawn_pose(self);
    int dice = random_next_int_limit(random, 100);
    if (dice < 10) {
        go_add_child(self, ({
            Sprite *sprite = sprite_create_with_image(NULL);
            sprite->position = pose.position;
            sprite->anchor = vec(0.5f, 0.5f);
            
            go_add_component(sprite, bug_fly_create(self->w_head, pose.rotation, &gecko_scene_fly_eaten, self));
            sprite;
        }));
        audio_play_file("fly_enter");
        LOG("CREATE BUG FLY");
    } else if (dice < 40) {
        go_add_child(self, ({
            Sprite *sprite = sprite_create_with_image(NULL);
            sprite->position = pose.position;
            sprite->anchor = vec(0.5f, 0.5f);
            
            go_add_component(sprite, bug_hopper_create(self->w_head, pose.rotation, &gecko_scene_hopper_eaten, self));
            sprite;
        }));
        LOG("CREATE BUG hopper");
    } else {
        go_add_child(self, ({
            Sprite *sprite = sprite_create_with_image(NULL);
            sprite->position = pose.position;
            sprite->anchor = vec(0.5f, 0.5f);
            
            go_add_component(sprite, bug_cricket_create(self->w_head, pose.rotation, &gecko_scene_cricket_eaten, self));
            sprite;
        }));
        LOG("CREATE BUG cricket");
    }
}

void gecko_scene_finish(void *scene)
{
    SceneManager *manager = go_get_scene_manager(scene);
    scene_change(manager, gecko_scene_create(), st_fade_black, 0.8f);
}

void gecko_scene_character_moved(float movement, Vector2D position, float direction_radians, void *callback_context)
{
    GeckoScene *self = (GeckoScene *)callback_context;
    self->accumulated_movement += movement;
    
    self->latest_speed_measurement = movement;
        
    if (self->accumulated_movement >= self->next_bug) {
        gecko_scene_spawn_bug(self);
    }
    
    self->score.focus -= movement * self->score.unfocus_multiplier;
    int visible_focus = (int)ceilf(self->score.focus);
    if (visible_focus != self->score.previous_visible_focus && visible_focus >= 0) {
        char focus_text[14];
        snprintf(focus_text, 14, "Focus: %d", visible_focus);
        label_set_text(self->w_focus_label, focus_text);
        self->score.previous_visible_focus = visible_focus;
    }
    if (self->follow && self->score.focus <= 0.f) {
        LOG("Focus Lost!");
        self->follow = false;
        go_delayed_callback(self, &gecko_scene_finish, 2.f);
    }
}

float gecko_scene_get_gecko_speed(void *scene)
{
    GeckoScene *self = (GeckoScene *)scene;
    return self->current_gecko_speed;
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
    context_clear_white(ctx);
    
    Vector2D position = ctx->render_camera->position;
    Vector2DInt intPosition = (Vector2DInt){ (int32_t)floorf(floorf(position.x) + (position.x - ceilf(position.x))),
        (int32_t)floorf(floorf(position.y) + (position.y - ceilf(position.y))) };
    
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
    
    self->w_score_label = ({
        Label *label = label_create("font4", "Score: 0");
        label->position.x = SCREEN_WIDTH - 2;
        label->position.y = SCREEN_HEIGHT - 2;
        label->anchor.x = 1.f;
        label->anchor.y = 1.f;
        label->ignore_camera = true;
        label->invert = false;
        label;
    });
    
    self->w_focus_label = ({
        Label *label = label_create("font4", "Focus: 100");
        label->position.x = 2;
        label->position.y = SCREEN_HEIGHT - 2;
        label->anchor.x = 0.f;
        label->anchor.y = 1.f;
        label->ignore_camera = true;
        label->invert = false;
        label;
    });
    
    self->follow = true;
        
    go_add_child(self, self->w_score_label);
    go_add_child(self, self->w_focus_label);

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
                
        GeckoCharacter *gecko_comp = gecko_char_create(self->w_debug, &gecko_scene_character_moved, self);
        
        go_add_component(sprite, gecko_comp);
        
        (GameObject *)sprite;
    });
    

    go_add_child(self, self->w_head);
    go_add_child(self, self->w_debug);
    
    const int count = sizeof gecko_bg_elements / sizeof gecko_bg_elements[0];
    for (int i = 0; i < count; ++i) {
        gecko_bg_elements[i].w_image = get_image(gecko_bg_elements[i].image_name);
    }
    
    self->w_fake_position = go_create_empty();
    go_add_child(self, self->w_fake_position);
    
    self->next_bug = 200.f;
    
    self->flash_counter = -1;
    
    self->score.score = 0;
    self->score.focus = 100.f;
    self->score.unfocus_multiplier = 0.025f;

    set_screen_dither(get_image_data("dither_blue2.png"));
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
    scene_set_required_audio_effects(p_scene, list_of_strings("cricket_alert",
                                                              "cricket_walk",
                                                              "fly_alert",
                                                              "fly_enter",
                                                              "fly_fly",
                                                              "hopper_alert",
                                                              "hopper_jump",
                                                              "eat"));
    
    return p_scene;
}
