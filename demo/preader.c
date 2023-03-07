#include "preader.h"
#include "../src/textscroll.h"
#include "pd_api/pd_api_sys.h"
#include <string.h>


PlaydateAPI* playdate;


// TODO: Cleanup everything
static int menu = 1;

static int bar_open_offset = 0;
static int bar_open = 0;
static int offset = 0;
static TextScroll* text;
static LCDBitmap* bitmap_grey;
static LCDBitmap* bitmap_clear;
static LCDFont* font;
static int files_length;
static char files[30][50];
static int selector = 0;

static void on_listfiles(const char* filename, void* userdata) {
  int filename_length = strlen(filename);

  // d e m o . t x t \0
  // 0 1 2 3 4 5 6 7 8
  // WHAT: ?Why is this the opposite?
  if (files_length < 30 && !strcmp(filename + filename_length - 4, ".txt")) {
    int file_index = files_length++;
    strcpy(files[file_index], filename);
    files[file_index][filename_length - 4] = 0;
  }
}

static int load_position(const char* filename) {
  char file_buffer[50] = ".cache/";
  strcat(file_buffer, filename);
  SDFile* file = playdate->file->open(file_buffer, kFileReadData);

  if (file == NULL) {
    return 0;
  }
  
  uint8_t buffer[4];
  playdate->file->read(file, buffer, 4);

  int result = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0] << 0);

  playdate->file->close(file);

  return result;
}

static void save_position(const char* filename, const int position) {
  char file_buffer[50] = ".cache/";
  strcat(file_buffer, filename);
  SDFile* file = playdate->file->open(file_buffer, kFileWrite);
  uint8_t buffer[4];
  buffer[0] = (position >> 0 ) & 0xff;
  buffer[1] = (position >> 8 ) & 0xff;
  buffer[2] = (position >> 16) & 0xff;
  buffer[3] = (position >> 24) & 0xff;
  playdate->file->write(file, buffer, 4);
  playdate->file->close(file);
}

static void draw_reader() {
  offset = textscroll_draw(text, offset);
  if (bar_open) {
    playdate->graphics->fillRect(0, 0, 400, 40, kColorBlack);
    playdate->graphics->setDrawMode(kDrawModeNXOR);
    playdate->graphics->drawText(files[selector], 50, kASCIIEncoding, 10, 10);
    playdate->graphics->setDrawMode(kDrawModeCopy);
    playdate->graphics->setStencilImage(bitmap_grey, 1);
    playdate->graphics->fillRect(0, 40, 400, 4, kColorBlack);
    playdate->graphics->setStencilImage(bitmap_clear, 1);
  }
  playdate->graphics->fillRect(396, textscroll_getProgress(text) * (240 - 80) + 5, 3, 70, kColorXOR);
}

void preader_set_pd_ptr(PlaydateAPI* pd) {
 playdate = pd;
}

void preader_start(void) {
  playdate->file->mkdir(".cache");
    
  const char* err;
  bitmap_grey = playdate->graphics->loadBitmap("images/grey", &err);
  bitmap_clear = playdate->graphics->newBitmap(32, 32, kColorWhite);
  font = playdate->graphics->loadFont("fonts/Georgia-14", &err);
  playdate->graphics->setFont(font);
  playdate->file->listfiles(".", on_listfiles, NULL, 0);
  
  textscroll_set_pd_ptr(playdate);
  text = textscroll_new();
  textscroll_setFont(text, "fonts/Georgia-14");
  textscroll_setMargin(text, 5, 50);
}

void preader_end(void) {
  if (!menu) {
    save_position(files[selector], offset);
  }
}

static void draw_menu() {
  playdate->graphics->clear(kColorWhite);
  int offset = 0;
  if (selector * 30 > 120) {
    offset = 120 - selector * 30;
  }
  for (int i = 0; i < files_length; i++) {
    playdate->graphics->drawText(files[i], 50, kASCIIEncoding, 10, 10 + 30 * i + offset);
    playdate->graphics->drawText(files[i], 50, kASCIIEncoding, 10, 10 + 30 * i + offset);
  }
  playdate->graphics->fillRect(5, 5 + 30 * selector + offset, 390, 30, kColorXOR);
}

static int once = 1;
void preader_update(void) {
  if (menu) {
    if (once) {
      draw_menu();
    }
    PDButtons buttons;
    playdate->system->getButtonState(NULL, &buttons, NULL);
    if (buttons & kButtonDown) {      
      selector += 1;
      if (selector == files_length) {
        selector = files_length - 1;
      }

      draw_menu();
    }
    if (buttons & kButtonUp) {
      playdate->graphics->clear(kColorWhite);
      selector -= 1;
      if (selector == -1) {
        selector = 0;
      }
      draw_menu();
    }
    if (buttons & kButtonA) {
      menu = 0;
      once = 1;
    }

  } else {
    if (once) {
      once = 0;
      char buffer[50];
      strcpy(buffer, files[selector]);
      strcat(buffer, ".txt");
      textscroll_setFile(text, buffer);
      offset = load_position(files[selector]);
      if (offset == 0) {
        bar_open = 1;
      }
      draw_reader();
    }
    PDButtons buttons;
    playdate->system->getButtonState(&buttons, NULL, NULL);
    int change = (int)playdate->system->getCrankChange();

    if (buttons & kButtonUp) {
      change -= 100;
    }
    if (buttons & kButtonDown) {
      change += 100;
    }
    
    if (change != 0) {
      offset += change;
      if (change > 0 && offset > 15) {
        bar_open = 0;
        bar_open_offset = offset - 15;
      } else {
        bar_open = 1;
      }
      draw_reader();
    }

    if (buttons & kButtonB) {
      save_position(files[selector], offset);
      menu = 1;
      once = 1;
    }
  }
}