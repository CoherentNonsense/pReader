#include "preader.h"
#include "text.h"

PlaydateAPI* playdate;

#define MARGIN (5)

int offset = 0;
TextScroll* text;

void preader_set_pd_ptr(PlaydateAPI* pd) {
 playdate = pd;
}

void preader_start() {
  // playdate->file->mkdir(".cache");
  text = textscroll_new("test.txt", "fonts/Georgia-14");

  textscroll_draw(text, 0);
}

void preader_update() {
  offset += (int)playdate->system->getCrankChange();
  if (offset != 0) {  
    offset = textscroll_draw(text, offset);
  }
}