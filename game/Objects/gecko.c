#include "gecko.h"
#include <float.h>
#include <math.h>

#define PREV_POS_COUNT 30

#define GECKO_DEBUG_DRAW
#undef GECKO_DEBUG_DRAW

typedef struct FloatPair {
    float a;
    float b;
} FloatPair;

FloatPair to_same_half_circle_degrees(float a, float b)
{
    if (a - b > 180.f) {
        b += 360.f;
    }
    if (a - b < -180.f) {
        a += 360.f;
    }
    
    return (FloatPair){a, b};
}

FloatPair to_same_half_circle_radians(float a, float b)
{
    if (a - b > (float)M_PI) {
        b += (float)(M_PI * 2);
    }
    if (a - b < -(float)M_PI) {
        a += (float)(M_PI * 2);
    }
    
    return (FloatPair){a, b};
}

typedef struct GeckoFoot {
    Sprite *w_parent;
    CacheSprite *w_foot;
    Sprite *w_shin;
    Sprite *w_thigh;
    Vector2D start;
    Vector2D position;
    float thigh_length;
    float shin_length;
    float max_length_sq;
    float min_length_sq;
    float target_angle;
    float target_length;
    float knee_distance_ratio;
} GeckoFoot;

typedef struct GeckoCharacter {
    GAME_OBJECT_COMPONENT;
    Vector2D *prev_positions;
    DebugDraw *w_debug;
    ArrayList *sprites;
    void *w_movement_callback_context;
    gecko_movement_callback *movement_callback;
    GeckoFoot feet[4];
    float movement_speed;
    float move_accumulated;
    float current_direction;
    uint8_t prev_position_index;
    bool moved;
} GeckoCharacter;

typedef struct GeckoPart {
    float length;
    float next;
    float breahe_effect;
    char *image;
} GeckoPart;

GeckoPart gecko_parts[] = {
    { 13.f, 8.f, 0.0f, "Gecko_Neck.png" },
    { 18.f, 15.f, 0.5f, "Gecko_Back-1.png" },
    { 17.f, 12.f, 1.0f, "Gecko_Back-2.png" },
    { 21.f, 16.f, 0.0f, "Gecko_Butt.png" },
    { 17.f, 14.f, 0.0f, "Gecko_Tail-1.png" },
    { 15.f, 12.f, 0.0f, "Gecko_Tail-2.png" },
    { 20.f, 17.f, 0.0f, "Gecko_Tail-3.png" }
};

void gecko_char_destroy(void *comp)
{
    GeckoCharacter *self = (GeckoCharacter *)comp;
    platform_free(self->prev_positions);
    destroy(self->sprites);
    comp_destroy(comp);
}

char *gecko_char_describe(void *comp)
{
    return comp_describe(comp);
}

void gecko_char_added(GameObjectComponent *comp)
{
    
}

