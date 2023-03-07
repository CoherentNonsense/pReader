#include "preader.h"
#include "text.h"


PlaydateAPI* playdate;

int menu_offset = 0;
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

  textscroll_draw(text, 0);
}

void preader_update() {
  unsigned int current_time = playdate->system->getCurrentTimeMilliseconds();
  if (current_time > ui_blink_timer) {
    ui_blink_timer = current_time + 1000;
    ui_blink = !ui_blink;
  }
  int change = (int)playdate->system->getCrankChange();
  if (change != 0) {
    offset += change;
    offset = textscroll_draw(text, offset);
    if (change > 0) {
      menu_offset = offset - 30;
    }
  }
    
    
  if (offset < menu_offset || offset == 0) {
    playdate->graphics->fillRect(0, 0, 400, 40, kColorBlack);
    if (ui_blink) {
      playdate->graphics->setDrawMode(kDrawModeNXOR);
    }
    playdate->graphics->drawBitmap(bitmap_menu, 2, 2, kBitmapUnflipped);
    playdate->graphics->setDrawMode(kDrawModeNXOR);
    playdate->graphics->drawText("The War of the Worlds", 50, kASCIIEncoding, 40, 10);
    playdate->graphics->setDrawMode(kDrawModeCopy);
    playdate->graphics->setStencilImage(bitmap_grey, 1);
    playdate->graphics->fillRect(0, 40, 400, 4, kColorBlack);
    playdate->graphics->setStencilImage(bitmap_clear, 1);
  }
}