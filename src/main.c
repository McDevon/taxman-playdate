#include <stdio.h>
#include <stdlib.h>
#include "pd_api.h"
#include "engine.h"
#include "platform_playdate.h"
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
    StringBuilder *sb = sb_create();
    sb_append_format(sb, "%s", file_path);
    
    char *path = sb_get_string(sb);
    playdate_platform_api->system->logToConsole("Read text file %s", path);
    SDFile* file = playdate_platform_api->file->open(path, kFileRead);
    platform_free(path);
    destroy(sb);
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

void platform_load_image(const char *file_path, load_image_data_callback_t *callback, void *context)
{
    /*const char *error = NULL;
    LCDBitmap *bitmap = playdate_platform_api->graphics->loadBitmap(file_path, &error);
    if (!bitmap) {
        playdate_platform_api->system->logToConsole("Error loading bitmap: %s", error);
        callback(file_path, 0, 0, false, NULL, context);
        return;
    }
    int width, height, rowbytes, hashmask;
    uint8_t *data;
    playdate_platform_api->graphics->getBitmapData(bitmap, &width, &height, &rowbytes, &hashmask, &data);
    playdate_platform_api->system->logToConsole("All good %s: %d, %d -> rowbytes: %d", file_path, width, height, rowbytes);
    callback(file_path, width, height, false, data, context);*/
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
    playdate_platform_api->system->logToConsole("Read file %s", path);
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
    playdate_platform_api->system->logToConsole("All good %s: %d, %d format: %d", file_path, width, height, format);
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

void platform_display_set_image(uint8_t *buffer)
{
    playdate_platform_api->graphics->clear(kColorWhite);
    uint8_t *display = playdate_platform_api->graphics->getFrame();
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH / 8; ++x) {
            int si = x * 8 + y * SCREEN_WIDTH;
            uint8_t *byte = display + y * 52 + x;
            *byte = 0;
            const uint8_t color = 1;
            *byte |= ((buffer[si++] > 128) << 7) * color;
            *byte |= ((buffer[si++] > 128) << 6) * color;
            *byte |= ((buffer[si++] > 128) << 5) * color;
            *byte |= ((buffer[si++] > 128) << 4) * color;
            *byte |= ((buffer[si++] > 128) << 3) * color;
            *byte |= ((buffer[si++] > 128) << 2) * color;
            *byte |= ((buffer[si++] > 128) << 1) * color;
            *byte |= ((buffer[si++] > 128) << 0) * color;
        }
    }
    //playdate_platform_api->graphics->display();
}

void playdate_list_file(const char* path, void* userdata)
{
    playdate_platform_api->system->logToConsole("File: %s", path);
    FileStat stat;
    int error = playdate_platform_api->file->stat(path, &stat);
    if (error) {
        playdate_platform_api->system->logToConsole("Error reading file stats %s: %s", path, playdate_platform_api->file->geterr());
    }
    playdate_platform_api->system->logToConsole("File stats %s: dir: %d size: %d", path, stat.isdir, stat.size);
    SDFile *file = playdate_platform_api->file->open(path, kFileRead);
    if (!file) {
        playdate_platform_api->system->logToConsole("Error opening file %s: %s", path, playdate_platform_api->file->geterr());
    }
    playdate_platform_api->file->close(file);
}

static void startGame(PlaydateAPI* pd)
{
    playdate_platform_api = pd;
    
    playdate_platform_api->file->listfiles("", &playdate_list_file, NULL);

    game_init(loading_scene_create());
    pd->system->setUpdateCallback(update, pd);
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;

    const int res = 1;
    platform_time_t previous_time = playdate_time;
    playdate_time = (platform_time_t)pd->system->getCurrentTimeMilliseconds();

    Number crank = nb_from_float(pd->system->getCrankAngle());
    
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
    Number delta = nb_from_long(playdate_time - previous_time);
    game_step(delta, controls);

    pd->system->drawFPS(0,0);
    
	return res;
}

