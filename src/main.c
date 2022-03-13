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
    SDFile* file = playdate_platform_api->file->open(file_path, kFileRead);
    if (!file) {
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
        if (read_count > 0) {
            position += increase_size;
            buffer = playdate_platform_api->system->realloc(buffer, position + increase_size);
        } else {
            break;
        }
    }
    
    playdate_platform_api->file->close(file);
    
    callback(file_path, buffer, context);
    
    playdate_platform_api->system->realloc(buffer, 0);
}

void platform_load_image(const char *file_path, load_image_data_callback_t *callback, void *context)
{
    const char *error = NULL;
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
    callback(file_path, width, height, false, data, context);
}

void platform_display_set_image(uint8_t *buffer)
{
    /*
     uint8_t* playdate->graphics->getFrame(void);
     Returns the current display frame buffer. Rows are 32-bit aligned, so the row stride is 52 bytes, with the extra 2 bytes per row ignored. Bytes are MSB-ordered; i.e., the pixel in column 0 is the 0x80 bit of the first byte of the row.
     */
    
    uint8_t *display = playdate_platform_api->graphics->getFrame();
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH / 8; ++x) {
            int si = x * 8 + y * SCREEN_WIDTH;
            uint8_t *byte = display + y * 52 + x;
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
}

static void startGame(PlaydateAPI* pd)
{
    playdate_platform_api = pd;
    
    game_init(loading_scene_create());
    pd->system->setUpdateCallback(update, pd);
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;

    const int res = 1;
    platform_time_t previous_time = playdate_time;
    playdate_time = (platform_time_t)pd->system->getCurrentTimeMilliseconds();

	pd->system->drawFPS(10,10);
    
    double crank = 0;
    int left = 0, right = 0, up = 0, down = 0, a = 0, b = 0, menu = 0;
        
    Controls controls = { nb_from_double(crank), (uint8_t)left, (uint8_t)right, (uint8_t)up, (uint8_t)down, (uint8_t)a, (uint8_t)b, (uint8_t)menu };
    Number delta = nb_from_long(playdate_time - previous_time);
    game_step(delta, controls);

	return res;
}

