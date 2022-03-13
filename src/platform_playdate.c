#include <stdio.h>
#include <stdlib.h>
#include "pd_api.h"
#include "platform_playdate.h"

platform_time_t playdate_time;
PlaydateAPI* playdate_platform_api;

void *platform_malloc(size_t size)
{
    return playdate_platform_api->system->realloc(NULL, size);
}

void *platform_calloc(size_t count, size_t size)
{
    if (count > SIZE_MAX / size) {
        return NULL;
    }
    size_t alloc_size = size * count;
    uint8_t *ptr = playdate_platform_api->system->realloc(NULL, alloc_size);
    for (size_t i = 0; i < alloc_size; ++i) {
        ptr[i] = '\0';
    }
    return ptr;
}

void *platform_realloc(void *ptr, size_t size)
{
    return playdate_platform_api->system->realloc(ptr, size);
}

char *platform_strdup(const char *str)
{
    size_t i = 0;
    while (str[i++] != '\0');
    if (i <= 1) { return NULL; }
    char *dup = playdate_platform_api->system->realloc(NULL, i);
    for (i = 0; (dup[i] = str[i]) != '\0'; ++i);
    return dup;
}

char *platform_strndup(const char *str, size_t size)
{
    size_t i = 0;
    while (str[i++] != '\0' && i <= size);
    if (i <= 1) { return NULL; }
    char *dup = playdate_platform_api->system->realloc(NULL, i - 1);
    for (i = 0; (dup[i] = str[i]) != '\0'; ++i);
    dup[i] = '\0';
    return dup;
}

void platform_free(void *ptr)
{
    playdate_platform_api->system->realloc(ptr, 0);
}

platform_time_t platform_current_time()
{
    return (platform_time_t)playdate_time;
}

float platform_time_to_seconds(platform_time_t time)
{
    return (float)(((double)time) / 1000.0);
}

void platform_print(const char *text)
{
    playdate_platform_api->system->logToConsole("%s", text);
}

