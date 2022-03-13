#include "test_scene.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

typedef struct {
    GAME_OBJECT;
    Controls previous_controls;
    Vector2D offset;
    int32_t step;
} TestScene;

void test_scene_update(GameObject *scene, Number dt_ms)
{
    TestScene *self = (TestScene *)scene;

    LOG("Update test scene");

    self->step += nb_mul(nb_from_int(20), dt_ms) / 1000;
    if (self->step >= nb_from_int(256)) {
        self->step -= nb_from_int(256);
    }

    Controls controls = go_get_scene_manager(self)->controls;

    Number move_step = nb_mul(nb_from_int(100), dt_ms) / 1000;

    if (controls.button_left) {
        self->offset.x -= move_step;
    }
    if (controls.button_right) {
        self->offset.x += move_step;
    }
    if (controls.button_up) {
        self->offset.y -= move_step;
    }
    if (controls.button_down) {
        self->offset.y += move_step;
    }
}

void test_scene_fixed_update(GameObject *scene, Number dt_ms)
{
}

void test_scene_render(GameObject *scene, RenderContext *ctx)
{
    TestScene *self = (TestScene *)scene;

    Image *dither = get_image("dither_blue");
    ImageData *xor_data = image_data_xor_texture((Size2DInt){ SCREEN_WIDTH, SCREEN_HEIGHT }, (Vector2DInt){ nb_to_int(self->offset.x), nb_to_int(self->offset.y) }, 0);
    Image *xor = image_from_data(xor_data);
    image_render_dither(ctx, xor, dither, (Vector2DInt){ 0, 0 }, (Vector2DInt){ nb_to_int(self->step), nb_to_int(self->step) / 4 }, 0);
    destroy(xor);
    destroy(xor_data);
}

void test_scene_initialize(GameObject *scene)
{
    TestScene *self = (TestScene *)scene;
}

void test_scene_start(GameObject *scene)
{
    
}

void test_scene_destroy(void *scene)
{
    go_destroy(scene);
}

char *test_scene_describe(void *scene)
{
    return platform_strdup("[]");
}

static GameObjectType TestSceneType = {
    { { "TestScene", &test_scene_destroy, &test_scene_describe } },
    &test_scene_initialize,
    NULL,
    &test_scene_start,
    &test_scene_update,
    &test_scene_fixed_update,
    &test_scene_render
};

GameObject *test_scene_create()
{
    GameObject *p_scene = go_alloc(sizeof(TestScene));
    p_scene->w_type = &TestSceneType;
    
    return p_scene;
}
