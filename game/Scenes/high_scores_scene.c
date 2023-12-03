#include "high_scores_scene.h"
#include "game_data.h"
#include "gecko_scene.h"

#define MAX_NAME_LENGTH 5
#define ENTRIES_COUNT 10
#define HIGH_SCORE_FILE "scores.dat"

typedef struct {
    int score;
    char name[MAX_NAME_LENGTH + 1];
} HighScoreEntry;

typedef struct {
    SCENE;
    HighScoreEntry entries[ENTRIES_COUNT];
    GameObject *w_score_base;
    Label *w_info_label;
    Label *w_new_high_score_label;
    int new_score_index;
} HighScoresScene;

void high_scores_scene_destroy(void *scene)
{
    HighScoresScene *self = (HighScoresScene *)scene;
    scene_destroy(self);
}

char *high_scores_scene_describe(void *scene)
{
    HighScoresScene *self = (HighScoresScene *)scene;
    return sb_string_with_format("");
}

void high_scores_scene_show_scores(void *context)
{
    HighScoresScene *self = (HighScoresScene *)context;
    
    self->w_score_base = go_create_empty();
    self->w_score_base->position = vec(SCREEN_WIDTH / 2.f, 52.f);
    self->w_score_base->ignore_camera = true;
    go_add_child(self, self->w_score_base);
    
    for (int i = 0; i < ENTRIES_COUNT; ++i) {
        Label *label_left = label_create("font4", self->entries[i].name);
        char *score_text = sb_string_with_format("%d", self->entries[i].score);
        Label *label_right = label_create("font4", score_text);
        platform_free(score_text);
        
        if (self->new_score_index == i) {
            label_left->anchor = vec(1.f, 0.5f);
            label_left->position = vec(-200.f, i * 18.f);
            label_left->draw_mode = drawmode_scale;
            
            label_right->anchor = vec(0.f, 0.5f);
            label_right->position = vec(200.f, i * 18.f);
            label_right->draw_mode = drawmode_scale;

            go_add_component(label_left, act_create(action_sequence_create(({
                ArrayList *list = list_create();
                list_add(list, action_delay_create(0.2f));
                list_add(list, action_ease_in_create(action_move_to_create(vec(-8.f, i * 18.f), 0.5f)));
                list_add(list, action_ease_out_create(action_scale_to_create(vec(0.5f, 2.f), 0.29f)));
                list_add(list, action_ease_in_create(action_scale_to_create(vec(1.f, 1.f), 0.29f)));
                list;
            }))));
            
            go_add_component(label_right, act_create(action_sequence_create(({
                ArrayList *list = list_create();
                list_add(list, action_delay_create(0.2f));
                list_add(list, action_ease_in_create(action_move_to_create(vec(8.f, i * 18.f), 0.5f)));
                list_add(list, action_ease_out_create(action_scale_to_create(vec(0.5f, 2.f), 0.29f)));
                list_add(list, action_ease_in_create(action_scale_to_create(vec(1.f, 1.f), 0.29f)));
                list;
            }))));
        } else {
            label_left->anchor = vec(1.f, 0.5f);
            label_left->position = vec(-10.f, i * 18.f);
            
            label_right->anchor = vec(0.f, 0.5f);
            label_right->position = vec(10.f, i * 18.f);
        }
    
        go_add_child(self->w_score_base, label_left);
        go_add_child(self->w_score_base, label_right);
    }
    
    go_add_child(self, ({
        Label *label = label_create("font4", "Hall of Fame");
        label->anchor = vec(0.5f, 0.f);
        label->position = vec(SCREEN_WIDTH / 2.f, 18.f);
        label->ignore_camera = true;
        label;
    }));
    
    self->w_info_label->active = false;
    if (self->w_new_high_score_label) {
        self->w_new_high_score_label->active = false;
    }
}

void high_scores_scene_save_scores(HighScoresScene *self)
{
    
}