void gecko_set_part_positions(GeckoCharacter *self)
{
#ifdef ENABLE_PROFILER
    profiler_start_segment("Calculate gecko part positions");
#endif
    Sprite *head = (Sprite*)comp_get_parent(self);

    Vector2D pos = head->position;
    int pos_index = self->prev_position_index;
    const int count = sizeof gecko_parts / sizeof gecko_parts[0];
    for (int i = 0; i < count; ++i) {
        GeckoPart part = gecko_parts[i];
        float needed_distance_sq = part.length * part.length;
        float distance = vec_length_sq(vec_vec_subtract(self->prev_positions[pos_index], pos));
        while (distance < needed_distance_sq) {
            pos_index = pos_index == 0 ? PREV_POS_COUNT - 1 : pos_index - 1;
            distance = vec_length_sq(vec_vec_subtract(self->prev_positions[pos_index], pos));
        }
        Vector2D line = vec_normalize(vec_vec_subtract(self->prev_positions[pos_index], pos));
        Vector2D center = vec_scale(line, part.length / 2);
        
        Sprite *sprite = list_get(self->sprites, i);
        sprite->position = vec_vec_add(pos, center);
        
        float angle = atan2f(center.y, center.x);
        sprite->rotation = angle + (float)M_PI;
        
        pos = vec_vec_add(pos, vec_scale(line, part.next));
    }
        
    GameObject *scene = go_get_root_ancestor(head);

    for (int i = 0; i < 4; ++i) {
        int angle_multiplier = i % 2 == 0 ? 1 : -1;
        int knee_multiplier = angle_multiplier * (i < 2 ? 1 : -1);

        Sprite *parent = self->feet[i].w_parent;
        float startAngle = parent->rotation + (float)M_PI_2 * angle_multiplier;
        self->feet[i].start = vec_vec_add(parent->position, vec_scale(vec(cosf(startAngle), sinf(startAngle)), 10.f));
        
        Vector2D line = vec_vec_subtract(self->feet[i].position, self->feet[i].start);
        float distance_sq = vec_length_sq(line);
        FloatPair angles = to_same_half_circle_radians(atan2f(line.y, line.x), parent->rotation);
        float difference = i % 2 == 0 ? angles.a - angles.b : angles.b - angles.a;

        bool reposition = false;
        if (distance_sq > self->feet[i].max_length_sq || distance_sq < self->feet[i].min_length_sq || difference < 0.f) {
            float angle = parent->rotation + self->feet[i].target_angle * angle_multiplier;
            self->feet[i].position = vec_vec_add(parent->position, vec_scale(vec(cosf(angle), sinf(angle)), self->feet[i].target_length));
            self->feet[i].w_foot->position = self->feet[i].position;
            line = vec_vec_subtract(self->feet[i].position, self->feet[i].start);
            reposition = true;
        }
        
        float line_length = vec_length(line);
        float knee_length = line_length * self->feet[i].knee_distance_ratio;
        float thigh_length = self->feet[i].thigh_length;
        float dist_b = sqrtf(thigh_length * thigh_length - knee_length * knee_length);
        Vector2D line_unit = vec(line.x / line_length, line.y / line_length);
        Vector2D b_unit = vec(-knee_multiplier * line_unit.y, knee_multiplier * line_unit.x);
        Vector2D knee_point = vec_vec_add(vec_scale(line_unit, knee_length), vec_scale(b_unit, dist_b));
        
        float thigh_angle = atan2f(knee_point.y, knee_point.x) + (float)(M_PI_2) - parent->rotation;
        Vector2D knee_to_feet = vec_vec_subtract(self->feet[i].position, vec_vec_add(knee_point, self->feet[i].start));
        float shin_angle = atan2f(knee_to_feet.y, knee_to_feet.x) + (float)(M_PI_2) - parent->rotation - thigh_angle;
        
        self->feet[i].w_thigh->rotation = thigh_angle;
        self->feet[i].w_shin->rotation = shin_angle;
        
        if (reposition) {
            cache_sprite_set_rotated(self->feet[i].w_foot, i < 2 ? go_rotation_in_ancestor(self->feet[i].w_shin, scene) : go_rotation_in_ancestor(self->feet[i].w_thigh, scene));
        }
        
#ifdef GECKO_DEBUG_DRAW
        debugdraw_line(self->w_debug, self->feet[i].start, vec_vec_add(knee_point, self->feet[i].start));
        debugdraw_line(self->w_debug, self->feet[i].position, vec_vec_add(knee_point, self->feet[i].start));
#endif
    }
    
#ifdef ENABLE_PROFILER
    profiler_end_segment();
#endif
}

