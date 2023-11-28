#include <stdio.h>
#include <stdlib.h>
#include "pd_api.h"
#include "engine.h"
#include "playdate_alloc.h"
#include "loading_scene.h"
#include "upng.h"

static int update(void*);
static void startGame(PlaydateAPI*);

#ifdef _WINDLL
__declspec(dllexport)
#endif

int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
    (void)arg;

    if (event == kEventInit) {
        startGame(pd);
    }
    
    return 0;
}

void platform_read_text_file(const char *file_path, load_text_data_callback_t *callback, void *context)
{
    SDFile* file = playdate_platform_api->file->open(file_path, kFileRead);
    if (!file) {
        playdate_platform_api->system->logToConsole("Error reading text file %s", file_path);
        callback(file_path, NULL, context);
        return;
    }
    const size_t increase_size = 128;
    size_t position = 0;
    char *buffer = playdate_platform_api->system->realloc(NULL, increase_size);
    while (true) {
        int read_count = playdate_platform_api->file->read(file, buffer + position, increase_size);
        if (read_count == -1) {
            playdate_platform_api->file->close(file);
            playdate_platform_api->system->realloc(buffer, 0);
            callback(file_path, NULL, context);
            return;
        }
        if (read_count < 128) {
            position += read_count;
            buffer[position] = '\0';
            break;
        } else {
            position += increase_size;
            buffer = playdate_platform_api->system->realloc(buffer, position + increase_size);
        }
    }
    
    playdate_platform_api->file->close(file);
    
    callback(file_path, buffer, context);
    
    playdate_platform_api->system->realloc(buffer, 0);
}

void platform_file_exists(const char *file_path, file_exists_callback_t *callback, void *context)
{
    FileStat stat;
    int result = playdate_platform_api->file->stat(file_path, &stat);
    callback(file_path, result == 0, context);
}

void platform_load_image(const char *file_path, load_image_data_callback_t *callback, void *context)
{
    const size_t len = strlen(file_path);
    SDFile* file;
    char *path;
    if (file_path[len - 4] == '.') {
        StringBuilder *sb = sb_create();
        sb_append_format(sb, "%si", file_path);
        
        path = sb_get_string(sb);
        destroy(sb);
    } else {
        StringBuilder *sb = sb_create();
        sb_append_format(sb, "%s.pngi", file_path);
        
        path = sb_get_string(sb);
        destroy(sb);
    }
    file = playdate_platform_api->file->open(path, kFileRead);
    
    if (!file) {
        playdate_platform_api->system->logToConsole("Error reading file %s: %s", path, playdate_platform_api->file->geterr());
        platform_free(path);
        callback(file_path, 0, 0, false, NULL, context);
        return;
    }
    platform_free(path);
    const size_t increase_size = 128;
    size_t position = 0;
    uint8_t *buffer = playdate_platform_api->system->realloc(NULL, increase_size);
    while (true) {
        int read_count = playdate_platform_api->file->read(file, buffer + position, increase_size);
        if (read_count == -1) {
            playdate_platform_api->file->close(file);
            playdate_platform_api->system->realloc(buffer, 0);
            callback(file_path, 0, 0, false, NULL, context);
            return;
        }
        if (read_count > 0) {
            position += increase_size;
            buffer = playdate_platform_api->system->realloc(buffer, position + increase_size);
        } else {
            break;
        }
    }
    
    playdate_platform_api->file->close(file);
    
    upng_t *png = upng_new_from_bytes(buffer, position);
    
    upng_error error = upng_header(png);
    if (error != UPNG_EOK) {
        playdate_platform_api->system->logToConsole("uPNG Error reading header %d", error);
        callback(file_path, 0, 0, false, NULL, context);
        upng_free(png);
        return;
    }
    error = upng_decode(png);
    if (error != UPNG_EOK) {
        playdate_platform_api->system->logToConsole("uPNG Error decoding %d", error);
        callback(file_path, 0, 0, false, NULL, context);
        upng_free(png);
        return;
    }

    const uint8_t *png_buffer = upng_get_buffer(png);
    unsigned int width = upng_get_width(png);
    unsigned int height = upng_get_height(png);
    upng_format format = upng_get_format(png);
    bool alpha = format == UPNG_RGBA8 || format == UPNG_LUMINANCE_ALPHA1 || format == UPNG_LUMINANCE_ALPHA2 || format == UPNG_LUMINANCE_ALPHA4 || format == UPNG_LUMINANCE_ALPHA8;
    if (format == UPNG_RGBA8) {
        playdate_platform_api->system->logToConsole("Warning! Asset file %s has RGBA colors, should be grayscale or black & white", file_path);

        uint8_t *target_buffer = playdate_platform_api->system->realloc(NULL, width * height * 2 * sizeof(uint8_t));
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = y * width + x;
                target_buffer[index * 2] = png_buffer[index * 4];
                target_buffer[index * 2 + 1] = png_buffer[index * 4 + 3];
            }
        }
        callback(file_path, width, height, true, target_buffer, context);

        playdate_platform_api->system->realloc(target_buffer, 0);
    } else if (format == UPNG_RGB8) {
        playdate_platform_api->system->logToConsole("Warning! Asset file %s has RGB colors, should be grayscale or black & white", file_path);
        uint8_t *target_buffer = playdate_platform_api->system->realloc(NULL, width * height * sizeof(uint8_t));
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = y * width + x;
                target_buffer[index] = png_buffer[index * 3];
            }
        }
        callback(file_path, width, height, false, target_buffer, context);

        playdate_platform_api->system->realloc(target_buffer, 0);
    } else {
        callback(file_path, width, height, alpha, png_buffer, context);
    }
    
    upng_free(png);
    playdate_platform_api->system->realloc(buffer, 0);
}

