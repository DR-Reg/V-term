#define VTERM_C_SOURCE
#include "vterm.h"

bool VTermSpawnPTY(VTermPTY *pty) {
  char *env[] = {"TERM=dumb", NULL};

  pid_t p = fork();

  if (p == 0) {
    /* child process */
    close(pty->master);
    setsid();
    if (ioctl(pty->slave, TIOCSCTTY, NULL) == -1)
      return false;
    dup2(pty->slave, 0);
    dup2(pty->slave, 1);
    dup2(pty->slave, 2);
    close(pty->slave);
    execle(pty->shell, pty->shell, (char *)NULL, env);
    return false;
  } else if (p > 0) {
    /* parent process */
    close(pty->slave);
    return true;
  }

  VTermError("fork");
  return false;
}

bool VTermInitPTY(VTermPTY *pty) {
  /* reading and writing, opened pt is not the controlling terminal: */
  pty->master = posix_openpt(O_RDWR | O_NOCTTY);

  if (pty->master == -1) {
    VTermError("posix_openpt");
    return false;
  }

  /* grantpt gives us ownership of pt */
  if (grantpt(pty->master) == -1) {
    VTermError("grantpt(master)");
    return false;
  }

  /* unlockpt unlocks the pt for modification */
  if (unlockpt(pty->master) == -1) {
    VTermError("unlockpt(master)");
    return false;
  }

  /* get slave path name and open it */
  const char *slave_name = ptsname(pty->master);
  if (slave_name == NULL) {
    VTermError("ptsname(master)");
    return false;
  }

  pty->slave = open(slave_name, O_RDWR | O_NOCTTY);
  if (pty->slave == -1) {
    VTermError("open(slave_name)");
    return false;
  }

  pty->shell = "/bin/sh";
  return true;
}

bool VTermInit(VTerm *vt, const uint16_t width, const uint16_t height, VTermMode mode) {
  /***** RAYLIB InitWindow MUST HAVE BEEN CALLED *****/
  if (!IsWindowReady())
  {
    VTermError("Call Raylib's InitWindow before VTermInit");
    return false;
  }

  uint16_t i;
  /***** INIT ALL BUFFERS TO NULL *****/
  for (i = 0; i < MAX_BUFFER_COUNT; i++)
    vt->buffers[i] = NULL;

  /***** INITIALISE OUR FONTS LIST *****/
  //  For now all use this font
  for (i = 0; i < 21; i++)
    VTermTextFonts[i] = LoadFont_Px437();
  

  /***** SET UP FIRST BUFFER *****/
  vt->pixel_width = width;
  vt->pixel_height = height;

  if (!VTermInitBuffer(vt, 0, mode)) {
    VTermError("VTermInitBuffer(vt, 0, mode)");
    return false;
  }

  vt->buffer_ix = 0;

  return true;
}

bool VTermInitBuffer(VTerm *vt, uint16_t bix, VTermMode mode) {
  if (bix >= MAX_BUFFER_COUNT) {
    VTermError("bix >= MAX_BUFFER_COUNT");
    return false;
  }

  if (vt->buffers[bix] != NULL)
    VTermCloseBuffer(vt->buffers[bix]);

  vt->buffers[bix] = (VTermDataBuffer *)malloc(sizeof(VTermDataBuffer));

  VTermDataBuffer *buf = vt->buffers[bix];
  buf->mode = mode;
  buf->font_size = 10;

  bool pty;

  buf->col = 0;
  buf->row = 0;
  buf->font = VTermTextFonts[buf->mode];

  switch (buf->mode) {
    case VTERM_MODE_MONOCHROME_TEXT_40_25:
      buf->buffer_size = 40 * 25;
      buf->column_count = 40;
      buf->row_count = 25;
      pty = true;
      break;

    case VTERM_MODE_COLOR_TEXT_40_25:
    case VTERM_MODE_MONOCHROME_TEXT_80_25:
    case VTERM_MODE_COLOR_TEXT_80_25:
    case VTERM_MODE_4COLOR_GRAPHICS_300_200:
    case VTERM_MODE_MONOCHROME_GRAPHICS_300_200:
    case VTERM_MODE_MONOCHROME_GRAPHICS_640_200:
    case VTERM_MODE_COLOR_GRAPHICS_320_200:
    case VTERM_MODE_16COLOR_GRAPHICS_640_200:
    case VTERM_MODE_MONOCHROME_GRAPHICS_640_350:
    case VTERM_MODE_16COLOR_GRAPHICS_640_350:
    case VTERM_MODE_MONOCHROME_GRAPHICS_640_480:
    case VTERM_MODE_16COLOR_GRAPHICS_640_480:
    case VTERM_MODE_256COLOR_GRAPHICS_320_200:
    case VTERM_MODE_FULL_COLOR_MAX_RES:
      VTermError("switch(mode) - not implemented");
      return false;
    default:
      VTermError("switch(mode) - unknown");
      return false;
  }
  buf->data = (uint8_t *)calloc(buf->buffer_size, sizeof(uint8_t));
  if (pty) {
    if (!VTermInitPTY(&buf->pty)) {
      VTermError("VTermInitPTY(&buf->pty)");
      return false;
    }
    if (!VTermSpawnPTY(&buf->pty)) {
      VTermError("VTermSpawnPTY(&buf->pty)");
      return false;
    }
  }
  return true;
}

