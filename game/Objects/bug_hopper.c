#include "bug_hopper.h"
#include "gecko_scene.h"
#include "life_timer.h"
#include "game_data.h"
#include <math.h>

typedef struct BugHopper {
    GAME_OBJECT_COMPONENT;
    GameObject *w_gecko_head;
    RenderTexture *rt_normal;
    RenderTexture *rt_alert;
    RenderTexture *rt_jump;
    context_callback_t *eaten_callback;
    void *eaten_callback_context;
    float alert_timer;
    float alert_time;
    float angle;
    float eat_distance;
    float alert_distance;
    float detect_distance;
    float detect_speed;
    float far_detect_distance;
    float far_detect_speed;
    bool jumping;
} BugHopper;

void bug_hopper_update(GameObjectComponent *comp, float dt)
{
}

void bug_hopper_new_angle_stay(BugHopper *self, float angle)
{
    self->angle = angle;
    destroy(self->rt_normal);
    destroy(self->rt_alert);
    self->rt_normal = render_texture_create_with_rotated(get_image("Grasshopper-1.png"), angle);
    self->rt_alert = render_texture_create_with_rotated(get_image("Grasshopper-A.png"), angle);
}

void bug_hopper_new_angle_jump(BugHopper *self, float angle)
{
    self->angle = angle;
    if (self->rt_jump) {
        destroy(self->rt_jump);
    }
    self->rt_jump = render_texture_create_with_rotated(get_image("Grasshopper-2.png"), angle);
}

void bug_hopper_stop(void *obj, void *context)
{
    BugHopper *self = (BugHopper *)context;
    
    Sprite *parent = (Sprite *)comp_get_parent(self);
    go_set_z_order(parent, 1);
    
    bug_hopper_new_angle_stay(self, self->angle);
    
    parent->draw_mode = drawmode_default;
    parent->scale = vec(1.f, 1.f);
    sprite_set_image(parent, self->rt_normal->image);
    
    self->jumping = false;
}

void bug_hopper_jump(BugHopper *self) {
    Sprite *parent = (Sprite *)comp_get_parent(self);
    
    GameData *data = (GameData*)comp_get_scene_manager(self)->data;
    Random *random = data->mechanics_random;
    
    Vector2D distance = vec_vec_subtract(parent->position, self->w_gecko_head->position);
    const float angle = atan2f(distance.y, distance.x) + (float)M_PI * (-0.3f + random_next_float_limit(random, 0.6f));
    bug_hopper_new_angle_jump(self, angle);

    parent->w_image = self->rt_jump->image;
    parent->draw_mode = drawmode_scale;
    
    self->jumping = true;
    
    const float jump_distance = 120.f + random_next_float_limit(random, 150.f);
    const float time = 0.65f + random_next_float_limit(random, .1f);
    const float jump_default_scale = 0.5f;
    
    go_set_z_order(parent, 20);
    parent->scale = vec(jump_default_scale, jump_default_scale);
    
    go_add_component(parent, act_create(action_sequence_create(({
        ArrayList *list = list_create();
        list_add(list, action_ease_in_out_create(action_move_by_create(vec(cosf(self->angle) * jump_distance, sinf(self->angle) * jump_distance), time)));
        list_add(list, action_callback_create(&bug_hopper_stop, self));
        list;
    }))));
    go_add_component(parent, act_create(action_sequence_create(({
        ArrayList *list = list_create();
        const float scale = 1.0f;
        list_add(list, action_ease_out_create(action_scale_to_create(vec(scale, scale), time * 0.5f)));
        list_add(list, action_ease_in_create(action_scale_to_create(vec(jump_default_scale, jump_default_scale), time * 0.5f)));
        list;
    }))));
    audio_play_file("hopper_jump");
}

void bug_hopper_fixed_update(GameObjectComponent *comp, float dt)
{
    BugHopper *self = (BugHopper *)comp;
    Sprite *parent = (Sprite *)comp_get_parent(self);
    
    float distance = vec_length(vec_vec_subtract(self->w_gecko_head->position, parent->position));
    
    if (distance > 500.f) {
        go_schedule_destroy(parent);
    }
    
    if (self->jumping) {
        return;
    }
    
    if (distance < self->eat_distance) {
        go_schedule_destroy(parent);
        self->eaten_callback(self->eaten_callback_context);
        return;
    }
    
    if (self->alert_timer > 0.f) {
        self->alert_timer -= dt;
        if (self->alert_timer < 0.f) {
            bug_hopper_jump(self);
        }
    } else {
        float gecko_speed = gecko_scene_get_gecko_speed(go_get_root_ancestor(comp_get_parent(self)));
        
        if ((distance < self->alert_distance)
            || (distance < self->detect_distance && gecko_speed > self->detect_speed)
            || (distance < self->far_detect_distance && gecko_speed > self->far_detect_speed)) {
            self->alert_timer = self->alert_time;
            sprite_set_image(parent, self->rt_alert->image);
            audio_play_file("hopper_alert");
        }
    }
}

void bug_hopper_destroy(void *obj)
{
    BugHopper *self = (BugHopper *)obj;

    if (self->rt_normal) {
        destroy(self->rt_normal);
    }
    if (self->rt_alert) {
        destroy(self->rt_alert);
    }
    if (self->rt_jump) {
        destroy(self->rt_jump);
    }

    comp_destroy(obj);
}

char *bug_hopper_describe(void *obj)
{
    return comp_describe(obj);
}

void bug_hopper_start(GameObjectComponent *comp)
{
    BugHopper *self = (BugHopper *)comp;
    
    GameData *data = (GameData*)comp_get_scene_manager(self)->data;
    Random *random = data->mechanics_random;
    
    self->rt_normal = render_texture_create_with_rotated(get_image("Grasshopper-1.png"), self->angle);
    self->rt_alert = render_texture_create_with_rotated(get_image("Grasshopper-A.png"), self->angle);

    self->eat_distance = 35.f;
    self->far_detect_distance = 130.f + random_next_float_limit(random, 50.f);
    self->detect_distance = 100.f + random_next_float_limit(random, 20.f);
    self->alert_distance = 40.f + random_next_float_limit(random, 30.f);
    self->far_detect_speed = 4.0f + random_next_float_limit(random, 3.5f);
    self->detect_speed = 0.9f + random_next_float_limit(random, 0.4f);
    self->alert_time = 0.12f + random_next_float_limit(random, 0.1f);
        
    Sprite *parent = (Sprite *)comp_get_parent(self);
    sprite_set_image(parent, self->rt_normal->image);
}

void bug_hopper_added(GameObjectComponent *comp)
{
    BugHopper *self = (BugHopper *)comp;
}

static GameObjectComponentType BugHopperComponentType = {
    { { "BugHopper", &bug_hopper_destroy, &bug_hopper_describe } },
    &bug_hopper_added,
    NULL,
    &bug_hopper_start,
    &bug_hopper_update,
    &bug_hopper_fixed_update
};

BugHopper *bug_hopper_create(GameObject *gecko_head, float angle, context_callback_t *eaten_callback, void *eaten_callback_context)
{
    BugHopper *self = (BugHopper *)comp_alloc(sizeof(BugHopper));
    
    self->w_type = &BugHopperComponentType;
    self->w_gecko_head = gecko_head;
    self->angle = angle;
    
    self->eaten_callback = eaten_callback;
    self->eaten_callback_context = eaten_callback_context;
    
    return self;
}