void platform_display_set_image(uint8_t *buffer, ScreenRenderOptions *options)
{
    playdate_platform_api->graphics->clear(kColorWhite);
    uint8_t *display = playdate_platform_api->graphics->getFrame();
    uint8_t colors[] = { 0, 1 };
    if (options->invert) {
        colors[0] = 1;
        colors[1] = 0;
    }
    
    uint32_t maskX, maskY, ditherWidth, ditherYComp, yComp, ditherX;
    uint8_t bufferValue;
    uint32_t *fa_buffer = (uint32_t *)buffer;
    uint32_t fa_bufferValue;
    int si;
    if (options->screen_dither && is_power_of_two(options->screen_dither->size.width) && is_power_of_two(options->screen_dither->size.height)) {
        maskX = options->screen_dither->size.width - 1;
        maskY = options->screen_dither->size.height - 1;
        ditherWidth = options->screen_dither->size.width;
        uint32_t *fa_ditherBuffer = (uint32_t *)options->screen_dither->buffer;
        uint32_t fa_ditherValue;
        uint8_t ditherValue;
        for (uint32_t y = 0; y < SCREEN_HEIGHT; ++y) {
            ditherYComp = (y & maskY) * ditherWidth;
            yComp = y * SCREEN_WIDTH;
            for (uint32_t x = 0; x < SCREEN_WIDTH >> 3; ++x) {
                si = (x << 3) + yComp;
                ditherX = (x << 3);
                *display = 0;
                fa_bufferValue = fa_buffer[si >> 2];
                fa_ditherValue = fa_ditherBuffer[(ditherYComp + (ditherX & maskX)) >> 2];
                bufferValue = (uint8_t)(fa_bufferValue);
                ditherValue = (uint8_t)(fa_ditherValue);
                *display |= (colors[bufferValue >= ditherValue] << 7);
                bufferValue = (uint8_t)(fa_bufferValue >> 8);
                ditherValue = (uint8_t)(fa_ditherValue >> 8);
                *display |= (colors[bufferValue >= ditherValue] << 6);
                bufferValue = (uint8_t)(fa_bufferValue >> 16);
                ditherValue = (uint8_t)(fa_ditherValue >> 16);
                *display |= (colors[bufferValue >= ditherValue] << 5);
                bufferValue = (uint8_t)(fa_bufferValue >> 24);
                ditherValue = (uint8_t)(fa_ditherValue >> 24);
                *display |= (colors[bufferValue >= ditherValue] << 4);
                fa_bufferValue = fa_buffer[(si+4) >> 2];
                fa_ditherValue = fa_ditherBuffer[(ditherYComp + ((ditherX + 4) & maskX)) >> 2];
                bufferValue = (uint8_t)(fa_bufferValue);
                ditherValue = (uint8_t)(fa_ditherValue);
                *display |= (colors[bufferValue >= ditherValue] << 3);
                bufferValue = (uint8_t)(fa_bufferValue >> 8);
                ditherValue = (uint8_t)(fa_ditherValue >> 8);
                *display |= (colors[bufferValue >= ditherValue] << 2);
                bufferValue = (uint8_t)(fa_bufferValue >> 16);
                ditherValue = (uint8_t)(fa_ditherValue >> 16);
                *display |= (colors[bufferValue >= ditherValue] << 1);
                bufferValue = (uint8_t)(fa_bufferValue >> 24);
                ditherValue = (uint8_t)(fa_ditherValue >> 24);
                *display |= (colors[bufferValue >= ditherValue] << 0);
                ++display;
            }
            display += 2;
        }
    } else {
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            for (int x = 0; x < SCREEN_WIDTH / 8; ++x) {
                int si = (x << 3) + y * SCREEN_WIDTH;
                *display = 0;
                fa_bufferValue = fa_buffer[si >> 2];
                bufferValue = (uint8_t)(fa_bufferValue);
                *display |= (colors[(bufferValue > 128)] << 7);
                bufferValue = (uint8_t)(fa_bufferValue >> 8);
                *display |= (colors[(bufferValue > 128)] << 6);
                bufferValue = (uint8_t)(fa_bufferValue >> 16);
                *display |= (colors[(bufferValue > 128)] << 5);
                bufferValue = (uint8_t)(fa_bufferValue >> 24);
                *display |= (colors[(bufferValue > 128)] << 4);
                fa_bufferValue = fa_buffer[(si+4) >> 2];
                bufferValue = (uint8_t)(fa_bufferValue);
                *display |= (colors[(bufferValue > 128)] << 3);
                bufferValue = (uint8_t)(fa_bufferValue >> 8);
                *display |= (colors[(bufferValue > 128)] << 2);
                bufferValue = (uint8_t)(fa_bufferValue >> 16);
                *display |= (colors[(bufferValue > 128)] << 1);
                bufferValue = (uint8_t)(fa_bufferValue >> 24);
                *display |= (colors[(bufferValue > 128)] << 0);
                ++display;
            }
            display += 2;
        }
    }
    //playdate_platform_api->graphics->display();
}