void VTermCloseBuffer(VTermDataBuffer *buf) {
  if (buf->data != NULL)
    free(buf->data);
  free(buf);
}

bool VTermUpdate(VTerm *vt)
{
  // TODO: check whether pty mode or not
  // TODO: key inputs
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  VTermPTY *pty = &buf->pty;
  char char_buf[1];

  /* make sure tv is not NULL, as o/w select blocks indefinitely */
  struct timeval tv;
  tv.tv_sec = 0; 
  tv.tv_usec = 0; 

  /* set of file descriptors ready to be read */
  fd_set readable;

  /* init to start as null set */
  FD_ZERO(&readable);

  /* add pty->master to the set */
  FD_SET(pty->master, &readable);

  /* query io from fd = 0 to pty->master */
  int fds_ready;
  if ((fds_ready = select(pty->master + 1, &readable, NULL, NULL, &tv)) == -1)
  {
    VTermError("select(master + 1, readable, NULL, NULL, NULL)");
    return false;
  }

  /* If master is in fact readable, read from it */
  if (FD_ISSET(pty->master, &readable))
  // if (true)
  {
    if (read(pty->master, char_buf, 1) <= 0)
    {
      VTermError("Child empty");
      return false;
    }

    // TODO: check for special characters
    //
    printf("Received: '%c' = '%d'\n", char_buf[0], char_buf[0]);
    if (char_buf[0] == '\r') 
    {
      buf->col = 0;
      return true;
    }
    if (char_buf[0] == '\n') 
    {
      buf->row++;
      return true;
    }
    if (char_buf[0] == '\b') 
    {
      buf->col--;
      return true;
    }

    // Else: store in data buffer
    buf->data[buf->col + buf->column_count * buf->row] = char_buf[0];
    buf->col++;
    if (buf->col >= buf->column_count)
    {
      buf->col = 0;
      buf->row++;
      if (buf->row > buf->row_count)
        VTermError("TODO");
    }
  }
  return true;
}

bool VTermDraw(VTerm *vt)
{
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  // TODO: check if pty mode or not
  int last_row_x = 0;
  for (uint16_t row = 0; row <= buf->row; row++)
  {
    char row_buf[buf->column_count + 1];
    // printf("row %d: 0x", row);
    uint16_t col;
    for (col = 0; col < buf->column_count; col++)
    {
      if (row == buf->row && col >= buf->col)
      {
        row_buf[col] = 0;
        break;
      }
      else
        row_buf[col] = buf->data[row * buf->column_count + col];
      // printf("%2x ", row_buf[col]);
    }
    row_buf[buf->column_count] = 0;
    // printf(" | '%s'\n", row_buf);
    // DrawText(row_buf, 0, row * buf->font_size, buf->font_size, RAYWHITE);
    DrawTextEx(buf->font, row_buf, (Vector2){0, row*buf->font_size}, buf->font_size, 0, RAYWHITE);

    // TODO: when custom fonts, use measureEx
    // For now as using Px437, can assume y = 2x
    if (row == buf->row)
      last_row_x = col * buf->font_size / 2;
      // last_row_x = MeasureText(row_buf, buf->font_size);
  }

  // draw cursor:
  DrawRectangle(last_row_x, buf->row * buf->font_size, buf->font_size / 2, buf->font_size, RAYWHITE);

  return true;
}

bool VTermSendInput(VTerm *vt) {
  int ch, kc;
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  int master = buf->pty.master;
  while ((ch = GetCharPressed()))
  {
    printf("Unicode %d pressed: '%c'\n", ch, *(char *)&ch);
    write(master, (const char *)&ch, 1);
  }
  while ((kc = GetKeyPressed()))
  {
    switch (kc)
    {
      case KEY_BACKSPACE:
        write(master, "\b", 1);
      case KEY_ENTER:
        write(master, "\n", 1);
    }
  }
  return true;
}

void VTermMoveCursorBy(int dx, int dy) {
  return;
}

void VTermIncreaseFontSize(VTerm *vt, int32_t delta)
{
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  buf->font_size += delta;
  VTermEnsureResolution(vt);
}

VTermDataBuffer *VTermGetCurrentBuffer(VTerm *vt)
{
  if (vt->buffer_ix >= MAX_BUFFER_COUNT)
  {
    VTermError("bix >= MAX_BUFFER_COUNT");
  }
  return vt->buffers[vt->buffer_ix];
}

void VTermEnsureResolution(VTerm *vt)
{
  // TODO: check and implement this for gfx types
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  vt->pixel_width = buf->column_count;
}
