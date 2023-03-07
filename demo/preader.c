#include "preader.h"
#include "../src/textscroll.h"
#include "pd_api/pd_api_sys.h"


PlaydateAPI* playdate;


// TODO: Cleanup everything
int menu = 1;

int bar_open_offset = 0;
int bar_open = 0;
int offset = 0;
TextScroll* text;
LCDBitmap* bitmap_grey;
LCDBitmap* bitmap_clear;
LCDBitmap* bitmap_menu;
int ui_blink;
unsigned int ui_blink_timer;

void preader_set_pd_ptr(PlaydateAPI* pd) {
 playdate = pd;
}

void preader_start() {
  const char* err;
  bitmap_grey = playdate->graphics->loadBitmap("images/grey", &err);
  bitmap_clear = playdate->graphics->newBitmap(32, 32, kColorWhite);
  bitmap_menu = playdate->graphics->loadBitmap("images/menu", &err);
  
  textscroll_set_pd_ptr(playdate);
  text = textscroll_new("test.txt", "fonts/Georgia-14");
  textscroll_changeMargin(text, 5, 50);
}

void preader_update() {
  if (menu) {
    playdate->graphics->drawRect(20, 20, 30, 30, kColorBlack);
    PDButtons buttons;
    playdate->system->getButtonState(NULL, &buttons, NULL);
    if (buttons > 0) {
      menu = 0;
      textscroll_draw(text, 0);
      bar_open = 1;
      playdate->graphics->fillRect(0, 0, 400, 40, kColorBlack);
      playdate->graphics->setDrawMode(kDrawModeNXOR);
      playdate->graphics->drawText("The War of the Worlds", 50, kASCIIEncoding, 40, 10);
      playdate->graphics->setDrawMode(kDrawModeCopy);
      playdate->graphics->setStencilImage(bitmap_grey, 1);
      playdate->graphics->fillRect(0, 40, 400, 4, kColorBlack);
      playdate->graphics->setStencilImage(bitmap_clear, 1);    
    }
  } else {    
    unsigned int current_time = playdate->system->getCurrentTimeMilliseconds();
    int change = (int)playdate->system->getCrankChange();
    if (change != 0) {
      offset += change;
      offset = textscroll_draw(text, offset);
      if (change > 0 && offset > 15) {
        bar_open = 0;
        bar_open_offset = offset - 15;
      } else {
        bar_open = 1;
        playdate->graphics->fillRect(0, 0, 400, 40, kColorBlack);
        playdate->graphics->drawBitmap(bitmap_menu, 2, 2, kBitmapUnflipped);
        playdate->graphics->setDrawMode(kDrawModeNXOR);
        playdate->graphics->drawText("The War of the Worlds", 50, kASCIIEncoding, 40, 10);
        playdate->graphics->setDrawMode(kDrawModeCopy);
        playdate->graphics->setStencilImage(bitmap_grey, 1);
        playdate->graphics->fillRect(0, 40, 400, 4, kColorBlack);
        playdate->graphics->setStencilImage(bitmap_clear, 1);
      }
    }
    
    if (bar_open && !change) {
      if (current_time > ui_blink_timer) {
        ui_blink_timer = current_time + 800;
        ui_blink = !ui_blink;
        playdate->graphics->setDrawMode(kDrawModeXOR);
        playdate->graphics->drawBitmap(bitmap_menu, 2, 2, kBitmapUnflipped);
        playdate->graphics->setDrawMode(kDrawModeCopy);
      }
    }
    
    PDButtons buttons;
    playdate->system->getButtonState(NULL, &buttons, NULL);
    if (buttons & kButtonA) {
      if (!bar_open) {
        bar_open = 1;
        playdate->graphics->fillRect(0, 0, 400, 40, kColorBlack);
        playdate->graphics->drawBitmap(bitmap_menu, 2, 2, kBitmapUnflipped);
        playdate->graphics->setDrawMode(kDrawModeNXOR);
        playdate->graphics->drawText("The War of the Worlds", 50, kASCIIEncoding, 40, 10);
        playdate->graphics->setDrawMode(kDrawModeCopy);
        playdate->graphics->setStencilImage(bitmap_grey, 1);
        playdate->graphics->fillRect(0, 40, 400, 4, kColorBlack);
        playdate->graphics->setStencilImage(bitmap_clear, 1);
      } else {
        menu = 1;
        playdate->graphics->clear(kColorWhite);
      }
    }
  }
}