void platform_load_audio_file(const char *file_name, audio_object_callback_t *callback, void *context)
{
    FilePlayer *player = playdate_platform_api->sound->fileplayer->newPlayer();
    int result = playdate_platform_api->sound->fileplayer->loadIntoPlayer(player, file_name);
    if (result == 0) {
        playdate_platform_api->sound->fileplayer->freePlayer(player);
        player = NULL;
        callback(file_name, NULL, context);
    } else {
        callback(file_name, player, context);
    }
}

void platform_play_audio_object(void *audio_object)
{
    FilePlayer *player = (FilePlayer*)audio_object;
    if (!player) {
        LOG_ERROR("Cannot play audio file, player is NULL");
        return;
    }
    playdate_platform_api->sound->fileplayer->play(player, 1);
}

void platform_stop_audio_object(void *audio_object)
{
    FilePlayer *player = (FilePlayer*)audio_object;
    if (!player) {
        LOG_ERROR("Cannot stop audio file, player is NULL");
        return;
    }
    playdate_platform_api->sound->fileplayer->stop(player);
}

void platform_free_audio_object(void *audio_object)
{
    FilePlayer *player = (FilePlayer*)audio_object;
    if (!player) {
        LOG_ERROR("Cannot free audio file, player is NULL");
        return;
    }
    playdate_platform_api->sound->fileplayer->freePlayer(player);
}

void playdate_list_file(const char* path, void* userdata)
{
    FileStat stat;
    int error = playdate_platform_api->file->stat(path, &stat);
    if (error) {
        playdate_platform_api->system->logToConsole("Error reading file stats %s: %s", path, playdate_platform_api->file->geterr());
    }
    SDFile *file = playdate_platform_api->file->open(path, kFileRead);
    if (!file) {
        playdate_platform_api->system->logToConsole("Error opening file %s: %s", path, playdate_platform_api->file->geterr());
    }
    playdate_platform_api->file->close(file);
}

static void startGame(PlaydateAPI* pd)
{
    playdate_platform_api = pd;
    
    playdate_platform_api->file->listfiles("", &playdate_list_file, NULL, 0);

    playdate_platform_api->system->resetElapsedTime();
    game_init(loading_scene_create());
    playdate_platform_api->system->setUpdateCallback(update, pd);
}

static int update(void* userdata)
{
    PlaydateAPI* pd = userdata;

    const int res = 1;
    platform_time_t previous_time = playdate_time;
    playdate_time = (platform_time_t)pd->system->getElapsedTime();

    Float crank = pd->system->getCrankAngle();
    
    PDButtons current;
    PDButtons pushed;
    PDButtons released;
    pd->system->getButtonState(&current, &pushed, &released);
    uint8_t left = (current & kButtonLeft) | (pushed & kButtonLeft) ? 1 : 0;
    uint8_t right = (current & kButtonRight) | (pushed & kButtonRight) ? 1 : 0;
    uint8_t up = (current & kButtonUp) | (pushed & kButtonUp) ? 1 : 0;
    uint8_t down = (current & kButtonDown) | (pushed & kButtonDown) ? 1 : 0;
    uint8_t a = (current & kButtonA) | (pushed & kButtonA) ? 1 : 0;
    uint8_t b = (current & kButtonB) | (pushed & kButtonB) ? 1 : 0;
    int menu = 0;

    Controls controls = { crank, (uint8_t)left, (uint8_t)right, (uint8_t)up, (uint8_t)down, (uint8_t)a, (uint8_t)b, (uint8_t)menu };
    Float delta = playdate_time - previous_time;
    game_step(delta, controls);

    pd->system->drawFPS(0,0);
    
    return res;
}