void gecko_char_start(GameObjectComponent *comp)
{
    GeckoCharacter *self = (GeckoCharacter *)comp;
    Sprite *head = (Sprite*)comp_get_parent(self);
    Controls controls = comp_get_scene_manager(self)->controls;

    const float dir_radians = controls.crank * (float)(M_PI) / 180.f - (float)(M_PI_2);
    const float step = 4.f;
    
    head->rotation = dir_radians;
    
    const float dir = dir_radians + (float)(M_PI);

    for (int i = 0; i < PREV_POS_COUNT; ++i) {
        Vector2D pos = vec_vec_add(head->position, vec_scale(vec_angle(dir), step * i));
        self->prev_positions[PREV_POS_COUNT - i - 1] = pos;
    }
    
    GameObject *parent_scene = go_get_parent(head);
    const int count = sizeof gecko_parts / sizeof gecko_parts[0];
    for (int i = 0; i < count; ++i) {
        Sprite *sprite = sprite_create(gecko_parts[i].image);
        sprite->anchor = vec(0.5f, 0.5f);
        go_add_child(parent_scene, sprite);
        sprite->draw_mode = drawmode_rotate;
        go_set_z_order(sprite, 10 + i);
        float breathe = gecko_parts[i].breahe_effect;
        if (breathe > 0.f) {
            go_add_component(sprite, act_create(action_repeat_create(action_sequence_create(({
                ArrayList *list = list_create();
                list_add(list, action_delay_create(0.3f));
                list_add(list, action_ease_out_create(action_scale_to_create(vec(1.0f + 0.15f * breathe, 1.0f + 0.15f * breathe), 0.6f)));
                list_add(list, action_ease_out_create(action_scale_to_create(vec(1.0f, 1.0f), 0.9f)));
                list;
            })),0)));
            sprite->draw_mode = drawmode_rotate_and_scale;
        }
        list_add(self->sprites, sprite);
    }
    
    self->feet[0].w_parent = list_get(self->sprites, 0);
    self->feet[1].w_parent = list_get(self->sprites, 0);
    self->feet[2].w_parent = list_get(self->sprites, 3);
    self->feet[3].w_parent = list_get(self->sprites, 3);

    for (int i = 0; i < 4; ++i) {
        int multiplier = i % 2 == 0 ? 1 : -1;
        Sprite *parent = self->feet[i].w_parent;
        
        if (i < 2) {
            self->feet[i].w_thigh = sprite_create("Gecko_Front-Leg.png");
            self->feet[i].w_shin = sprite_create("Gecko_Front-Shin.png");
            self->feet[i].w_foot = cache_sprite_create_rotated("Gecko_Front-Foot.png", 0, vec(0.5f, 0.6f));
            self->feet[i].thigh_length = 13.f;
            self->feet[i].shin_length = 15.f;
        } else {
            self->feet[i].w_thigh = sprite_create("Gecko_Rear-Leg.png");
            self->feet[i].w_shin = sprite_create("Gecko_Rear-Shin.png");
            self->feet[i].w_foot = cache_sprite_create_rotated("Gecko_Rear-Foot.png", 0, vec(0.5f, 0.6f));
            self->feet[i].thigh_length = 17.f;
            self->feet[i].shin_length = 16.f;
        }
        self->feet[i].w_thigh->draw_mode = drawmode_rotate;
        self->feet[i].w_shin->draw_mode = drawmode_rotate;
        
        float full_length = self->feet[i].thigh_length + self->feet[i].shin_length;
        float min_length = max(self->feet[i].thigh_length, self->feet[i].shin_length) - min(self->feet[i].thigh_length, self->feet[i].shin_length);
        self->feet[i].max_length_sq = full_length * full_length;
        self->feet[i].min_length_sq = min_length * min_length;
        
        self->feet[i].w_thigh->anchor = vec(0.5f, 1.f);
        self->feet[i].w_shin->anchor = vec(0.5f, 0.92f);
        //self->feet[i].w_foot->anchor = vec(0.5f, 0.6f);
        //self->feet[i].w_thigh->active = false;

        go_add_child(parent, self->feet[i].w_thigh);
        go_set_z_order(self->feet[i].w_thigh, -1);
        go_add_child(self->feet[i].w_thigh, self->feet[i].w_shin);
        go_add_child(parent_scene, self->feet[i].w_foot);
        self->feet[i].w_shin->position = vec(0.f, -self->feet[i].thigh_length);
        
        self->feet[i].target_angle = M_PI * 0.3f;
        self->feet[i].target_length = full_length * 1.2f;

        float startAngle = parent->rotation + (float)(M_PI_2) * multiplier;
        float footAngle = parent->rotation + (float)(M_PI_2) * multiplier;
        self->feet[i].start = vec_scale(vec(cosf(startAngle), sinf(startAngle)), 30.f);
        self->feet[i].position = vec_scale(vec(cosf(footAngle), sinf(footAngle)), 70.f);
        self->feet[i].knee_distance_ratio = self->feet[i].thigh_length / full_length;
        
        self->feet[i].w_thigh->position = vec(0.f, 10.f * multiplier);
    }
    
    gecko_set_part_positions(self);
}

