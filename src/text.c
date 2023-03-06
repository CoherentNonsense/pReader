#include "text.h"
#include "preader.h"
#include <pd_api.h>

#define CHUNK_TEXT_SIZE (1024)
#define CHUNKS_LENGTH (8)

typedef struct TextChunk {
  int start_height; // Offset from the start of the document in pixels.
  unsigned int height; // Height of chunk in pixels.
  
  unsigned int start_index; // Offset from the start of the document in bytes.
  unsigned int length; // Length of chunk in bytes.
  
  char* text; // Proccessed text. (with newlines)
} TextChunk;

typedef struct TextScroll {
  SDFile* file; // File to read from.
  FileStat file_stat; // File stats.
  char font_path[100]; // Font path (to compare with updated font).
  LCDFont* font; // Current font.
  
  int* chunk_sizes;
  int chunk_sizes_length;
  int*chunk_heights;
  int total_chunks;
  int chunks_index;
  int chunks_length;
  int margin;
  TextChunk chunks[CHUNKS_LENGTH];
} TextScroll;

TextScroll* textscroll_new(const char* file_path, const char* font_path) {
  FileStat stat;
  int stat_result = playdate->file->stat(file_path, &stat);
  
  if (stat_result == -1) {
    return NULL;
  }
  
  TextScroll* text = (TextScroll*)playdate->system->realloc(NULL, sizeof(TextScroll));

  // TODO: Remove cache if file has been modified

  text->file_stat = stat;  
  text->file = playdate->file->open(file_path, kFileReadData);
  const char* err;

  for (int i = 0; i < 100; i++) {
    text->font_path[i] = font_path[i];
    if (font_path[i] == 0) {
      break;
    }
  }  
  text->font = playdate->graphics->loadFont(font_path, &err);
  text->margin = 5;
  // HACK: Add 5 extra since total_chunks is a little less than actual
  text->total_chunks = (text->file_stat.size / CHUNK_TEXT_SIZE) + 5;
  text->chunk_sizes = (int*)playdate->system->realloc(NULL, sizeof(int) * text->total_chunks);
  text->chunk_heights = (int*)playdate->system->realloc(NULL, sizeof(int) * text->total_chunks);
  text->chunks_index = 0;
  text->chunks_length = 0;
  text->chunk_sizes_length = 0;
  
  for (int i = 0; i < text->total_chunks; i++) {
    text->chunk_sizes[i] = -1;
    text->chunk_heights[i] = -1;
  }
  
  for (int i = 0; i < CHUNKS_LENGTH; i++) {
    text->chunks[i].text = (char*)playdate->system->realloc(NULL, CHUNK_TEXT_SIZE);
    text->chunks[i].text[CHUNK_TEXT_SIZE - 1] = 0;
  }
    
  return text;
}

void textscroll_free(TextScroll* text) {
  playdate->system->realloc(text->chunk_sizes, 0);
  
  for (int i = 0; i < CHUNKS_LENGTH; ++i) {
    playdate->system->realloc(text->chunks[i].text, 0);
  }
  
  playdate->file->close(text->file);
}

// Only use with valid indices (Start of newline)
static int textscroll_newChunk(TextScroll* text, unsigned int index, int height) {
  int chunk_id = text->chunks_index;
  TextChunk* chunk = &text->chunks[chunk_id];
  if (text->chunks_length < CHUNKS_LENGTH) {
    text->chunks_length += 1;
  }
  text->chunks_index = (chunk_id + 1) % CHUNKS_LENGTH;
  chunk->start_index = index;
  chunk->start_height = height;
  
  playdate->file->seek(text->file, index, SEEK_SET);
  int read = playdate->file->read(text->file, chunk->text, CHUNK_TEXT_SIZE - 1);
  
  // Process text
  int width = 0;
  int max_width = 400 - (text->margin * 2);
  int last_newline = 0;
  int font_height = playdate->graphics->getFontHeight(text->font);
  chunk->height = 0;
  for (int i = 0; i < read; ++i) {
    if (chunk->text[i] == '\n') {
      width = 0;
      last_newline = i;
      chunk->height += font_height;
      continue;
    }
    
    width += playdate->graphics->getTextWidth(text->font, chunk->text + i, 1, kUTF8Encoding, 0);
    if (width > max_width) {
      // Go back to the last space
      do {
        i -= 1;
      } while (chunk->text[i] != ' ');
      width = 0;
      last_newline = i;
      chunk->height += font_height;
      chunk->text[i] = '\n';
    }
  }
  
  if (last_newline == 0) {
    chunk->length = read;
    chunk->height = font_height;
  } else {
    chunk->length = last_newline + 1;
  }
  return chunk_id;
}

