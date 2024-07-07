#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"


int main(int argc, char **argv)
{
  if (argc < 2)
  {
    puts("Usage: ./make_font_headers <file-1>.ttf ... <file-n>.ttf");
    return 1;
  }

  Font f;
  const char *prefix = "../fonts/headers/";
  int prelen = strlen(prefix);
  const char *suffix = ".h";
  int sufflen = strlen(suffix);
  const char *remprefix = "../fonts/int10h/pixel_outline_ttf/";
  int remprelen = strlen(remprefix);
  const char *remsuffix = ".ttf";
  int remsufflen = strlen(remsuffix);

  InitWindow(1,1,"font");

  for (int i = 1; i < argc; i++)
  {
    const char *filename = argv[i];
    int len = strlen(filename);
    if (strcmp(filename + len - 4, ".ttf"))
    {
      printf("Skipped %s: invalid filetype (only ttfs supported)\n", filename);
      continue;
    }
    f = LoadFont(filename);
    char *export_fname = (char *)malloc(len - remprelen - remsufflen + sufflen + prelen + 1); // NUL
    strcpy(export_fname, prefix);
    strcpy(export_fname + prelen, filename + remprelen);
    strcpy(export_fname + prelen + len - remprelen - remsufflen, suffix);
    printf("Exporting as %s... ", export_fname);
    if (!ExportFontAsCode(f, export_fname))
    {
      printf("Skipped %s: ExportFontAsCode failed.\n", filename);
      continue;
    }
    printf("Successfully exported %s to %s\n", filename, export_fname);
  }
  CloseWindow();
  return 0;
}
