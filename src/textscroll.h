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
TextScroll* textscroll_new(void);

// Frees a TextScroll.
void textscroll_free(TextScroll* text);

// Changes the file to be read from.
void textscroll_setFile(TextScroll* text, const char* path);

// Changes the font for a TextScroll.
void textscroll_setFont(TextScroll* text, const char* path);

// Changes the left and right margins of a TextScroll.
void textscroll_setMargin(TextScroll* text, const unsigned int horizontal, const unsigned int vertical);

// Draws a TextScroll with a specific scroll in pixels.
// 0 is the top of the document and the bottom of the document is unknown,
// You must have a font and file set before drawing.
int textscroll_draw(TextScroll* text, int scroll);

// Gets the progress of the last draw call in the range [0, 1]
// where 0 is the top of the document, and 1 is the bottom.
float textscroll_getProgress(TextScroll* text);

#endif