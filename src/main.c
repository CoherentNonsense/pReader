#include "preader.h"

#include <pd_api.h>

static int update(void* userdata);

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
  (void)arg; // arg is currently only used for event = kEventKeyPressed
  
  if (event == kEventInit) {
    preader_set_pd_ptr(playdate);
    preader_start();
    playdate->system->setUpdateCallback(update, playdate);
  }
  
  return 0;
}

static int update(void* userdata) {	
  preader_update();

  return 1;
}

