#ifndef TEXTSCROLL_H
#define TEXTSCROLL_H

typedef struct TextScroll TextScroll;

TextScroll* textscroll_new(const char* file_path, const char* font_path);

void textscroll_free(TextScroll* text);

void textscroll_changeFont(TextScroll* text, const char* path);

void textscroll_changeMargin(TextScroll* text, const unsigned int margin);

int textscroll_draw(TextScroll* text, int scroll);

#endif