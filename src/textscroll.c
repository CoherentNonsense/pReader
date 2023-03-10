#include "textscroll.h"
#include <pd_api.h>

#define BUFFER_SIZE (1000)
#define BUFFERS_LENGTH (10)

static PlaydateAPI* playdate;

typedef struct TextChunk {
  int start_height; // Offset from the start of the document in pixels.
  int height; // Height of chunk in pixels.
  
  int start_index; // Offset from the start of the document in bytes.
  int length; // Length of chunk in bytes.
  
  // char* text; // Proccessed text. (with newlines)
  int buffer_index;
} TextChunk;

typedef struct TextBuffer {
  int owner_index;
  char text[BUFFER_SIZE];
} TextBuffer;

typedef struct TextScroll {
  SDFile* file;
  FileStat file_stat;
  char file_path[100]; // File path (to compare with updated file).
  char font_path[100]; // Font path (to compare with updated font).
  LCDFont* font;
  int margin_x;
  int margin_y;
  float progress;
  
  int total_chunks;
  TextChunk* chunks;
  
  int buffers_index;
  int buffers_length;
  TextBuffer* buffers;
} TextScroll;

void textscroll_set_pd_ptr(PlaydateAPI* pd) {
  playdate = pd;
}

TextScroll* textscroll_new() {
  TextScroll* text = (TextScroll*)playdate->system->realloc(NULL, sizeof(TextScroll));

  text->file = NULL;
  text->font = NULL;
  text->file_path[0] = 0;
  text->font_path[0] = 0;  
  text->margin_x = 5;
  text->margin_y = 5;
    
  // Text buffers
  text->buffers_index = 0;
  text->buffers_length = 0;
  text->buffers = (TextBuffer*)playdate->system->realloc(NULL, sizeof(TextBuffer) * BUFFERS_LENGTH);
  
  for (int i = 0; i < BUFFERS_LENGTH; i++) {
    text->buffers[i].owner_index = -1;
  }
    
  return text;
}

void textscroll_free(TextScroll* text) {
  playdate->file->close(text->file);
  
  playdate->system->realloc(text->buffers, 0);
  if (text->chunks != NULL) {
    playdate->system->realloc(text->chunks, 0);
  }
  playdate->system->realloc(text, 0);
}

static void textscroll_createChunk(TextScroll* text, int chunk_id, int index, int height) {
  text->chunks[chunk_id].start_index = index;
  text->chunks[chunk_id].start_height = height;
}

static int frame = 0;
// Only use with valid indices (Start of newline) and height.
static int textscroll_fillBuffer(TextScroll* text, int chunk_id) {
  // Push to circular array
  int buffer_id = text->buffers_index;
  TextBuffer* buffer = &text->buffers[buffer_id];
  if (text->buffers_length < BUFFERS_LENGTH) {
    text->buffers_length += 1;
  }  
  text->buffers_index = (buffer_id + 1) % BUFFERS_LENGTH;
  
  // Set chunk  
  TextChunk* chunk = &text->chunks[chunk_id];
  chunk->buffer_index = buffer_id;
    
  // Set buffer
  buffer->owner_index = chunk_id;
  playdate->file->seek(text->file, chunk->start_index, SEEK_SET);
  int read = playdate->file->read(text->file, buffer->text, BUFFER_SIZE - 1);
  
  // Process text
  int width = 0;
  int max_width = 400 - (text->margin_x * 2);
  int last_newline = 0;
  int font_height = playdate->graphics->getFontHeight(text->font);
  chunk->height = 0;
  for (int i = 0; i < read; ++i) {
    if (buffer->text[i] == '\n') {
      width = 0;
      last_newline = i;
      chunk->height += font_height;
      continue;
    }
    
    if (buffer->text[i] == '\r') {
      buffer->text[i] = ' ';
      continue;
    }
    
    width += playdate->graphics->getTextWidth(text->font, buffer->text + i, 1, kUTF8Encoding, 0);
    if (width > max_width) {
      // Go back to the last space
      do {
        i -= 1;
      } while (buffer->text[i] != ' ');
      width = 0;
      last_newline = i;
      chunk->height += font_height;
      buffer->text[i] = '\n';
    }    
  }
  
  if (last_newline == 0) {
    chunk->length = read;
    chunk->height = font_height;
  } else {
    chunk->length = last_newline + 1;
  }
  return buffer_id;
}

static int textscroll_getBufferFromHeight(TextScroll* text, int height) {
  for (int i = 0; i < text->buffers_length; i++) {
    TextBuffer* buffer = &text->buffers[i];
    TextChunk* chunk = &text->chunks[buffer->owner_index];
    if (chunk->start_height <= height && height < chunk->start_height + chunk->height) {
      return i;
    }
  }
  
  return -1;
}

