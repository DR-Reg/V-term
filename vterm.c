#define VTERM_C_SOURCE
#include "vterm.h"

bool VTermSpawnPTY(VTermPTY *pty) {
  char *env[] = {"TERM=xterm-256color", NULL};

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

bool VTermInitPTY(VTermPTY **pty_ptr) {
  VTermPTY *pty = *pty_ptr;
  /* PTY should NULL or free'd if initialised */
  if (pty != NULL)
    free(*pty_ptr);
  pty = *pty_ptr = malloc(sizeof(VTermPTY));
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

  /***** INITIALISE OUR ANSI COLOR LIST *****/
#ifdef __BIG_ENDIAN__
  VTermANSIColors[0] = 0x000000ff;
  VTermANSIColors[1] = 0xff0000ff;
  VTermANSIColors[2] = 0x00ff00ff;
  VTermANSIColors[3] = 0xffff00ff;
  VTermANSIColors[4] = 0x0000ffff;
  VTermANSIColors[5] = 0xff00ffff;
  VTermANSIColors[6] = 0x00ffffff;
  VTermANSIColors[7] = 0xffffffff;
#elif defined __LITTLE_ENDIAN__
  VTermANSIColors[0] = 0xff000000;
  VTermANSIColors[1] = 0xff0000ff;
  VTermANSIColors[2] = 0xff00ff00;
  VTermANSIColors[3] = 0xff00ffff;
  VTermANSIColors[4] = 0xffff0000;
  VTermANSIColors[5] = 0xffff00ff;
  VTermANSIColors[6] = 0xffffff00;
  VTermANSIColors[7] = 0xffffffff;
#else
#error "Unknown endian"
#endif


  /***** SET UP FIRST BUFFER *****/
  vt->pixel_width = width;
  vt->pixel_height = height;

  if (!VTermInitBuffer(vt->buffers, mode)) {
    VTermError("VTermInitBuffer(vt, 0, mode)");
    return false;
  }

  vt->buffer_ix = 0;

  VTermEnsureResolution(vt);

  return true;
}

bool VTermInitBufferFrom(VTermDataBuffer **dest, VTermDataBuffer *src)
{
  if (!_VTermInitBuffer(dest, src->mode, false))
  {
    VTermError("_VTermInitBuffer(dest, src->mode, false)");
    return false;
  }
  (*dest)->pty = src->pty;
  return true;
}

bool VTermInitBuffer(VTermDataBuffer **buf_ptr, VTermMode mode)
{
  return _VTermInitBuffer(buf_ptr, mode, true);
}


bool _VTermInitBuffer(VTermDataBuffer **buf_ptr, VTermMode mode, bool pty) {
  VTermDataBuffer *buf = *buf_ptr;
  if (buf != NULL)
    VTermCloseBuffer(buf);

  buf = *buf_ptr = (VTermDataBuffer *)malloc(sizeof(VTermDataBuffer));

  buf->mode = mode;
  buf->font_size = 20;


  buf->col = 0;
  buf->row = 0;
  buf->font = VTermTextFonts[buf->mode];
  buf->alt_buffer = NULL;
  buf->pty = NULL;      // Inited below if needed
  buf->default_fgbg = ((uint64_t)*(uint32_t*)&RAYWHITE << 32) | *(uint32_t*)&DARKGRAY;
  printf("%8x : ", *(uint32_t*)&RAYWHITE);
  printf("DEFAULT: %2x%2x%2x%2x%2x%2x%2x%2x = %16llx\n", 
         // LITTLE ENDIAN
         RAYWHITE.a, 
         RAYWHITE.b, 
         RAYWHITE.g, 
         RAYWHITE.r, 

         DARKGRAY.a, 
         DARKGRAY.b, 
         DARKGRAY.g, 
         DARKGRAY.r,
         buf->default_fgbg);
  buf->fgbg_color = buf->default_fgbg;

  switch (buf->mode) {
    case VTERM_MODE_MONOCHROME_TEXT_40_25:
      buf->buffer_size = 40 * 25;
      buf->column_count = 40;
      buf->row_count = 25;
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
  buf->fgbg_colors = (uint64_t *)calloc(buf->buffer_size, sizeof(uint64_t));
  if (pty) {
    if (!VTermInitPTY(&buf->pty)) {
      VTermError("VTermInitPTY(buf->pty)");
      return false;
    }
    if (!VTermSpawnPTY(buf->pty)) {
      VTermError("VTermSpawnPTY(buf->pty)");
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

void VTermGetEscapeCodeArgs(VTermEscapeArgs *args, char *argstr, int arglen)
{
  args->count = 0; // o/w arglen <= 0 above
  if (arglen <= 0) return;
  int i, argix = 0, curr_argix = 0;
  char ch;
  args->count = 1; // o/w arglen <= 0 above
  // printf("ArgStr to parse: %s\n", argstr);
  for (i = 0; i < arglen; i++)
  {
    ch = argstr[i];
    if (ch == ';')
      ch = 0;

    args->args[argix][curr_argix++] = ch;

    if (ch == 0)
    {
      argix++;
      args->count++;
      curr_argix = 0;
    }
  }
  args->args[argix][curr_argix] = 0;
}

void VTermPrintEscapeCode(char *escape, VTermEscapeArgs *args)
{
  printf("ESC[%s", escape);
  printf("\tARGS(%d): ", args->count);
  for (int i = 0; i < args->count; i++)
  {
    printf("%s, ", args->args[i]);
  }
  printf("\n");
}

bool VTermResetBufferData(VTermDataBuffer *buf, uint16_t row, uint16_t col, VTermResetBufferDataDir dir)
{
  if (dir == VTERM_RESET_BUFFER_DATA_ALL)
  {
    if (!VTermResetBufferData(buf, row, col, VTERM_RESET_BUFFER_DATA_BACKWARDS))
    {
      VTermError("VTermResetBufferData(buf, row, col, VTERM_RESET_BUFFER_DATA_BACKWARS)");
      return false;
    }
    if (!VTermResetBufferData(buf, row, col, VTERM_RESET_BUFFER_DATA_FORWARDS))
    {
      VTermError("VTermResetBufferData(buf, row, col, VTERM_RESET_BUFFER_DATA_FORWARDS)");
      return false;
    }
  }
  else if (dir == VTERM_RESET_BUFFER_DATA_FORWARDS)
  {
    nmemset(buf->data + row * buf->column_count + col, 
             0, 
             buf->buffer_size - (row * buf->column_count + col));
    nmemset(buf->fgbg_colors + row * buf->column_count + col, 
             buf->default_fgbg, 
             buf->buffer_size - (row * buf->column_count + col));
  } else if (dir == VTERM_RESET_BUFFER_DATA_BACKWARDS)
  {
    nmemset(buf->data, 
             0, 
             row * buf->column_count + col);
    nmemset(buf->fgbg_colors, 
             buf->default_fgbg, 
             row * buf->column_count + col);
  }
  else if (dir == VTERM_RESET_BUFFER_DATA_UP)
  {
    if (row <= 0) return true;
    nmemset(buf->data, 
             0, 
             (row - 1) * buf->column_count);
    nmemset(buf->fgbg_colors, 
             buf->default_fgbg, 
             (row - 1) * buf->column_count);
  }
   else if (dir == VTERM_RESET_BUFFER_DATA_DOWN)
  {
    if (row >= buf->row_count - 1) return true;
    nmemset(buf->data + (row + 1) * buf->column_count, 
             0, 
             buf->buffer_size - ((row + 1) * buf->column_count));
    nmemset(buf->fgbg_colors + (row + 1) * buf->column_count, 
             buf->default_fgbg, 
             buf->buffer_size - ((row + 1) * buf->column_count));
  }
  else {
    VTermError("Invalid enum VTermResetBufferDataDir");
    return false;
  }

  return true;
}

// str at least 64
void VTermModeToStr(VTermMode mode, char *str)
{
  switch (mode)
  {
  case VTERM_MODE_MONOCHROME_TEXT_40_25:
    strcpy(str, "VTERM_MODE_MONOCHROME_TEXT_40_25");
    break;
  case VTERM_MODE_COLOR_TEXT_40_25:
    strcpy(str, "VTERM_MODE_COLOR_TEXT_40_25");
    break;
  case VTERM_MODE_MONOCHROME_TEXT_80_25:
    strcpy(str, "VTERM_MODE_MONOCHROME_TEXT_80_25");
    break;
  case VTERM_MODE_COLOR_TEXT_80_25:
    strcpy(str, "VTERM_MODE_COLOR_TEXT_80_25");
    break;
  case VTERM_MODE_4COLOR_GRAPHICS_300_200:
    strcpy(str, "VTERM_MODE_4COLOR_GRAPHICS_300_200");
    break;
  case VTERM_MODE_MONOCHROME_GRAPHICS_300_200:
    strcpy(str, "VTERM_MODE_MONOCHROME_GRAPHICS_300_200");
    break;
  case VTERM_MODE_MONOCHROME_GRAPHICS_640_200:
    strcpy(str, "VTERM_MODE_MONOCHROME_GRAPHICS_640_200");
    break;

  case VTERM_MODE_COLOR_GRAPHICS_320_200:
    strcpy(str, "VTERM_MODE_COLOR_GRAPHICS_320_200");
    break;
  case VTERM_MODE_16COLOR_GRAPHICS_640_200:
    strcpy(str, "VTERM_MODE_16COLOR_GRAPHICS_640_200");
    break;   
  case VTERM_MODE_MONOCHROME_GRAPHICS_640_350:
    strcpy(str, "VTERM_MODE_MONOCHROME_GRAPHICS_640_350");
    break;
  case VTERM_MODE_16COLOR_GRAPHICS_640_350:
    strcpy(str, "VTERM_MODE_16COLOR_GRAPHICS_640_350");
    break;
  case VTERM_MODE_MONOCHROME_GRAPHICS_640_480:
    strcpy(str, "VTERM_MODE_MONOCHROME_GRAPHICS_640_480");
    break;
  case VTERM_MODE_16COLOR_GRAPHICS_640_480:
    strcpy(str, "VTERM_MODE_16COLOR_GRAPHICS_640_480");
    break;
  case VTERM_MODE_256COLOR_GRAPHICS_320_200:
    strcpy(str, "VTERM_MODE_256COLOR_GRAPHICS_320_200");
    break;
  case VTERM_MODE_FULL_COLOR_MAX_RES:
    strcpy(str, "VTERM_MODE_FULL_COLOR_MAX_RES");
    break;
    default:
    strcpy(str, "Unknown mode");
  }
}

bool VTermExecuteEscapeCode(VTerm *vt, char *escape, int escape_len)
{
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  VTermEscapeArgs args;
  VTermResetBufferDataDir dir;

  VTermGetEscapeCodeArgs(&args, escape, escape_len - 1);

  bool high = false;
  

  switch (escape[escape_len - 1])
  {
    case 'H':
      if (args.count == 0)
      {
        buf->row = 0;
        buf->col = 0;
      } else if (args.count == 1)
      {
        sscanf(args.args[0], "%hd", &buf->row);
        buf->row--; // xterm row/col start at 1
        buf->col = 0;
      } else {
        sscanf(args.args[0], "%hd", &buf->row);
        sscanf(args.args[1], "%hd", &buf->col);
        buf->row--; // xterm row/col start at 1
        buf->col--; // xterm row/col start at 1
      }

      buf->row %= buf->row_count;
      buf->col %= buf->column_count;
      goto success;
    case 'J':
      // we don't support selective erase for now, go to success
      if (escape[0] == '?')
        goto success;

      if (args.count == 0) // default: erase below
        dir = VTERM_RESET_BUFFER_DATA_DOWN;
      else if (args.args[0][0] == '0')
        dir = VTERM_RESET_BUFFER_DATA_DOWN;
      else if (args.args[0][0] == '1')
        dir = VTERM_RESET_BUFFER_DATA_UP;
      else if (args.args[0][0] == '2')
        dir = VTERM_RESET_BUFFER_DATA_ALL;
      else if (args.args[0][0] == '3')
        // history not implemented, just go to success for now
        goto success;
      
      if (!VTermResetBufferData(buf, buf->row, buf->col, dir))
      {
        VTermError("VTermResetBufferData(buf, buf->row, buf->col, dir)");
        return false;
      }
      goto success;
    case 'K':
      // we don't support selective erase for now, go to success
      if (escape[0] == '?')
        goto success;

      if (args.count == 0) // default: erase below
        dir = VTERM_RESET_BUFFER_DATA_FORWARDS;
      else if (args.args[0][0] == '0')
        dir = VTERM_RESET_BUFFER_DATA_FORWARDS;
      else if (args.args[0][0] == '1')
        dir = VTERM_RESET_BUFFER_DATA_BACKWARDS;
      else if (args.args[0][0] == '2')
        dir = VTERM_RESET_BUFFER_DATA_ALL;
      
      if (!VTermResetBufferData(buf, buf->row, buf->col, dir))
      {
        VTermError("VTermResetBufferData(buf, buf->row, buf->col, dir)");
        return false;
      }
      goto success;
    case 'h':
      high = true;
    case 'l':
      if (escape[0] == '?')
      {
        VTermGetEscapeCodeArgs(&args, escape + 1, escape_len - 2);
        if (args.count > 0)
        {
          uint32_t n;
          sscanf(args.args[0], "%d", &n);
          switch (n)
          {
            case 1047:
            case 1049:
              if (high)
              {
                if (!VTermInAlternateBuffer(vt))
                {
                  VTermInitBufferFrom((VTermDataBuffer **)&buf->alt_buffer, buf);
                  char str[64];
                  VTermModeToStr(((VTermDataBuffer *)buf->alt_buffer)->mode, str);
                  printf("Type of altbuffer: %s\n", str);
                }
              }
              else
              {
                if (VTermInAlternateBuffer(vt))
                {
                  // buf is the alt-buffer!
                  VTermCloseBuffer((VTermDataBuffer *)buf);
                  VTermDataBuffer *pbuf = VTermGetCurrentPrincipalBuffer(vt);
                  pbuf->alt_buffer = NULL;
                }
              }
          }
        }
      }
      goto success;
    case 'm':
      if (args.count > 0)
      {
        for (int i = 0; i < args.count; i++)
        {
          uint32_t n, m10;
          sscanf(args.args[i], "%d", &n);
          m10 = n % 10;

          if (n == 0)
            buf->fgbg_color = buf->default_fgbg;
          else if (m10 <= 7 && n - m10 == 30)
            buf->fgbg_color = (uint64_t) VTermANSIColors[n % 10] << 32 | (buf->fgbg_color & 0xffffffff);
          else if (m10 <= 7 && n - m10 == 40)
            buf->fgbg_color = (uint64_t) (buf->fgbg_color & ((uint64_t) 0xffffffff << 32)) | VTermANSIColors[n % 10];
        }
      }

      goto success;
    default:
      return false;
  }
success:
  VTermPrintEscapeCode(escape, &args);
  return true;
}

bool VTermUpdate(VTerm *vt)
{
  // TODO: check whether pty mode or not
  // TODO: key inputs
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  VTermPTY *pty = buf->pty;
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
    // printf("Received: '%c' = #%d\tPWrap: %s\tPEsc: %s\tPCRAW: %s\tInEsc: %s\n",
    //        char_buf[0], char_buf[0],
    //        previousWasWrap ? "true" : "false",
    //        previousWasEscape ? "true" : "false",
    //        previousWasCRAfterWrap ? "true" : "false",
    //        currentEscapeIx >= 0 ? "true" : "false"
    // );
    // printf("Received %d\n", char_buf[0]);
    switch (char_buf[0])
    {
      case '\r':
        if (previousWasWrap)
          previousWasCRAfterWrap = true;
        buf->col = 0;
        break;
      case '\n':
        if (!previousWasWrap && !previousWasCRAfterWrap)
          buf->row++;
        break;
      case '\b':
        buf->col--;
        break;
      case '\t':
        memset(buf->data + buf->column_count * buf->row + buf->col, 32, 4);
        buf->col+= 4;
        break;
      case '\v':
        memset(buf->data + buf->column_count * buf->row + buf->col, 32, buf->column_count);
        buf->row++;
        break;
      case '\a':
        // TODO: good bell, allow for playing sound using esc codes
        system("osascript -e 'beep'");
        break;
      case '\33':
        // puts("ESCAPE CODE");
        previousWasEscape = true;
        return true; // not affect buffer/cursor
      case '[':
        if (previousWasEscape)
        {
          currentEscapeIx = 0;
          previousWasEscape = false;
          return true; // don't affect buffer/cursor
        }
      default:
        // If in escape (currEscIx >= 0)
        if (currentEscapeIx >= 0)
        {
          if (currentEscapeIx >= 32)
            currentEscapeIx = -1;
          else {
            currentEscapeBuf[currentEscapeIx++] = char_buf[0];
            if (VTermExecuteEscapeCode(vt, currentEscapeBuf, currentEscapeIx))
            {
              memset(currentEscapeBuf, 0, 32);
              currentEscapeIx = -1;
            }
          }
          // escapes don't affect cursor
          return true;
        } else {
          // Else: store in data buffer
          buf->data[buf->col + buf->column_count * buf->row] = char_buf[0];
          buf->fgbg_colors[buf->col + buf->column_count * buf->row] = buf->fgbg_color;
          buf->col++;
        }
    }

    if (char_buf[0] != '\33' && previousWasEscape)
      previousWasEscape = false;

    if (buf->col >= buf->column_count)
    {
      buf->col = 0;
      buf->row++;
      previousWasWrap = true;
    } else {
      previousWasWrap = false;
    }
  
    if (previousWasCRAfterWrap && char_buf[0] != '\r')
      previousWasCRAfterWrap = false;

    if (buf->row >= buf->row_count)
    {
      memmove(buf->data, buf->data + buf->column_count, buf->buffer_size - buf->column_count);
      memset(buf->data + buf->buffer_size - buf->column_count, 0, buf->column_count);

      memmove(buf->fgbg_colors, buf->fgbg_colors + buf->column_count, sizeof(uint64_t)*(buf->buffer_size - buf->column_count));
      memset(buf->fgbg_colors + buf->buffer_size - buf->column_count, buf->default_fgbg, sizeof(uint64_t)*buf->column_count);
      buf->row--;
    }
  }
  return true;
}

bool VTermIsTextMode(VTermDataBuffer *buf)
{
  switch (buf->mode)
  {
    case VTERM_MODE_MONOCHROME_TEXT_40_25:
    case VTERM_MODE_COLOR_TEXT_40_25: 
    case VTERM_MODE_MONOCHROME_TEXT_80_25:
    case VTERM_MODE_COLOR_TEXT_80_25:
      return true;

    default:
      return false;
  }
}

bool VTermDrawText(VTermDataBuffer *buf)
/* Adapted from Raylib's DrawTextEx */
{
  int textLineSpacing = 0;
  Font font = buf->font;
  Vector2 position = (Vector2){0, 0};
  float fontSize = buf->font_size;
  float spacing = 0;

  uint32_t default_bg = UNPACK_bg(buf->default_fgbg);
  if (font.texture.id == 0) font = GetFontDefault();  // Security check in case of not valid font

  float textOffsetY = 0;          // Offset between lines (on linebreak '\n')
  float textOffsetX = 0.0f;       // Offset X to next character to draw

  float scaleFactor = fontSize/font.baseSize;         // Character quad scaling factor

  for (int i = 0; i < buf->buffer_size;)
  {
    // Get next codepoint from byte string and glyph index in font
    if (buf->data[i] == 0) { i++; continue; }
    int codepointByteCount = 0;
    int codepoint = GetCodepointNext((const char *)(buf->data + i), &codepointByteCount);
    uint32_t fg = UNPACK_fg(buf->fgbg_colors[i]);
    uint32_t bg = UNPACK_bg(buf->fgbg_colors[i]);
    Color tint = *(Color*)&fg;
    Color back = *(Color*)&bg;
    int index = GetGlyphIndex(font, codepoint);

    if (i != 0 && i % buf->column_count == 0)
    {
      textOffsetY += (fontSize + textLineSpacing);
      textOffsetX = 0.0f;
    }


    Vector2 where = (Vector2){ position.x + textOffsetX, position.y + textOffsetY };
    if (bg != default_bg)
      DrawRectangle(where.x, where.y, fontSize / 2, fontSize, back);
    if ((codepoint != ' ') && (codepoint != '\t'))
    {
      DrawTextCodepoint(font, codepoint, where, fontSize, tint);
    }

    if (font.glyphs[index].advanceX == 0) textOffsetX += ((float)font.recs[index].width*scaleFactor + spacing);
    else textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
    i += codepointByteCount;   // Move text bytes counter to next codepoint
  }
  return true;
}

bool VTermDraw(VTerm *vt)
{
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  // TODO: check if pty mode or not
  VTermDrawText(buf);
  // draw cursor:
  DrawRectangle(buf->col * buf->font_size / 2, buf->row * buf->font_size, buf->font_size / 2, buf->font_size, RAYWHITE);

  return true;
}

bool VTermSendInput(VTerm *vt) {
  int ch, kc;
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  int master = buf->pty->master;
  while ((ch = GetCharPressed()))
  {
    // printf("Unicode %d pressed: '%c'\n", ch, *(char *)&ch);
    write(master, (const char *)&ch, 1);
  }
  while ((kc = GetKeyPressed()))
  {
    switch (kc)
    {
      case KEY_BACKSPACE:
        write(master, "\b", 1);
        break;
      case KEY_ENTER:
        write(master, "\n", 1);
        break;
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
  if (vt->buffers[vt->buffer_ix]->alt_buffer == NULL)
    return vt->buffers[vt->buffer_ix];
  return vt->buffers[vt->buffer_ix]->alt_buffer;
}

VTermDataBuffer *VTermGetCurrentPrincipalBuffer(VTerm *vt)
{
  if (vt->buffer_ix >= MAX_BUFFER_COUNT)
  {
    VTermError("bix >= MAX_BUFFER_COUNT");
  }
  return vt->buffers[vt->buffer_ix];
}

bool VTermInAlternateBuffer(VTerm *vt)
{
  return VTermGetCurrentPrincipalBuffer(vt)->alt_buffer != NULL;
}

void VTermEnsureResolution(VTerm *vt)
{
  // TODO: check and implement this for gfx types
  // TODO: check for fullscreen (margin)
  VTermDataBuffer *buf = VTermGetCurrentBuffer(vt);
  vt->pixel_width = buf->column_count * buf->font_size/2;
  vt->pixel_height = buf->row_count * buf->font_size;
  SetWindowSize(vt->pixel_width, vt->pixel_height);
}