void high_scores_scene_check_for_new_high_score(HighScoresScene *self)
{
    SceneManager *manager = go_get_scene_manager(self);
    GameData *data = (GameData *)manager->data;
    
    int latest_score = data->latest_score;
    int replace_index = -1;
    
    for (int i = 0; i < ENTRIES_COUNT; ++i) {
        if (latest_score > self->entries[i].score) {
            replace_index = i;
            break;
        }
    }
    
    if (replace_index >= 0) {
        for (int i = ENTRIES_COUNT - 1; i > replace_index; --i) {
            self->entries[i].score = self->entries[i - 1].score;
            strncpy(self->entries[i].name, self->entries[i - 1].name, MAX_NAME_LENGTH + 1);
        }
        
        self->entries[replace_index].score = latest_score;
        strncpy(self->entries[replace_index].name, "New", MAX_NAME_LENGTH + 1);
        
        self->w_new_high_score_label = ({
            Label *label = label_create("font4", "New high score!");
            
            label->anchor.x = 0.5f;
            label->anchor.y = 0.0f;
                            
            label->position.x = SCREEN_WIDTH / 2.f;
            label->position.y = SCREEN_HEIGHT + 20.f;
            
            go_add_component(label, act_create(action_sequence_create(({
                ArrayList *list = list_create();
                list_add(list, action_delay_create(0.5f));
                list_add(list, action_ease_in_create(action_move_to_create(vec(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f + 10.f), 0.6f)));
                list_add(list, action_ease_out_create(action_scale_to_create(vec(2.f, 0.5f), 0.29f)));
                list_add(list, action_ease_in_create(action_scale_to_create(vec(1.f, 1.f), 0.29f)));
                list;
            }))));

            label->ignore_camera = true;
            label->draw_mode = drawmode_scale;
            
            label;
        });
        
        self->new_score_index = replace_index;
        
        go_add_child(self, self->w_new_high_score_label);
        
        high_scores_scene_save_scores(self);
    }
        
    go_delayed_callback(self, &high_scores_scene_show_scores, 3.f);
}

void high_scores_scene_create_empty_high_scores(HighScoresScene *self)
{
    for (int i = 0; i < ENTRIES_COUNT; ++i) {
        strncpy(self->entries[i].name, "T-man", MAX_NAME_LENGTH + 1);
        self->entries[i].score = (ENTRIES_COUNT - i) * 5;
    }
    high_scores_scene_check_for_new_high_score(self);
}

void high_scores_scene_scores_file_exists(const char *file_name, bool exists, void *context)
{
    HighScoresScene *self = (HighScoresScene *)context;
    LOG("Score file %s exists: %s", file_name, exists ? "true" : "false");
    if (exists) {
        // Load high scores
    } else {
        high_scores_scene_create_empty_high_scores(self);
    }
}

void high_scores_scene_initialize(GameObject *scene)
{
    HighScoresScene *self = (HighScoresScene *)scene;
    SceneManager *manager = go_get_scene_manager(scene);
    GameData *data = (GameData *)manager->data;
    
    int latest_score = data->latest_score;
    
    self->w_info_label = ({
        char your_score[20];
        snprintf(your_score, 20, "Your score: %d", latest_score);
        
        Label *label = label_create("font4", your_score);
        
        label->anchor.x = 0.5f;
        label->anchor.y = 0.5f;
                        
        label->position.x = SCREEN_WIDTH / 2.f;
        label->position.y = SCREEN_HEIGHT / 2.f - 15.f;
        
        label->ignore_camera = true;
        
        label;
    });
    
    go_add_child(self, self->w_info_label);
    
    platform_file_exists(HIGH_SCORE_FILE, true, &high_scores_scene_scores_file_exists, self);
}

void high_scores_scene_end(void *obj)
{
    HighScoresScene *self = (HighScoresScene *)obj;
    SceneManager *manager = go_get_scene_manager(self);

    scene_change(manager, gecko_scene_create(), st_fade_black, 0.8f);
}

void high_scores_scene_start(GameObject *scene)
{
    HighScoresScene *self = (HighScoresScene *)scene;
    
    go_delayed_callback(self, &high_scores_scene_end, 10.f);
}

void high_scores_scene_update(GameObject *scene, Float dt)
{
    HighScoresScene *self = (HighScoresScene *)scene;
}

void high_scores_scene_fixed_update(GameObject *scene, Float dt_s)
{
    HighScoresScene *self = (HighScoresScene *)scene;
}

void high_scores_scene_render(GameObject *scene, RenderContext *ctx)
{
    HighScoresScene *self = (HighScoresScene *)scene;
    context_clear_white(ctx);
}

// Functions which are not used can be removed and set to NULL in the type list
static SceneType HighScoresSceneType =
    scene_type(
        "HighScoresScene",
        &high_scores_scene_destroy,
        &high_scores_scene_describe,
        &high_scores_scene_initialize,
        NULL, // A scene will never be removed from parent so the function is not set
        &high_scores_scene_start,
        &high_scores_scene_update,
        &high_scores_scene_fixed_update,
        &high_scores_scene_render
);

Scene *high_scores_scene_create()
{
    Scene *scene = scene_alloc(sizeof(HighScoresScene));
    scene->w_type = &HighScoresSceneType;
    
    HighScoresScene *self = (HighScoresScene *)scene;
    
    self->new_score_index = -1;
    
    // List assets which will be loaded automatically before scene starts
    //scene_set_required_image_asset_names(scene, list_of_strings("demo_image"));
    scene_set_required_grid_atlas_infos(scene, list_of_grid_atlas_infos(grid_atlas_info("font4", (Size2DInt){ 8, 14 })));
    //scene_set_required_audio_effects(scene, list_of_strings("demo_audio"));
    
    return scene;
}