// Gets the nearest buffer to height.
// Fills a buffer if none exist.
static int textscroll_getBuffer(TextScroll* text, int height) {
  int buffer_id = textscroll_getBufferFromHeight(text, height);
  if (buffer_id != -1) {
    return buffer_id;
  }
    
  int result_buffer_id = 0;
  for (int i = 0; i < text->total_chunks; i++) {
    TextChunk* chunk = &text->chunks[i];
    
    // If chunk has not been loaded
    if (chunk->start_index == -1) {
      if (i == 0) {
        textscroll_createChunk(text, i, 0, 0);
      } else {
        TextChunk* prev_chunk = &text->chunks[i - 1];
        textscroll_createChunk(text, i, prev_chunk->start_index + prev_chunk->length, prev_chunk->start_height + prev_chunk->height);
      }
          
      textscroll_fillBuffer(text, i);
    }

    // We reached the right chunk
    if (chunk->start_height + chunk->height > height) {      
      // If chunk lost its buffer
      TextBuffer* buffer = &text->buffers[chunk->buffer_index];
      if (buffer->owner_index != i) {
        textscroll_fillBuffer(text, i);
      }
      
      result_buffer_id = chunk->buffer_index;
      break;
    }
  }
    
  return result_buffer_id;
}

int textscroll_draw(TextScroll* text, int scroll) {
  if (text->file_path[0] == 0 && text->font_path[0] == 0) {
    return 0;
  }
  
  if (scroll < 0) {
    scroll = 0;
  }
  
  frame += 1;
  int buffer_id = textscroll_getBuffer(text, scroll - text->margin_y);
  
  TextBuffer* buffer= &text->buffers[buffer_id];
  TextChunk* chunk = &text->chunks[buffer->owner_index];
  
  text->progress = (float)chunk->start_index / (float)text->file_stat.size;
  playdate->graphics->fillRect(0, 0, 400, 240, kColorWhite);
  playdate->graphics->setFont(text->font);
  
  if (scroll - text->margin_y > chunk->start_height + chunk->height) {
    scroll = chunk->start_height + chunk->height + text->margin_y;
    text->progress = 1.0f;
  } else {
    int next_buffer_id = textscroll_getBuffer(text, chunk->start_height + chunk->height);
    
    TextBuffer* next_buffer = &text->buffers[next_buffer_id];
    TextChunk* next_chunk = &text->chunks[next_buffer->owner_index];
    
    playdate->graphics->drawText(next_buffer->text, next_chunk->length, kUTF8Encoding, text->margin_x, -scroll + next_chunk->start_height + text->margin_y);
  }
  
  playdate->graphics->drawText(buffer->text, chunk->length, kUTF8Encoding, text->margin_x, -scroll + chunk->start_height + text->margin_y);

  return scroll;
}

void textscroll_setFile(TextScroll* text, const char* path) {
  // Compare paths
  for (int i = 0; i < 100; i++) {    
    if (text->file_path[i] != path[i]) {
      break;
    }
        
    if (text->file_path[i] == 0 && path[i] == 0) {
      return;
    }
  }

  SDFile* file = playdate->file->open(path, kFileReadData);
  if (file == NULL) {
    return;
  }  
  // Copy path
  for (int i = 0; i < 100; i++) {
    text->file_path[i] = path[i];
    if (path[i] == 0) {
      break;
    }
  }
  FileStat stat;
  playdate->file->stat(path, &stat);
  text->file_stat = stat;

  // Close old file
  if (text->file != NULL) {
    playdate->file->close(text->file);
    playdate->system->realloc(text->chunks, 0);
  }
  text->file= file;

  // Chunks
  // HACK: Add 5 extra since total_chunks is a little less than actual
  text->total_chunks = (text->file_stat.size / BUFFER_SIZE) + 5;
  text->chunks = (TextChunk*)playdate->system->realloc(NULL, sizeof(TextChunk) * text->total_chunks);  
  
  for (int i = 0; i < text->total_chunks; i++) {
    text->chunks[i].start_index = -1;
    text->chunks[i].start_height = -1;
  }
  
  for (int i = 0; i < text->total_chunks; i++) {
    TextChunk* chunk = &text->chunks[i];
    if (chunk->start_index == -1) {
      break;
    }
    chunk->start_index = -1;
  }
  
  text->buffers_length = 0;
  text->buffers_index = 0;  
}

void textscroll_setFont(TextScroll* text, const char* path) {
  // Compare paths
  for (int i = 0; i < 100; i++) {    
    if (text->font_path[i] != path[i]) {
      break;
    }
        
    if (text->font_path[i] == 0 && path[i] == 0) {
      return;
    }
  }
    
  const char* err;
  LCDFont* font = playdate->graphics->loadFont(path, &err);
  if (!font) {
    return;
  }
  // Copy path
  for (int i = 0; i < 100; i++) {
    text->font_path[i] = path[i];
    if (path[i] == 0) {
      break;
    }
  }    
  text->font = font;
  
  for (int i = 0; i < text->total_chunks; i++) {
    TextChunk* chunk = &text->chunks[i];
    if (chunk->start_index == -1) {
      break;
    }
    chunk->start_index = -1;
  }
  
  text->buffers_length = 0;
  text->buffers_index = 0;  
}

void textscroll_setMargin(TextScroll* text, const unsigned int horizontal, const unsigned int vertical) {
  if (horizontal == text->margin_x && vertical == text->margin_y) {
    return;
  }
  
  text->margin_x = horizontal;
  text->margin_y = vertical;
  
  for (int i = 0; i < text->total_chunks; i++) {
    TextChunk* chunk = &text->chunks[i];
    if (chunk->start_index == -1) {
      break;
    }
    chunk->start_index = -1;
  }
  
  text->buffers_length = 0;
  text->buffers_index = 0;
}

float textscroll_getProgress(TextScroll* text) {
  return text->progress;  
}