void gecko_char_update(GameObjectComponent *comp, Float dt)
{
    GeckoCharacter *self = (GeckoCharacter *)comp;
#ifdef GECKO_DEBUG_DRAW
    debugdraw_clear(self->w_debug);
    
    for (int i = PREV_POS_COUNT - 1; i > 0 ; --i) {
         debugdraw_line(self->w_debug, self->prev_positions[(i + self->prev_position_index) % PREV_POS_COUNT], self->prev_positions[(i + 1 + self->prev_position_index) % PREV_POS_COUNT]);
    }
    for (int i = 0; i < 4 ; ++i) {
        debugdraw_line(self->w_debug, self->feet[i].start, self->feet[i].position);
    }
#endif

    if (self->moved) {
        gecko_set_part_positions(self);
        self->moved = false;
    }
}

#define MAX_MOVE_PER_FRAME 40.f

void gecko_move(GeckoCharacter *self, float crankChange, float crank)
{
    Sprite *head = (Sprite*)comp_get_parent(self);
    
    float dir_change = crankChange;
    
    if (fabsf(dir_change) > MAX_MOVE_PER_FRAME) {
        dir_change = MAX_MOVE_PER_FRAME * float_sign(dir_change);
    }
    self->current_direction = self->current_direction + dir_change;
    
    const float movement = fabsf(dir_change) * self->movement_speed;
    const float dir_radians = self->current_direction * (float)M_PI / 180 - (float)(M_PI_2);
    const float min_move = 4.f;
    
    head->rotation = dir_radians;
    head->position.x += cosf(dir_radians) * movement;
    head->position.y += sinf(dir_radians) * movement;
    if (movement + self->move_accumulated < min_move) {
        self->move_accumulated += movement;
    } else {
        self->move_accumulated = 0.f;
        self->prev_position_index = (self->prev_position_index + 1) % PREV_POS_COUNT;
        self->prev_positions[self->prev_position_index] = head->position;
    }
    
    self->movement_callback(movement, head->position, dir_radians, self->w_movement_callback_context);
    
    self->moved = true;
}

void gecko_char_fixed_update(GameObjectComponent *comp, Float dt)
{
    GeckoCharacter *self = (GeckoCharacter *)comp;
    Controls controls = comp_get_scene_manager(self)->controls;

    FloatPair pair = to_same_half_circle_degrees(self->current_direction, controls.crank);
    float prev_crank = pair.a;
    float curr_crank = pair.b;

    const float crankChange = curr_crank - prev_crank;
    
    if (crankChange != 0.f) {
        gecko_move(self, crankChange, curr_crank);
    }
}

static GameObjectComponentType GeckoCharacterComponentType =
    go_component_type("GeckoCharacter",
                      &gecko_char_destroy,
                      &gecko_char_describe,
                      &gecko_char_added,
                      NULL,
                      &gecko_char_start,
                      &gecko_char_update,
                      &gecko_char_fixed_update
                      );

GeckoCharacter *gecko_char_create(DebugDraw *debug, gecko_movement_callback movement_callback, void *movement_callback_context)
{
    GeckoCharacter *self = (GeckoCharacter *)comp_alloc(sizeof(GeckoCharacter));
    
    self->w_type = &GeckoCharacterComponentType;
    self->movement_speed = 0.7f;
    self->prev_positions = platform_calloc(PREV_POS_COUNT, sizeof(Vector2D));
    self->prev_position_index = PREV_POS_COUNT - 1;
    self->move_accumulated = 0;
    self->sprites = list_create_with_weak_references();
    self->w_debug = debug;
    self->moved = false;
    
    self->movement_callback = movement_callback;
    self->w_movement_callback_context = movement_callback_context;
    
    return self;
}
