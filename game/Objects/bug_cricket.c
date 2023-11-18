#include "bug_cricket.h"
#include "gecko_scene.h"
#include "life_timer.h"
#include "game_data.h"
#include <math.h>

typedef struct BugCricket {
    GAME_OBJECT_COMPONENT;
    GameObject *w_gecko_head;
    RenderTexture *rt_normal;
    RenderTexture *rt_alert;
    RenderTexture *rt_walk_1;
    RenderTexture *rt_walk_2;
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
    bool walking;
} BugCricket;

void bug_cricket_update(GameObjectComponent *comp, float dt)
{
}

void bug_cricket_new_angle_walk(BugCricket *self, float angle)
{
    self->angle = angle;
    if (self->rt_walk_1) {
        destroy(self->rt_walk_1);
    }
    if (self->rt_walk_2) {
        destroy(self->rt_walk_2);
    }
    self->rt_walk_1 = render_texture_create_with_rotated(get_image("Cricket-2.png"), angle);
    self->rt_walk_2 = render_texture_create_with_rotated(get_image("Cricket-3.png"), angle);
}

void bug_cricket_new_angle_stay(BugCricket *self, float angle)
{
    self->angle = angle;
    destroy(self->rt_normal);
    destroy(self->rt_alert);
    self->rt_normal = render_texture_create_with_rotated(get_image("Cricket-1.png"), angle);
    self->rt_alert = render_texture_create_with_rotated(get_image("Cricket-A.png"), angle);
}

void bug_cricket_stop(void *obj, void *context)
{
    BugCricket *self = (BugCricket *)context;
    
    bug_cricket_new_angle_stay(self, self->angle);
    
    Animator *animator = (Animator *)comp_get_component(self, &SpriteAnimationComponentType);
    ArrayList *anim_idle = list_create();
    list_add(anim_idle, anim_frame_create_with_image(self->rt_normal->image, 0.f));
    animator_add_animation(animator, "idle", anim_idle);
    
    animator_set_animation(animator, "idle");
    
    self->walking = false;
}

void bug_cricket_walk(BugCricket *self) {
    Sprite *parent = (Sprite *)comp_get_parent(self);
    
    GameData *data = (GameData*)comp_get_scene_manager(self)->data;
    Random *random = data->mechanics_random;
    
    Vector2D distance = vec_vec_subtract(parent->position, self->w_gecko_head->position);
    float angle = atan2f(distance.y, distance.x) + (float)M_PI * (-0.3f + random_next_float_limit(random, 0.6f));
    bug_cricket_new_angle_walk(self, angle);

    float frame_time = 0.1;
    Animator *animator = (Animator *)comp_get_component(self, &SpriteAnimationComponentType);
    if (animator != NULL) {
        comp_remove_from_parent(animator);
        destroy(animator);
        animator = NULL;
    }
    animator = animator_create();
    go_add_component(parent, animator);
    
    ArrayList *anim_walk = list_create();
    list_add(anim_walk, anim_frame_create_with_image(self->rt_walk_1->image, frame_time));
    list_add(anim_walk, anim_frame_create_with_image(self->rt_walk_2->image, frame_time));
    animator_add_animation(animator, "walk", anim_walk);
    
    animator_set_animation(animator, "walk");
    
    self->walking = true;
    
    float walk_distance = 100.f + random_next_float_limit(random, 150.f);
    float speed = 120.f + random_next_float_limit(random, 50.f);
    
    float time = walk_distance / speed;
    
    go_add_component(parent, act_create(action_sequence_create(({
        ArrayList *list = list_create();
        list_add(list, action_ease_in_out_create(action_move_by_create(vec(cosf(self->angle) * walk_distance, sinf(self->angle) * walk_distance), time)));
        list_add(list, action_callback_create(&bug_cricket_stop, self));
        list;
    }))));
}

void bug_cricket_fixed_update(GameObjectComponent *comp, float dt)
{
    BugCricket *self = (BugCricket *)comp;
    Sprite *parent = (Sprite *)comp_get_parent(self);
    
    float distance = vec_length(vec_vec_subtract(self->w_gecko_head->position, parent->position));
    
    if (distance > 500.f) {
        go_schedule_destroy(parent);
    }
    
    if (distance < self->eat_distance) {
        go_add_child(go_get_parent(parent), ({
            Label *label = label_create("font4", "Crunch!");
            
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
    
    if (self->walking) {
        return;
    }
    
    if (self->alert_timer > 0.f) {
        self->alert_timer -= dt;
        if (self->alert_timer < 0.f) {
            bug_cricket_walk(self);
        }
    } else {
        float gecko_speed = gecko_scene_get_gecko_speed(go_get_root_ancestor(comp_get_parent(self)));
        
        if ((distance < self->alert_distance)
            || (distance < self->detect_distance && gecko_speed > self->detect_speed)
            || (distance < self->far_detect_distance && gecko_speed > self->far_detect_speed)) {
            self->alert_timer = self->alert_time;
            sprite_set_image(parent, self->rt_alert->image);
        }
    }
}

void bug_cricket_destroy(void *obj)
{
    BugCricket *self = (BugCricket *)obj;

    if (self->rt_normal) {
        destroy(self->rt_normal);
    }
    if (self->rt_alert) {
        destroy(self->rt_alert);
    }
    if (self->rt_walk_1) {
        destroy(self->rt_walk_1);
    }
    if (self->rt_walk_2) {
        destroy(self->rt_walk_2);
    }

    comp_destroy(obj);
}

char *bug_cricket_describe(void *obj)
{
    return comp_describe(obj);
}

void bug_cricket_start(GameObjectComponent *comp)
{
    BugCricket *self = (BugCricket *)comp;
    
    GameData *data = (GameData*)comp_get_scene_manager(self)->data;
    Random *random = data->mechanics_random;
    
    self->rt_normal = render_texture_create_with_rotated(get_image("Cricket-1.png"), self->angle);
    self->rt_alert = render_texture_create_with_rotated(get_image("Cricket-A.png"), self->angle);

    self->eat_distance = 30.f;
    self->far_detect_distance = 150.f + random_next_float_limit(random, 50.f);
    self->detect_distance = 100.f + random_next_float_limit(random, 20.f);
    self->alert_distance = 40.f + random_next_float_limit(random, 30.f);
    self->far_detect_speed = 4.0f + random_next_float_limit(random, 3.5f);
    self->detect_speed = 0.8f + random_next_float_limit(random, 0.5f);
    self->alert_time = 0.10f + random_next_float_limit(random, 0.1f);
        
    Sprite *parent = (Sprite *)comp_get_parent(self);
    sprite_set_image(parent, self->rt_normal->image);
}

void bug_cricket_added(GameObjectComponent *comp)
{
    BugCricket *self = (BugCricket *)comp;
}

static GameObjectComponentType BugCricketComponentType = {
    { { "BugCricket", &bug_cricket_destroy, &bug_cricket_describe } },
    &bug_cricket_added,
    NULL,
    &bug_cricket_start,
    &bug_cricket_update,
    &bug_cricket_fixed_update
};

BugCricket *bug_cricket_create(GameObject *gecko_head, float angle, context_callback_t *eaten_callback, void *eaten_callback_context)
{
    BugCricket *self = (BugCricket *)comp_alloc(sizeof(BugCricket));
    
    self->w_type = &BugCricketComponentType;
    self->w_gecko_head = gecko_head;
    self->angle = angle;
    
    self->eaten_callback = eaten_callback;
    self->eaten_callback_context = eaten_callback_context;
    
    return self;
}
