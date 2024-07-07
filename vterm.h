#include "raylib.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h>

#define MAX_BUFFER_COUNT 16
#define VTermError(str) printf("%s", str " failed\n")
// #define VTermError(str)                                                        \
//   write(2, __FILE__ ":" __LINE__ " " str " failed",                       \
//         sizeof(__FILE__ ":" __LINE__ " " str " failed"));

typedef enum {
  /* Regular VGA modes */
  VTERM_MODE_MONOCHROME_TEXT_40_25 = 0,
  VTERM_MODE_COLOR_TEXT_40_25 = 1,
  VTERM_MODE_MONOCHROME_TEXT_80_25 = 2,
  VTERM_MODE_COLOR_TEXT_80_25 = 3,
  VTERM_MODE_4COLOR_GRAPHICS_300_200 = 4,
  VTERM_MODE_MONOCHROME_GRAPHICS_300_200 = 5,
  VTERM_MODE_MONOCHROME_GRAPHICS_640_200 = 6,

  VTERM_MODE_COLOR_GRAPHICS_320_200 = 13,
  VTERM_MODE_16COLOR_GRAPHICS_640_200 = 14,
  VTERM_MODE_MONOCHROME_GRAPHICS_640_350 = 15,
  VTERM_MODE_16COLOR_GRAPHICS_640_350 = 16,
  VTERM_MODE_MONOCHROME_GRAPHICS_640_480 = 17,
  VTERM_MODE_16COLOR_GRAPHICS_640_480 = 18,
  VTERM_MODE_256COLOR_GRAPHICS_320_200 = 19,

  /* New custom modes */
  VTERM_MODE_FULL_COLOR_MAX_RES = 20
} VTermMode;

typedef struct {
  int master, slave;
  const char *shell;
} VTermPTY;

typedef struct {
  uint8_t *data;
  uint16_t column_count;
  uint16_t row_count;
  uint16_t col;
  uint16_t row;
  VTermPTY pty;   // pseudo-terminal
  VTermMode mode; // Mode this buffer is using
  size_t buffer_size;
  uint16_t font_size;
} VTermDataBuffer;

typedef struct {
  VTermDataBuffer *buffers[MAX_BUFFER_COUNT]; // At most can have MAX_BUFFERS

  uint16_t pixel_width;
  uint16_t pixel_height;
  uint16_t buffer_ix; // current buffer index
} VTerm;

bool VTermInit(VTerm *, const uint16_t, const uint16_t, VTermMode);
bool VTermSpawn(VTerm *);
bool VTermInitPTY(VTermPTY *);
bool VTermSpawnPTY(VTermPTY *);

/*   TODO: Set global variable VTERM_ERROR or something which is set if err
 * returned */
bool VTermUpdate(VTerm *);
bool VTermDraw(VTerm *);
bool VTermSendInput(VTerm *);

void VTermIncreaseFontBy(VTerm *, int32_t);
void VTermEnsureResolution(VTerm *);

bool VTermInitBuffer(VTerm *, uint16_t, VTermMode);
void VTermCloseBuffer(VTermDataBuffer *);

VTermDataBuffer *VTermGetCurrentBuffer(VTerm *);
