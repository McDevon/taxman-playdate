#include <stdio.h>
#include <stdlib.h>
#include "pd_api.h"

static int update(void* userdata);

#ifdef _WINDLL
__declspec(dllexport)
#endif

int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	(void)arg;

	if (event == kEventInit)
	{
		pd->system->setUpdateCallback(update, pd);
	}
	
	return 0;
}

#define RECT_SIZE 16

int x = (400 - RECT_SIZE) / 2;
int y = (240 - RECT_SIZE) / 2;
int dx = 2;
int dy = 1;

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;

    const int res = 1;
	
    pd->graphics->fillRect(x, y, RECT_SIZE, RECT_SIZE, kColorWhite);

    x += dx;
    y += dy;
    
    if ( x < 0 || x > LCD_COLUMNS - RECT_SIZE ) {
        dx = -dx;
    }
    
    if ( y < 0 || y > LCD_ROWS - RECT_SIZE ) {
        dy = -dy;
    }
            
    pd->graphics->fillRect(x, y, RECT_SIZE, RECT_SIZE, kColorBlack);
	pd->system->drawFPS(10,10);

	return res;
}

