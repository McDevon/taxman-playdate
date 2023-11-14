#include "bug_fly.h"
#include "gecko_scene.h"
#include "life_timer.h"
#include "game_data.h"
#include <math.h>

typedef struct BugFly {
    GAME_OBJECT_COMPONENT;
    GameObject *w_gecko_head;
    RenderTexture *rt_normal;
    RenderTexture *rt_alert;
    RenderTexture *rt_fly_1;
    RenderTexture *rt_fly_2;
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
    float flying_speed;
    float max_flying_speed;
    float fly_timer;
} BugFly;

void bug_fly_update(GameObjectComponent *comp, float dt)
{
}

void bug_fly_fly(BugFly *self) {
    Sprite *parent = (Sprite *)comp_get_parent(self);

    go_set_z_order(comp_get_parent(self), 10);
    self->flying_speed = 3.f;
    
    self->rt_fly_1 = render_texture_create_with_rotated(get_image("Fly-2.png"), self->angle);
    self->rt_fly_2 = render_texture_create_with_rotated(get_image("Fly-3.png"), self->angle);

    float frame_time = 0.0666666;
    Animator *animator = animator_create();
    ArrayList *anim_fly = list_create();
    list_add(anim_fly, anim_frame_create_with_image(self->rt_fly_1->image, frame_time));
    list_add(anim_fly, anim_frame_create_with_image(self->rt_fly_2->image, frame_time));
    animator_add_animation(animator, "fly", anim_fly);
    
    go_add_component(parent, animator);
    go_add_component(parent, life_timer_create(5.f, false));
    
    animator_set_animation(animator, "fly");
}

void bug_fly_fixed_update(GameObjectComponent *comp, float dt)
{
    BugFly *self = (BugFly *)comp;
    Sprite *parent = (Sprite *)comp_get_parent(self);
    
    if (self->flying_speed > 0.f) {
        float cos_angle = cosf(self->angle);
        float sin_angle = sinf(self->angle);
        
        self->fly_timer += dt;
        
        parent->position = vec_vec_add(parent->position,
                                       vec(cos_angle * self->flying_speed,
                                           sin_angle * self->flying_speed));
        
        float oscillation_offset = sinf(self->fly_timer * 6.f * (float)M_PI) * self->fly_timer * 20.f;
        Vector2D oscillation = vec(sin_angle * oscillation_offset, cos_angle * oscillation_offset);
        
        parent->position = vec_vec_add(parent->position, oscillation);
        
        self->flying_speed += 50.f * dt;
        return;
    }
    
    float distance = vec_length(vec_vec_subtract(self->w_gecko_head->position, parent->position));
    
    if (distance > 500.f) {
        go_schedule_destroy(parent);
    }
    
    if (distance < self->eat_distance) {
        go_add_child(go_get_parent(parent), ({
            Label *label = label_create("font4", "Om nom!");
            
            label->anchor.x = 0.5f;
            label->anchor.y = 1.f;
                        
            label->position.x = SCREEN_WIDTH / 2.f;
            label->position.y = SCREEN_HEIGHT / 2.f - 15.f;
            
            label->ignore_camera = true;
            
            go_set_z_order(label, 20);
                        
            ArrayList *list = list_create();
            list_add(list, action_ease_out_create(action_move_by_create(vec(0.f, -30.f), 1.f)));
            list_add(list, action_destroy_create());
            
            go_add_component(label, act_create(action_sequence_create(list)));
            
            label;
        }));
        
        go_schedule_destroy(parent);
        
        self->eaten_callback(self->eaten_callback_context);

        return;
    }
    
    if (self->alert_timer > 0.f) {
        self->alert_timer -= dt;
        if (self->alert_timer < 0.f) {
            bug_fly_fly(self);
        }
    } else {
        float gecko_speed = gecko_scene_get_gecko_speed(go_get_root_ancestor(comp_get_parent(self)));
        
        if ((distance < self->alert_distance)
            || (distance < self->detect_distance && gecko_speed > self->detect_speed)
            || (distance < self->far_detect_distance && gecko_speed > self->far_detect_speed)) {
            self->alert_timer = self->alert_time;
            sprite_set_image(parent, self->rt_alert->image);
            
            go_add_child(go_get_parent(parent), ({
                Label *label = label_create("font4", "!?");
                
                label->anchor.x = 0.5f;
                label->anchor.y = 1.f;
                                
                label->position.x = parent->position.x;
                label->position.y = parent->position.y - 15.f;
                
                go_set_z_order(label, 20);
                            
                ArrayList *list = list_create();
                list_add(list, action_ease_out_create(action_move_by_create(vec(0.f, -20.f), 0.6f)));
                list_add(list, action_destroy_create());
                
                go_add_component(label, act_create(action_sequence_create(list)));
                
                label;
            }));
        }
    }
}

void bug_fly_destroy(void *obj)
{
    BugFly *self = (BugFly *)obj;

    if (self->rt_alert) {
        destroy(self->rt_alert);
    }
    if (self->rt_normal) {
        destroy(self->rt_normal);
    }
    if (self->rt_fly_1) {
        destroy(self->rt_fly_1);
    }
    if (self->rt_fly_2) {
        destroy(self->rt_fly_2);
    }
    
    comp_destroy(obj);
}

char *bug_fly_describe(void *obj)
{
    return comp_describe(obj);
}

void bug_fly_start(GameObjectComponent *comp)
{
    BugFly *self = (BugFly *)comp;
    
    GameData *data = (GameData*)comp_get_scene_manager(self)->data;
    Random *random = data->mechanics_random;
    
    self->rt_normal = render_texture_create_with_rotated(get_image("Fly-1.png"), self->angle);
    self->rt_alert = render_texture_create_with_rotated(get_image("Fly-A.png"), self->angle);

    self->eat_distance = 30.f;
    self->far_detect_distance = 150.f + random_next_float_limit(random, 50.f);
    self->detect_distance = 100.f + random_next_float_limit(random, 20.f);
    self->alert_distance = 62.f + random_next_float_limit(random, 15.f);
    self->far_detect_speed = 2.0f + random_next_float_limit(random, 1.5f);
    self->detect_speed = 0.3f + random_next_float_limit(random, 0.2f);
    self->alert_time = 0.20f + random_next_float_limit(random, 0.25f);
    self->fly_timer = 0.f;
        
    Sprite *parent = (Sprite *)comp_get_parent(self);
    sprite_set_image(parent, self->rt_normal->image);
}

void bug_fly_added(GameObjectComponent *comp)
{
    BugFly *self = (BugFly *)comp;
}

static GameObjectComponentType BugFlyComponentType = {
    { { "BugFly", &bug_fly_destroy, &bug_fly_describe } },
    &bug_fly_added,
    NULL,
    &bug_fly_start,
    &bug_fly_update,
    &bug_fly_fixed_update
};

BugFly *bug_fly_create(GameObject *gecko_head, float angle, context_callback_t *eaten_callback, void *eaten_callback_context)
{
    BugFly *self = (BugFly *)comp_alloc(sizeof(BugFly));
    
    self->w_type = &BugFlyComponentType;
    self->w_gecko_head = gecko_head;
    self->angle = angle;
    
    self->eaten_callback = eaten_callback;
    self->eaten_callback_context = eaten_callback_context;
    
    return self;
}