static int textscroll_getChunkFromHeight(TextScroll* text, int height) {
  for (int i = 0; i < text->chunks_length; i++) {
    TextChunk* chunk = &text->chunks[i];
    if (chunk->start_height <= height && height < chunk->start_height + chunk->height) {
      return i;
    }
  }
  
  return -1;
}

static int textscroll_getChunk(TextScroll* text, int height) {
  int chunk_id = textscroll_getChunkFromHeight(text, height);
  if (chunk_id != -1) {
    return chunk_id;
  }
    
  int total_bytes = 0;
  int total_height = 0;
  for (int i = 0; i < text->total_chunks; i++) {
    int* chunk_size = &text->chunk_sizes[i];
    int* chunk_height = &text->chunk_heights[i];
    if (*chunk_size == -1) {
      TextChunk* chunk = &text->chunks[textscroll_newChunk(text, total_bytes, total_height)];
      *chunk_size = chunk->length;
      *chunk_height = chunk->height;
    }
    
    total_bytes += *chunk_size;
    total_height += *chunk_height;
    if (total_height > height) {
      total_bytes -= *chunk_size;
      total_height -= *chunk_height;
      break;
    }
  }

  chunk_id = textscroll_getChunkFromHeight(text, total_height);
  
  if (chunk_id == -1) {
    chunk_id = textscroll_newChunk(text, total_bytes, total_height);
  }
  
  return chunk_id;
}

int textscroll_draw(TextScroll* text, int scroll) {
  if (scroll < 0) {
    scroll = 0;
  }
  
  int chunk_id = textscroll_getChunk(text, scroll);
  
  TextChunk* chunk = &text->chunks[chunk_id];
  
  int next_chunk_id = textscroll_getChunk(text, chunk->start_height + chunk->height);
    
  TextChunk* next_chunk = &text->chunks[next_chunk_id];
  
  playdate->graphics->setFont(text->font);
  playdate->graphics->fillRect(text->margin, 0, 400 - (text->margin * 2) + 1, 240, kColorWhite);
  playdate->graphics->drawText(chunk->text, chunk->length, kUTF8Encoding, text->margin, -scroll + chunk->start_height);
  playdate->graphics->drawText(next_chunk->text, next_chunk->length, kUTF8Encoding, text->margin, -scroll + next_chunk->start_height);
  
  return scroll;
}

void textscroll_changeFont(TextScroll* text, const char* path) {
  for (int i = 0; i < 100; i++) {    
    if (text->font_path[i] != path[i]) {
      return;
    }
    
    if (text->font_path[i] == 0) {
      break;
    }
  }
  
  const char* err;
  LCDFont* font = playdate->graphics->loadFont(path, &err);
  if (!font) {
    return;
  }
  
  for (int i = 0; i < text->total_chunks; i++) {
    if (text->chunk_sizes[i] == -1) {
      break;
    }
    text->chunk_sizes[i] = -1;
    text->chunk_heights[i] = -1;
  }
  text->chunk_sizes_length = 0;
  text->chunks_length = 0;
  text->chunks_index = 0;
  
  text->font = font;
}

void textscroll_changeMargin(TextScroll* text, const unsigned int margin) {
  if (margin == text->margin) {
    return;
  }
  
  playdate->graphics->fillRect(text->margin, 0, 400 - (text->margin * 2), 240, kColorWhite);
  for (int i = 0; i < text->total_chunks; i++) {
    if (text->chunk_sizes[i] == -1) {
      break;
    }
    text->chunk_sizes[i] = -1;
    text->chunk_heights[i] = -1;
  }
  
  text->chunks_index = 0;
  text->chunks_length = 0;
  text->chunk_sizes_length = 0;  
  text->margin = margin;
}
