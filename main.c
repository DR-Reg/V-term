#include "raylib.h"
#include "vterm.h"

#include <stdio.h>

int main() {
  const uint16_t width = 800;
  const uint16_t height = 450;
  VTerm vt;

  VTermInit(&vt, width, height, VTERM_MODE_MONOCHROME_TEXT_40_25);

  InitWindow(width, height, "vterm");
  // SetTargetFPS(60);


  SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_UNDECORATED);
  SetExitKey(KEY_NULL);

  while (!WindowShouldClose()) {
    // Handle shortcuts first
    // TODO: set input polling timer for shortcuts so not
    // keydown very quick
    if (IsKeyDown(KEY_LEFT_SUPER))
    {
      if(IsKeyDown(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
        vt.buffers[vt.buffer_ix]->font_size++;
      if (IsKeyDown(KEY_MINUS) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
        vt.buffers[vt.buffer_ix]->font_size--;
    }
    // Input
    if (!VTermSendInput(&vt))
      return 1;

    // Update
    if (!VTermUpdate(&vt))
      return 1;

    // Draw
    BeginDrawing();
    ClearBackground(DARKGRAY);
    
    if (!VTermDraw(&vt))
      return 2;

    DrawFPS(width - 100, height-20);
    EndDrawing();
  }

  // De-Initialization
  CloseWindow();
  return 0;
}
