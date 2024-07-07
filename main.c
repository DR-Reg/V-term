#include "raylib.h"
#include "vterm.h"

#include <stdio.h>

int main() {
  const uint16_t width = 800;
  const uint16_t height = 450;
  VTerm vt;

  InitWindow(width, height, "vterm");

  if (!VTermInit(&vt, width, height, VTERM_MODE_MONOCHROME_TEXT_40_25))
  {
    return -1;
  }
  // SetTargetFPS(60);


  SetWindowState(FLAG_WINDOW_UNDECORATED);
  SetExitKey(KEY_NULL);

  while (!WindowShouldClose()) {
    // Handle shortcuts first
    // 1 must be keydown so polling not too quick
    if (IsKeyDown(KEY_LEFT_SUPER))
    {
      if(IsKeyPressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
        VTermIncreaseFontSize(&vt, 1);
      if (IsKeyPressed(KEY_MINUS) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
        VTermIncreaseFontSize(&vt, -1);
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

    DrawFPS(vt.pixel_width - 100, 0);
    EndDrawing();
  }

  // De-Initialization
  CloseWindow();
  return 0;
}
