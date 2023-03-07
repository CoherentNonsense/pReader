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
  textscroll_set_pd_ptr(playdate);
  text = textscroll_new("test.txt", "fonts/Georgia-14");

  textscroll_draw(text, 0);
}

void preader_update() {
  int change = (int)playdate->system->getCrankChange();
  if (change != 0) {
    // textscroll_changeFont(text, "fonts/Georgia-12");
    offset += change;
    offset = textscroll_draw(text, offset);
  }
}