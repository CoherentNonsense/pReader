#include "text.h"
#include <pd_api.h>

#define BUFFER_SIZE (1000)
#define BUFFERS_LENGTH (10)

PlaydateAPI* playdate;

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
  SDFile* file; // File to read from.
  FileStat file_stat; // File stats.
  char font_path[100]; // Font path (to compare with updated font).
  LCDFont* font; // Current font.
  int margin;
  
  int total_chunks;
  TextChunk* chunks;
  
  int buffers_index;
  int buffers_length;
  TextBuffer* buffers;
} TextScroll;

void textscroll_set_pd_ptr(PlaydateAPI* pd) {
  playdate = pd;
}

TextScroll* textscroll_new(const char* file_path, const char* font_path) {
  FileStat stat;
  int stat_result = playdate->file->stat(file_path, &stat);

  // If file doesn't exist  
  if (stat_result == -1) {
    return NULL;
  }
  
  TextScroll* text = (TextScroll*)playdate->system->realloc(NULL, sizeof(TextScroll));
  
  // Copy font path
  for (int i = 0; i < 100; i++) {
    text->font_path[i] = font_path[i];
    if (font_path[i] == 0) {
      break;
    }
  }    
  const char* err;
  text->file_stat = stat;  
  text->file = playdate->file->open(file_path, kFileReadData);
  text->font = playdate->graphics->loadFont(font_path, &err);
  
  text->margin = 5;
  
  // Chunks
  // HACK: Add 5 extra since total_chunks is a little less than actual
  text->total_chunks = (text->file_stat.size / BUFFER_SIZE) + 5;
  text->chunks = (TextChunk*)playdate->system->realloc(NULL, sizeof(TextChunk) * text->total_chunks);  
  
  for (int i = 0; i < text->total_chunks; i++) {
    text->chunks[i].start_index = -1;
    text->chunks[i].start_height = -1;
  }
  
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
  playdate->system->realloc(text->chunks, 0);
  playdate->system->realloc(text, 0);
}

static void textscroll_createChunk(TextScroll* text, int chunk_id, int index, int height) {
  text->chunks[chunk_id].start_index = index;
  text->chunks[chunk_id].start_height = height;
}

static int frame = 0;
// Only use with valid indices (Start of newline) and height.
static int textscroll_fillBuffer(TextScroll* text, int chunk_id) {
  playdate->system->logToConsole("FILLING %d", frame);  
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
  int max_width = 400 - (text->margin * 2);
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
  if (scroll < 0) {
    scroll = 0;
  }
  
  frame += 1;
  int buffer_id = textscroll_getBuffer(text, scroll);
  
  TextBuffer* buffer= &text->buffers[buffer_id];
  TextChunk* chunk = &text->chunks[buffer->owner_index];
  
  int next_buffer_id = textscroll_getBuffer(text, chunk->start_height + chunk->height);
      
  TextBuffer* next_buffer = &text->buffers[next_buffer_id];
  TextChunk* next_chunk = &text->chunks[next_buffer->owner_index];
    
  playdate->graphics->setFont(text->font);
  playdate->graphics->fillRect(text->margin, 0, 400 - (text->margin * 2) + 1, 240, kColorWhite);
  playdate->graphics->drawText(buffer->text, chunk->length, kUTF8Encoding, text->margin, -scroll + chunk->start_height);
  playdate->graphics->drawText(next_buffer->text, next_chunk->length, kUTF8Encoding, text->margin, -scroll + next_chunk->start_height);
  
  return scroll;
}

void textscroll_changeFont(TextScroll* text, const char* path) {
  // Compare paths
  for (int i = 0; i < 100; i++) {    
    if (text->font_path[i] != path[i]) {
      break;
    }
        
    if (text->font_path[i] == 0 && path[i] == 0) {
      return;
    }
  }
  // Copy path
  for (int i = 0; i < 100; i++) {
    text->font_path[i] = path[i];
    if (path[i] == 0) {
      break;
    }
  }    
    
  const char* err;
  LCDFont* font = playdate->graphics->loadFont(path, &err);
  if (!font) {
    return;
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
  
  text->font = font;
}

void textscroll_changeMargin(TextScroll* text, const unsigned int margin) {
  if (margin == text->margin) {
    return;
  }
  
  playdate->graphics->fillRect(text->margin, 0, 400 - (text->margin * 2), 240, kColorWhite);
  text->margin = margin;
  
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
