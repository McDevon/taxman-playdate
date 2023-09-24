#include "gecko.h"

#define PREV_POS_COUNT 30

typedef struct NumberPair {
    Number a;
    Number b;
} NumberPair;

NumberPair nb_to_same_half_circle_degrees(Number a, Number b)
{
    if (a - b > nb_from_int(180)) {
        b += nb_from_int(360);
    }
    if (a - b < -nb_from_int(180)) {
        a += nb_from_int(360);
    }
    
    return (NumberPair){a, b};
}

NumberPair nb_to_same_half_circle_radians(Number a, Number b)
{
    if (a - b > nb_pi) {
        b += nb_pi_times_two;
    }
    if (a - b < -nb_pi) {
        a += nb_pi_times_two;
    }
    
    return (NumberPair){a, b};
}

typedef struct GeckoFoot {
    Sprite *w_parent;
    Sprite *w_foot;
    Sprite *w_shin;
    Sprite *w_thigh;
    Vector2D start;
    Vector2D position;
    Number thigh_length;
    Number shin_length;
    Number max_length_sq;
    Number min_length_sq;
    Number target_angle;
    Number target_length;
    Number knee_distance_ratio;
} GeckoFoot;

typedef struct GeckoCharacter {
    GAME_OBJECT_COMPONENT;
    Vector2D *prev_positions;
    DebugDraw *w_debug;
    ArrayList *sprites;
    GeckoFoot feet[4];
    Controls previous_controls;
    Number movement_speed;
    Number move_accumulated;
    uint8_t prev_position_index;
    bool moved;
} GeckoCharacter;

typedef struct GeckoPart {
    Number length;
    Number next;
    float breahe_effect;
    char *image;
} GeckoPart;

#ifdef NUMBER_TYPE_FIXED_POINT
#define NB_FROM_INT(value) (value * 1024)
#else
#define NB_FROM_INT(value) ((float)value)
#endif

