// Text Scroll
//
// Dynamically load and display .txt files.
//
// Author: Coherent Nonsense

#ifndef TEXTSCROLL_H
#define TEXTSCROLL_H

#include <pd_api.h>


typedef struct TextScroll TextScroll;


// Must be called before any other function.
void textscroll_set_pd_ptr(PlaydateAPI* playdate);

// Creates a new TextScroll. You are responsible for freeing it with 'textscroll_free'.
// You must include a file_path to a .txt and a font_path to a font.
TextScroll* textscroll_new(const char* file_path, const char* font_path);

// Frees a TextScroll.
void textscroll_free(TextScroll* text);

// Changes the font for a TextScroll.
void textscroll_changeFont(TextScroll* text, const char* path);

// Changes the left and right margins of a TextScroll.
void textscroll_changeMargin(TextScroll* text, const unsigned int margin);

// Draws a TextScroll with a specific scroll in pixels.
// 0 is the top of the document and the bottom of the document is unknown,
// however, the TextScroll will clamp between 0 and the bottom and return the
// clamped scroll.
int textscroll_draw(TextScroll* text, int scroll);

#endif