GeckoPart gecko_parts[] = {
    { NB_FROM_INT(13), NB_FROM_INT(8), 0.0f, "Gecko_Neck.png" },
    { NB_FROM_INT(18), NB_FROM_INT(15), 0.5f, "Gecko_Back-1.png" },
    { NB_FROM_INT(17), NB_FROM_INT(12), 1.0f, "Gecko_Back-2.png" },
    { NB_FROM_INT(21), NB_FROM_INT(16), 0.0f, "Gecko_Butt.png" },
    { NB_FROM_INT(17), NB_FROM_INT(14), 0.0f, "Gecko_Tail-1.png" },
    { NB_FROM_INT(15), NB_FROM_INT(12), 0.0f, "Gecko_Tail-2.png" },
    { NB_FROM_INT(20), NB_FROM_INT(17), 0.0f, "Gecko_Tail-3.png" }
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
        Number needed_distance_sq = nb_mul(part.length, part.length);
        Number distance = vec_length_sq(vec_vec_subtract(self->prev_positions[pos_index], pos));
        while (distance < needed_distance_sq) {
            pos_index = pos_index == 0 ? PREV_POS_COUNT - 1 : pos_index - 1;
            distance = vec_length_sq(vec_vec_subtract(self->prev_positions[pos_index], pos));
        }
        Vector2D line = vec_normalize(vec_vec_subtract(self->prev_positions[pos_index], pos));
        Vector2D center = vec_scale(line, part.length / 2);
        
        Sprite *sprite = list_get(self->sprites, i);
        sprite->position = vec_vec_add(pos, center);
        
        Number angle = nb_atan2(center.y, center.x);
        sprite->rotation = angle + nb_pi;
        
        pos = vec_vec_add(pos, vec_scale(line, part.next));
    }
        
    GameObject *scene = go_get_root_ancestor(head);

    for (int i = 0; i < 4; ++i) {
        int angle_multiplier = i % 2 == 0 ? 1 : -1;
        int knee_multiplier = angle_multiplier * (i < 2 ? 1 : -1);

        Sprite *parent = self->feet[i].w_parent;
        Number startAngle = parent->rotation + nb_pi_over_two * angle_multiplier;
        self->feet[i].start = vec_vec_add(parent->position, vec_scale(vec(nb_cos(startAngle), nb_sin(startAngle)), nb_from_int(10)));
        
        Vector2D line = vec_vec_subtract(self->feet[i].position, self->feet[i].start);
        Number distance_sq = vec_length_sq(line);
        NumberPair angles = nb_to_same_half_circle_radians(nb_atan2(line.y, line.x), parent->rotation);
        Number difference = i % 2 == 0 ? angles.a - angles.b : angles.b - angles.a;

        bool reposition = false;
        if (distance_sq > self->feet[i].max_length_sq || distance_sq < self->feet[i].min_length_sq || difference < nb_zero) {
            Number angle = parent->rotation + self->feet[i].target_angle * angle_multiplier;
            self->feet[i].position = vec_vec_add(parent->position, vec_scale(vec(nb_cos(angle), nb_sin(angle)), self->feet[i].target_length));
            self->feet[i].w_foot->position = self->feet[i].position;
            line = vec_vec_subtract(self->feet[i].position, self->feet[i].start);
            reposition = true;
        }
        
        Number line_length = vec_length(line);
        Number knee_length = nb_mul(line_length, self->feet[i].knee_distance_ratio);
        Number thigh_length = self->feet[i].thigh_length;
        Number dist_b = nb_sqrt(nb_mul(thigh_length, thigh_length) - nb_mul(knee_length, knee_length));
        Vector2D line_unit = vec(nb_div(line.x, line_length), nb_div(line.y, line_length));
        Vector2D b_unit = vec(-knee_multiplier * line_unit.y, knee_multiplier * line_unit.x);
        Vector2D knee_point = vec_vec_add(vec_scale(line_unit, knee_length), vec_scale(b_unit, dist_b));
        
        Number thigh_angle = nb_atan2(knee_point.y, knee_point.x) + nb_pi_over_two - parent->rotation;
        Vector2D knee_to_feet = vec_vec_subtract(self->feet[i].position, vec_vec_add(knee_point, self->feet[i].start));
        Number shin_angle = nb_atan2(knee_to_feet.y, knee_to_feet.x) + nb_pi_over_two - parent->rotation - thigh_angle;
        
        self->feet[i].w_thigh->rotation = thigh_angle;
        self->feet[i].w_shin->rotation = shin_angle;
        
        if (reposition) {
            self->feet[i].w_foot->rotation = i < 2 ? go_rotation_in_ancestor(self->feet[i].w_shin, scene) : go_rotation_in_ancestor(self->feet[i].w_thigh, scene);
        }
        
        //debugdraw_line(self->w_debug, self->feet[i].start, vec_vec_add(knee_point, self->feet[i].start));
        //debugdraw_line(self->w_debug, self->feet[i].position, vec_vec_add(knee_point, self->feet[i].start));
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

    const Number dir_radians = nb_mul(controls.crank, nb_pi) / 180 - nb_pi_over_two;
    const Number step = nb_from_int(4);
    
    head->rotation = dir_radians;
    
    const Number dir = dir_radians + nb_pi;

    for (int i = 0; i < PREV_POS_COUNT; ++i) {
        Vector2D pos = vec_vec_add(head->position, vec_scale(vec_angle(dir), step * i));
        self->prev_positions[PREV_POS_COUNT - i - 1] = pos;
    }
    
    GameObject *parent_scene = go_get_parent(head);
    const int count = sizeof gecko_parts / sizeof gecko_parts[0];
    for (int i = 0; i < count; ++i) {
        Sprite *sprite = sprite_create(gecko_parts[i].image);
        sprite->anchor = vec(nb_half, nb_half);
        go_add_child(parent_scene, sprite);
        sprite->draw_mode = drawmode_rotate;
        go_set_z_order(sprite, 10 + i);
        float breathe = gecko_parts[i].breahe_effect;
        if (breathe > 0.f) {
            go_add_component(sprite, act_create(action_repeat_create(action_sequence_create(({
                ArrayList *list = list_create();
                list_add(list, action_delay_create(0.3f));
                list_add(list, action_ease_out_create(action_scale_to_create(vec(nb_from_float(1.0f + 0.15f * breathe), nb_from_float(1.0f + 0.15f * breathe)), 0.6f)));
                list_add(list, action_ease_out_create(action_scale_to_create(vec(nb_one, nb_one), 0.9f)));
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
            self->feet[i].w_foot = sprite_create("Gecko_Front-Foot.png");
            self->feet[i].thigh_length = nb_from_int(13);
            self->feet[i].shin_length = nb_from_int(15);
        } else {
            self->feet[i].w_thigh = sprite_create("Gecko_Rear-Leg.png");
            self->feet[i].w_shin = sprite_create("Gecko_Rear-Shin.png");
            self->feet[i].w_foot = sprite_create("Gecko_Rear-Foot.png");
            self->feet[i].thigh_length = nb_from_int(17);
            self->feet[i].shin_length = nb_from_int(16);
        }
        self->feet[i].w_thigh->draw_mode = drawmode_rotate;
        self->feet[i].w_shin->draw_mode = drawmode_rotate;
        self->feet[i].w_foot->draw_mode = drawmode_rotate;
        
        Number full_length = self->feet[i].thigh_length + self->feet[i].shin_length;
        Number min_length = max(self->feet[i].thigh_length, self->feet[i].shin_length) - min(self->feet[i].thigh_length, self->feet[i].shin_length);
        self->feet[i].max_length_sq = nb_mul(full_length, full_length);
        self->feet[i].min_length_sq = nb_mul(min_length, min_length);
        
        self->feet[i].w_thigh->anchor = vec(nb_half, nb_one);
        self->feet[i].w_shin->anchor = vec(nb_half, nb_from_float(0.92f));
        self->feet[i].w_foot->anchor = vec(nb_half, nb_from_float(0.6f));
        //self->feet[i].w_thigh->active = false;

        go_add_child(parent, self->feet[i].w_thigh);
        go_set_z_order(self->feet[i].w_thigh, -1);
        go_add_child(self->feet[i].w_thigh, self->feet[i].w_shin);
        go_add_child(parent_scene, self->feet[i].w_foot);
        self->feet[i].w_shin->position = vec(nb_zero, -self->feet[i].thigh_length);
        
        self->feet[i].target_angle = nb_mul(nb_pi, nb_from_float(0.3f));
        self->feet[i].target_length = nb_mul(full_length, nb_from_float(1.2f));

        Number startAngle = parent->rotation + nb_pi_over_two * multiplier;
        Number footAngle = parent->rotation + nb_pi_over_two * multiplier;
        self->feet[i].start = vec_scale(vec(nb_cos(startAngle), nb_sin(startAngle)), nb_from_int(30));
        self->feet[i].position = vec_scale(vec(nb_cos(footAngle), nb_sin(footAngle)), nb_from_int(70));
        self->feet[i].knee_distance_ratio = nb_div(self->feet[i].thigh_length, full_length);
        
        self->feet[i].w_thigh->position = vec(nb_from_int(0), nb_from_int(10 * multiplier));
    }
    
    gecko_set_part_positions(self);
}

void gecko_char_update(GameObjectComponent *comp, Number dt_ms)
{
    GeckoCharacter *self = (GeckoCharacter *)comp;
    debugdraw_clear(self->w_debug);
    
    /*for (int i = PREV_POS_COUNT - 1; i > 0 ; --i) {
         debugdraw_line(self->w_debug, self->prev_positions[(i + self->prev_position_index) % PREV_POS_COUNT], self->prev_positions[(i + 1 + self->prev_position_index) % PREV_POS_COUNT]);
    }*/
    for (int i = 0; i < 4 ; ++i) {
        //debugdraw_line(self->w_debug, self->feet[i].start, self->feet[i].position);
    }

    if (self->moved) {
        gecko_set_part_positions(self);
        self->moved = false;
    }
}

void gecko_move(GeckoCharacter *self, Number crankChange, Number crank)
{
    Sprite *head = (Sprite*)comp_get_parent(self);
    
    const Number movement = nb_mul(nb_abs(crankChange), self->movement_speed);
    const Number dir_radians = nb_mul(crank, nb_pi) / 180 - nb_pi_over_two;
    const Number min_move = nb_from_int(4);
    
    head->rotation = dir_radians;
    head->position.x += nb_mul(nb_cos(dir_radians), movement);
    head->position.y += nb_mul(nb_sin(dir_radians), movement);
    if (movement + self->move_accumulated < min_move) {
        self->move_accumulated += movement;
    } else {
        self->move_accumulated = nb_zero;
        self->prev_position_index = (self->prev_position_index + 1) % PREV_POS_COUNT;
        self->prev_positions[self->prev_position_index] = head->position;
    }
    
    self->moved = true;
}

void gecko_char_fixed_update(GameObjectComponent *comp, Number dt_ms)
{
    GeckoCharacter *self = (GeckoCharacter *)comp;
    Controls controls = comp_get_scene_manager(self)->controls;

    NumberPair pair = nb_to_same_half_circle_degrees(self->previous_controls.crank, controls.crank);
    Number prev_crank = pair.a;
    Number curr_crank = pair.b;

    const Number crankChange = curr_crank - prev_crank;
    
    if (crankChange != nb_zero) {
        gecko_move(self, crankChange, curr_crank);
    }
    
    self->previous_controls = controls;
}

static GameObjectComponentType GeckoCharacterComponentType = {
    { { "GeckoCharacter", &gecko_char_destroy, &gecko_char_describe } },
    &gecko_char_added,
    NULL,
    &gecko_char_start,
    &gecko_char_update,
    &gecko_char_fixed_update
};

GeckoCharacter *gecko_char_create(DebugDraw *debug)
{
    GeckoCharacter *self = (GeckoCharacter *)comp_alloc(sizeof(GeckoCharacter));
    
    self->w_type = &GeckoCharacterComponentType;
    self->movement_speed = nb_from_double(0.7);
    self->prev_positions = platform_calloc(PREV_POS_COUNT, sizeof(Vector2D));
    self->prev_position_index = PREV_POS_COUNT - 1;
    self->move_accumulated = 0;
    self->sprites = list_create_with_weak_references();
    self->w_debug = debug;
    self->moved = false;
    
    return self;
}
