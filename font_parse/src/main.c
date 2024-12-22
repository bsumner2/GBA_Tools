
#include "SDL_events.h"
#include "SDL_video.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <assert.h>
#include <stdbool.h>
#include "bmp_parse.h"

#define perr(s) fputs("\x1b[1;31m[Error]:\x1b[0m "s, stderr)
#define perrf(fmt, ...) fprintf(stderr, "\x1b[1;31m[Error]:\x1b[0m "fmt, __VA_ARGS__)
#define warn(s) fputs("\x1b[1;32m[Warning]:\x1b[0m "s, stderr)
#define warnf(fmt, ...) fprintf(stderr, "\x1b[1;32m[Warning]:\x1b[0m "fmt, __VA_ARGS__)


#define PIXEL(bmp_, x, y) bmp_.pbuf[y*bmp_.width + x]

int cell_width, glyph_height, cell_height;

typedef struct s_FontCtx {
  uint8_t *glyph_set;
  uint16_t *glyph_widths;
  uint16_t glyph_height;
  uint16_t cell_width, cell_height;
  uint32_t cell_size; 
  uint32_t glyph_ct;
} FontCtx_t;


typedef struct s_LinkedListNode {
  uint8_t *glyph;
  uint32_t width;
  struct s_LinkedListNode *next;
} LL_Node_t;

typedef struct s_LinkedList {
  LL_Node_t *head, *tail;
  int len;
} LinkedList_t;

typedef struct {
  uint8_t *pbuf;
  uint32_t width;
} Font_Glyph_t;

LinkedList_t *LL_Init(void) {
  return (LinkedList_t*)calloc(1, sizeof(LinkedList_t));
}

LL_Node_t *New_Node(uint8_t *glyph, uint16_t width) {
  LL_Node_t *ret = malloc(sizeof(LL_Node_t));
  ret->glyph = glyph;
  ret->width = width;
  ret->next = NULL;
  return ret;
}


bool LL_Add(LinkedList_t *list, uint8_t *glyph, uint16_t width) {
  if (!list)
    return false;
  if (!(list->len)) {
    list->head = list->tail = New_Node(glyph, width);
    if (!(list->tail))
      return false;
    ++list->len;
    return true;
  }

  list->tail->next = New_Node(glyph, width);
  list->tail = list->tail->next;
  if (!list->tail)
    return false;
  ++list->len;
  return true;
}

bool LL_Dequeue(LinkedList_t *list, Font_Glyph_t *dest) {
  if (!list->len)
    return false;
  LL_Node_t *node = list->head;
  list->head = node->next;
  *dest = *((Font_Glyph_t*)node);
  free((void*)node);
  --list->len;
  return true;
}

bool LL_Close(LinkedList_t *list, _Bool from_err) {
  LL_Node_t *cur, *nxt;
  cur = list->head;
  if (from_err) { 
    // ghetto way to skip next check without having to nest
  } else if (list->len || cur) {
    perrf("Stragglers remained despite finishing the dequeueing."
        "\n\t\x1b[1;34mList Len: %d\n\tNodes present: %s\x1b[0m\n",
        list->len, cur ? "True" : "False");
  }

  while (cur) {
    nxt = cur->next;
    free((void*)(cur->glyph));
    free((void*)cur);
    cur = nxt;
    --list->len;
  }
  if ((list->len)) {
    perrf("When closing list, unexpected length reached by end of list: %d\n", list->len);
    free((void*)list);
    return false;
  }
  free((void*)list);
  return true;
}

void Glyph_Get_Dimms(Pal_BMP_t *gset) {
  uint8_t *cursor = gset->pbuf;
  cell_width = 0;
  while (*cursor)
    ++cell_width, ++cursor;
  while (!*cursor++)
    ++cell_width;
  cursor = gset->pbuf;
  glyph_height = 0;
  while (*cursor)
    glyph_height++, cursor += gset->width;
  cell_height = glyph_height;
  while (!*cursor)
    cell_height++, cursor += gset->width;
  
}


ssize_t Next_Glyph(Pal_BMP_t *gset, LinkedList_t *glist, size_t gofs) {
  uint8_t *glyph = malloc(cell_height);
  const uint8_t *crsr = gset->pbuf + gofs;
  int i;
  uint8_t tmp;
  *glyph = 0;
  for (tmp = crsr[i=0]; tmp && i < cell_width; tmp = crsr[++i]) {
    if (tmp&2)
      glyph[0] |= 1<<i;
  }
  
  uint16_t w;
  crsr += gset->width; 
  w = i;
  for (i = 1; i < glyph_height; ++i) {
    glyph[i] = 0;
    for (int j = 0; j < w; ++j) {
      if (crsr[j]&2)
        glyph[i] |= 1<<j;
    }
    crsr += gset->width;
  }

  for ( ; i < cell_height; ++i) {
    glyph[i] = 0;
  }


  
  if (!LL_Add(glist, glyph, w)) {
    free((void*)(glyph));
    return -1;
  }
  gofs += cell_width;
  if (gofs % gset->width)
    return gofs;
  

#if 0
  gofs += gset->width * (glyph_padded_height-1);
#else
  gofs -= gset->width;
  gofs += gset->width * cell_height;
#endif
  
  return gofs;
  

}

bool Font_Parse(FontCtx_t *dest, Pal_BMP_t *glyph_set) {
  Font_Glyph_t tmp;
  LinkedList_t *glyph_list = LL_Init();
  uint8_t *glyph_buf_cursor;
  uint16_t *widths_cursor;
  ssize_t ofs = 0, lim = (glyph_set->width)*(glyph_set->height);


  do {
    ofs = Next_Glyph(glyph_set, glyph_list, ofs);
    if (0 > ofs) {
      LL_Close(glyph_list, true);
      return false;
    }
  } while (ofs < lim);
  
  dest->glyph_set = glyph_buf_cursor = malloc(cell_height*(glyph_list->len));
  dest->glyph_widths = widths_cursor = malloc(sizeof(uint16_t)*(glyph_list->len));
  dest->cell_size = cell_height;
  dest->cell_height = cell_height;
  dest->cell_width = cell_width;
  dest->glyph_height = glyph_height;
  dest->glyph_ct = glyph_list->len;
  while (LL_Dequeue(glyph_list, &tmp)) {
    for (int i = 0; i < cell_height; ++i) {
      *glyph_buf_cursor++ = tmp.pbuf[i];
    }
    *widths_cursor++ = (uint16_t)tmp.width;
  }

  LL_Close(glyph_list, false);
  return true;
}

void Font_Close(FontCtx_t *font) {
  if (!font)
    return;
  if (font->glyph_set) {
    free((void*)(font->glyph_set));
    font->glyph_set = NULL;
  }
  if (font->glyph_widths) {
    free((void*)(font->glyph_widths));
    font->glyph_widths = NULL;
  }
  font->cell_size = 0;
  font->glyph_ct = 0;
}

bool Export_Font(FontCtx_t *font, const char *font_name, const char *outdir, int lb, int ub) {
  int fname_len = strlen(font_name), outdir_len = strlen(outdir),
      hdr_guard_namelen = fname_len + 4 /* 4=strlen("__H_")*/, 
      file_name_len = fname_len + outdir_len + 2  /* strlen(".h") */ + 
        (outdir[outdir_len-1] == '/' ? 0 : 1);  // + outdir path len + (1 only if outdir doesn't end with '/')
  
  uint32_t glyph_ct = (lb >= 0 && ub > 0 && ub <= font->glyph_ct) ? ub-lb+1:font->glyph_ct;
  char hdr_guard[hdr_guard_namelen+1], file_name[file_name_len+1];
  memset(file_name, 0, sizeof(file_name));
  {
    const char *fcur = font_name, *outdir_cur = outdir;
    char *hdr_cur = &hdr_guard[1], *fn_cur = &file_name[0], c;
    *(hdr_cur-1) = '_';

    for (c = *outdir_cur++; c; c = *outdir_cur++)
      *fn_cur++ = c;
    if (outdir[outdir_len-1] != '/')
      *fn_cur++ = '/';

    while ((c = *fcur++)) {
      *fn_cur++ = c;
      *hdr_cur++ = toupper(c);
    }
    *fn_cur++ = '.';
    *fn_cur++ = 'h';
    *fn_cur++ = '\0';
    *hdr_cur++ = '_';
    *hdr_cur++ = 'H';
    *hdr_cur++ = '_';
    *hdr_cur++ = '\0';
  }

  printf("outpath: %s\n", file_name);
  FILE *fp = fopen(file_name, "w");
  if (!fp)
    return false;
  fprintf(fp, "#ifndef %s\n#define %s\n\n", hdr_guard, hdr_guard);
  fputs("#ifdef __cplusplus\nextern \"C\" {\n#endif  /* CXX Name Mangler Guard */\n\n", fp);

  fprintf(fp, "#define %s_CellWidth %d\n", font_name, font->cell_width);
  fprintf(fp, "#define %s_CellHeight %d\n", font_name, font->cell_height);
  
  fprintf(fp, "#define %s_GlyphHeight %d\n", font_name, font->glyph_height);
  fprintf(fp, "#define %s_GlyphCount %d\n", font_name, glyph_ct);

  
  fprintf(fp, "\n/* Uniform cell size (in bytes) for each glyph.\n"
      " * (Use as offset for glyph indexing): */\n"
      "#define %s_CellSize %d\n\n", font_name, font->cell_size);

  fprintf(fp, "/* Glyph pixel data. 1bpp. */\n"
      "extern const unsigned short %s_GlyphData[%d];\n"
      "extern const unsigned char %s_GlyphWidths[%d];\n", 
      font_name, (font->cell_size*glyph_ct)>>1, font_name, glyph_ct);
  

  fputs("#ifdef __cplusplus\n}\n#endif  /* CXX Name Mangler Guard */\n\n", fp);
  fprintf(fp, "#endif  /* %s */\n\n", hdr_guard);


  fclose(fp);
  file_name[file_name_len-1] = 'c';
  if (!(fp = fopen(file_name, "w")))
    return false;
  fprintf(fp,
      "/*****************************************\n"
      " * [[[Auto-Generated by Burt's Font Parser\n"
      " * Font name: %s\n"
      " * Glyph Count: %d\n"
      " * Glyph Cell Dims (WxH): %dx%d\n"
      " * Glyph Cell Size (Bytes): %d\n"
      " * Glyph bit depth (bpp): %d\n"
      " ****************************************/\n\n\n",
      font_name, glyph_ct, font->cell_width, font->cell_height,
      font->cell_size, 1);
  
  int data_nmemb =  (font->cell_size*glyph_ct)>>1;
  if (data_nmemb&1) {
    perrf("Glyph data buffer can't be stored unsigned short-wise since data size is not divisible by 2.\n"
        "Data length: %d", font->cell_size*font->glyph_ct);
    fclose(fp);
    return false;
  }

  fprintf(fp, "const unsigned short %s_GlyphData[%d] = {", font_name, data_nmemb);
  {
    uint16_t *data = (uint16_t*)(font->glyph_set + (font->cell_size*(glyph_ct!=font->glyph_ct ? lb : 0)));
    int last_idx = data_nmemb-1, i;
    for (i = 0; i < last_idx; ++i) {
      if (!(i&7)) {
        fprintf(fp, "\n  0x%04x,", data[i]);
      } else {
        fprintf(fp, " 0x%04x,", data[i]);
      }
    }
    
    if (!(last_idx&7)) {
      fprintf(fp, "\n  0x%04x\n};\n\n", data[last_idx]);
    } else {
      fprintf(fp, " 0x%04x\n};\n\n", data[last_idx]);
    }
  }
  
  {
    uint16_t *widths = font->glyph_widths + (glyph_ct != font->glyph_ct ? lb : 0);
    int i, last_idx;
    data_nmemb = glyph_ct;
    last_idx = data_nmemb-1;
    fprintf(fp, "const unsigned char %s_GlyphWidths[%d] = {", font_name, data_nmemb);
    for (i = 0; i < last_idx; ++i) {
      if (!(i&7)) {
        fprintf(fp, "\n  0x%02x,", 0xFF&widths[i]);
      } else {
        fprintf(fp, " 0x%02x,", 0xFF&widths[i]);
      }
    }

    if (!(last_idx&7)) {
      fprintf(fp, "\n  0x%02x\n};\n\n", 0xFF&widths[last_idx]);
    } else {
      fprintf(fp, " 0x%02x\n};\n\n", 0xFF&widths[last_idx]);
    } 
  }
  
  fclose(fp);


  return true;

  
  


  

}

bool Check_FontName(const char *name) {
  char c;
  for (c = *name; c; c = *++name) {
    if (!isalpha(c) && c!='_')
      return false;
  }

  return true;
}

typedef struct {
  uint16_t l, r;
} Bound_t;
  
typedef Bound_t RowBounds[8];
static int BoundsSearch(RowBounds, int, int, int);
int FindGlyphUnderCursor(RowBounds *bounds_by_row, int cell_height, int cursor_x, int cursor_y) {
  int row = cursor_y / cell_height;
  return BoundsSearch(bounds_by_row[row], cursor_x, 0, 8);
}

int BoundsSearch(RowBounds row, int x, int lb, int len) {
  Bound_t bounds;
  if (len < 2) {
    if (!len)
      return -1;
    bounds = row[lb];
    return bounds.l <= x && bounds.r > x ? lb : -1;
  }

  int midpt = (len>>1) + lb;
  bounds = row[midpt];
  if (bounds.l > x)
    return BoundsSearch(row, x, lb, len>>1);
  else if (bounds.r <= x)
    return BoundsSearch(row, x, midpt + 1, len-midpt-1);
  else
    return midpt;
  
}


void verdana_render_glyph_id(SDL_Renderer *ren, FontCtx_t *font, const int win_width, const int yofs, int idx) {
  uint8_t *glyph_set = font->glyph_set, *glyph;
  uint16_t *widths = font->glyph_widths, height = font->glyph_height, curr_width;
  int cell_len = font->cell_size;
  int slen;
  static const int VERDANA_CHAR_OFS = (int)' ';
  if (0 > idx)
    slen = snprintf(NULL, 0ULL, "Idx = N/A");
  else
    slen = snprintf(NULL, 0ULL, "Idx = %d", idx);
  char str[slen+1];
  memset(str, 0, slen+1);
  if (0 > idx)
    snprintf(str, slen+1, "Idx = N/A");
  else
    snprintf(str, slen+1, "Idx = %d", idx);
  char *crsr=str, cur;
  SDL_Rect r = (SDL_Rect){.w = 2, .h = 2, .x = 0, .y = yofs};
  while ((cur=*crsr++)) {
    cur -= VERDANA_CHAR_OFS;
    if (cur >= 0x5F) {
      continue;
    }
    glyph = glyph_set + cell_len*cur;
    curr_width = widths[(int)cur];
    for (int i = 0; i < height; ++i) {
      for (int j = 0; j < curr_width; ++j) {
        if (glyph[i]&(1<<j))
          SDL_SetRenderDrawColor(ren, 255,255,255,255);
        else
          SDL_SetRenderDrawColor(ren, 0,0,0,255);
        SDL_RenderDrawRect(ren, &r);
        r.x += 2;
      }
      r.x -= curr_width*2;
      r.y += 2;
    }
    r.x += curr_width*2;
    r.y = yofs;
  }
  r.w = win_width - r.x;
  r.h = glyph_height*2;
  SDL_SetRenderDrawColor(ren, 0,0,0,255);
  SDL_RenderFillRect(ren, &r);

}

void print_bad_range_arg_error(const char *arg) {
  perrf("Invalid font range arg, \"%s\".\n"
      "\t\x1b[1;34mProper format: \x1b[39m<lower bound>-<upper bound>\x1b[0m\n"
      "\tWhere lower bound and upper bound are numbers indicating the\n"
      "\tindex range of chars from the parsed image to be put in the outputted font,\n"
      "\tand lower bound < upper bound.\n"
      "\t\t\x1b[1;3;34m[E.g.]:\x1b[39m 0-95\x1b[22m indicates to only add the first 96\n"
      "\t\tchars parsed to the outputted font asset.\x1b[0m\n", arg);

}

void print_usage_and_exit(const char *err_message, const char *exename) {
  perrf("%s. Usage below:\n"
    "\t\x1b[1m%s <bmp path> <font name> [option args (opts)]\x1b[0m\n"
    "\t\t\x1b[1;34m[Options (Opts)]:\x1b[0m\n"
    "\t\t\t\x1b[1;32m--outdir\x1b[33m <output dir path>\x1b[0m\n"
    "\t\t\t\x1b[1;32m--range\x1b[33m <index range>\x1b[0m\n", err_message, exename);
  exit(1);
} __attribute__ ((noreturn));

int main(int argc, char *argv[]) {

  Pal_BMP_t bmp;
  FontCtx_t font = {0};
  char *outdir = "./";
  int outcome, lower_bound = -1, upper_bound = -1;
  bool is_verdana = false;
  if (argc!=3) {
    if (5!=(argc&5) || 0!=(argc&(~7))) {
      if (argc==2 && *(uint16_t*)argv[1] == *(uint16_t*)"-h") {
        printf("\x1b[1;34m%s\x1b[0m: A glyphset parser for creating raw, 1 "
            "bit-per-pixel font resources in C for the GBA\n"
            "\t\x1b[1;31m[Usage]:\x1b[39m \x1n[34m%s\x1b[33m <glyphset bmp path> "
            "<font name> \x1b[32m[option args (opts)]\x1b[0m\n"
            "\t\t\x1b[1;34m[Options (Opts)]:\x1b[0m\n"
            "\t\t\t\x1b[1;32m--outdir\x1b[33m <output dir path>\n"
            "\t\t\t\x1b[1;32m--range\x1b[33m <index range>\n" , 
            argv[0], argv[0]);
        return 0;
      }
      print_usage_and_exit("Invalid arg count", argv[0]);

    }

    int range_idx, outdir_idx;
    range_idx = outdir_idx = -1;
    if (!strcmp(argv[3], "--range")) {
      range_idx = 4;
    } else if (!strcmp(argv[3], "--outdir")) {
      outdir_idx = 4;
    } else {
      print_usage_and_exit("Invalid option flag", argv[0]);
    }
    if (argc==7) {
      if (!strcmp(argv[5], "--range")) {
        if (range_idx!=-1) {
          warnf("Ignoring duplicate opt flag for range option, --range."
              "Using previously-declared range, %s", argv[4]);
        } else {
          range_idx = 6;
        }
      } else if (!strcmp(argv[5], "--outdir")) {
        if (outdir_idx != -1) {
          warnf("Ignoring duplicate opt flag for range option, --outdir."
              "Using previously-declared output directory, %s", argv[4]);
        } else {
          outdir_idx = 6;
        }
      } else {
        print_usage_and_exit("Invalid option flag", argv[0]);
      }
    }


    char *range = 0>range_idx ? NULL : argv[range_idx];
    if (range) {
      char c, *cur = range;
      bool hyphen_reached = false;
      while ((c = *cur++)) {
        if (c == '-') {
          if (hyphen_reached) {
            print_bad_range_arg_error(argv[range_idx]);
            return 1;
          }
          hyphen_reached = true;
          continue;
        }

        if (!isdigit(c)) {
          print_bad_range_arg_error(argv[range_idx]);
          return 1;
        }
      }

      if (!hyphen_reached) {
        print_bad_range_arg_error(argv[range_idx]);
        return 1;
      }
      
      cur = NULL;
      if ((lower_bound = strtol(range, &cur, 10)) < 0) {
        print_bad_range_arg_error(argv[range_idx]);
        return 1;
      }
      if (!(upper_bound = strtol(++cur, &cur, 10)) || upper_bound <= lower_bound) {
        print_bad_range_arg_error(argv[range_idx]);
        return 1;
      }

      printf("%d-%d\n", lower_bound, upper_bound);
    }
    if (outdir_idx > 0) {
      outdir = argv[outdir_idx];
    }
  }
  

  if (!Check_FontName(argv[2])) {
    perrf("Invalid font name, \x1b[1m%s\x1b[0m. Font name must only consist"
        " of alphabetical characters and underscores.\n", argv[2]);
    return 1;
  }

    
  is_verdana = strcmp(argv[1], "verdana9.bmp");
 
  outcome = Pal_BMP_Parse(&bmp, argv[1]);
  if (0 > outcome) {
    perrf("Parsing failed. Details below:\n\t%s\n", BMP_Parse_Strerror(outcome));
    return 1;
  }


  
#if 0
  SDL_Window *win = NULL;
  SDL_Renderer *ren = NULL;

  if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
    perrf("SDL failed to initialize. Details below:\n\t%s\n", SDL_GetError());
    Pal_BMP_Close(&bmp);
    return 1;
  }
  if (0 > SDL_CreateWindowAndRenderer(bmp.width, bmp.height, SDL_WINDOW_SHOWN, &win, &ren)) {

    perrf("SDL failed to create window and renderer.\nDetails below:\n\t%s\n", SDL_GetError());
    Pal_BMP_Close(&bmp);
    SDL_Quit();
    return 1;
  }
  // Draw whatever is needed here.

  SDL_RenderPresent(ren);
  Pal_BMP_Close(&bmp);

  SDL_Event ev;
  _Bool quit = 0;
  while (!quit) {
    while (SDL_PollEvent(&ev)) {
      if ((quit = (ev.type==SDL_QUIT)))
        break;
    }
  }
  SDL_DestroyRenderer(ren);

  SDL_DestroyWindow(win);

  SDL_Quit();
  return 0;
#else
  Glyph_Get_Dimms(&bmp);

  if (cell_width!=8) {
    perrf("\x1b[1mUnexpected width for glyphs.\n\tActual Glyph Width: \x1b[34m%d\x1b[0m\n\t\x1b[1mExpected: \x1b[34m8\x1b[0m\n", cell_width);
    Pal_BMP_Close(&bmp);
    return 1;
  }
  if (!Font_Parse(&font, &bmp)) {
    perr("Something failed during font parsing.\n");
    Pal_BMP_Close(&bmp);
    return 1; 
  }


  SDL_Window *win = NULL;
  SDL_Renderer *ren = NULL;

  if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
    perrf("SDL failed to initialize. Details below:\n\t%s\n", SDL_GetError());
    Font_Close(&font);
    Pal_BMP_Close(&bmp);
    return 1;
  }
  const int WIN_WIDTH = (cell_width*2)*8;
  const int FONT_WIN_HEIGHT = (glyph_height*2)*(font.glyph_ct/8);
  const int WIN_HEIGHT = FONT_WIN_HEIGHT + (2*font.glyph_height);
  if (0 > SDL_CreateWindowAndRenderer(WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN|SDL_WINDOW_BORDERLESS, &win, &ren)) {

    perrf("SDL failed to create window and renderer.\nDetails below:\n\t%s\n", SDL_GetError());
    Font_Close(&font);
    Pal_BMP_Close(&bmp);
    SDL_Quit();
    return 1;
  }
  SDL_Rect r = (SDL_Rect){.w=2,.h=2, .x=0, .y=0};
  int row_no = 0;
  int width;
  uint8_t *glyph;

  Bound_t bounds[font.glyph_ct];

  for (int i = 0; i < font.glyph_ct; ++i) {
    if (!(i&7)) {
      r.x = 0;
      r.y = row_no++*glyph_height*2;
    }
    bounds[i] = (Bound_t){.l = r.x, .r = r.x+(font.glyph_widths[i]*2)};
    glyph = font.glyph_set + i*font.cell_size;
    width = font.glyph_widths[i];
    for (int j = 0; j < glyph_height; ++j) {
      for (int k = 0; k < width; ++k) {
        if (glyph[j]&(1<<k))
          SDL_SetRenderDrawColor(ren, 0,0,0,255);
        else
          SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderFillRect(ren, &r);
        r.x += 2;
      }
      r.x -= width*2;
      r.y += 2;
    }
    r.y -= glyph_height*2;
    r.x += width*2;
  }

  
  RowBounds *bounds_by_row = (RowBounds*)bounds;

  int curr_hl = -1, prev_hl = -1;

  


  
  if (is_verdana) {
    verdana_render_glyph_id(ren, &font, WIN_WIDTH, FONT_WIN_HEIGHT, curr_hl);
  } 

  SDL_RenderPresent(ren);

  SDL_Event ev;
  Bound_t cur_bound;
  _Bool quit = 0;
  int loc, mx = 0, my = 0;
  int gwidth;
  while (!quit) {
    while (SDL_PollEvent(&ev)) {
      if ((quit = (ev.type==SDL_QUIT)))
        break;
      if (ev.type == SDL_MOUSEMOTION) {
        SDL_GetMouseState(&mx, &my);
        if (mx < 0 || mx > WIN_WIDTH)
          continue;
        if (my < 0)
          continue;
        if (my > FONT_WIN_HEIGHT)
          loc = -1;
        else
          loc = FindGlyphUnderCursor(bounds_by_row, font.glyph_height*2, mx, my);
        if (curr_hl == loc)
          continue;
        prev_hl = curr_hl;
        curr_hl = 0 > loc ? loc : (8*(my/(font.glyph_height*2)))+loc;
        if (prev_hl >= 0) {
          cur_bound = bounds[prev_hl];
          r.x = cur_bound.l;
          r.y = (prev_hl/8)*font.glyph_height*2;
          glyph = font.glyph_set + font.cell_size*prev_hl;
          gwidth = font.glyph_widths[prev_hl];
          for (int i = 0; i < font.glyph_height; ++i) {
            for (int j = 0; j < gwidth; ++j) {
              if (glyph[i]&(1<<j))
                SDL_SetRenderDrawColor(ren, 0,0,0,255);
              else
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
              SDL_RenderFillRect(ren, &r);
              r.x += 2;
            }
            r.x -= gwidth*2;
            r.y += 2;
          }
        }

        if (is_verdana)
          verdana_render_glyph_id(ren, &font, WIN_WIDTH, FONT_WIN_HEIGHT, curr_hl);
        if (0 > curr_hl) {
          SDL_RenderPresent(ren);
          continue;
        }

        cur_bound = bounds[curr_hl];
        r.x = cur_bound.l;
        r.y = (curr_hl/8)*font.glyph_height*2;
        glyph = font.glyph_set + font.cell_size*curr_hl;
        gwidth = font.glyph_widths[curr_hl];
        for (int i = 0; i < font.glyph_height; ++i) {
          for (int j = 0; j < gwidth; ++j) {
            if (glyph[i]&(1<<j))
              SDL_SetRenderDrawColor(ren, 0,0,0,255);
            else
              SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);
            SDL_RenderFillRect(ren, &r);
            r.x += 2;
          }
          r.x -= gwidth*2;
          r.y += 2;
        }
        

        SDL_RenderPresent(ren);
      }
    }
  }
  int ret = 0;
  if (!Export_Font(&font, argv[2], outdir, lower_bound, upper_bound)) {
    ret = 1;
    perr("Font exportation failed.\n");
  }

  Font_Close(&font);
  Pal_BMP_Close(&bmp);

  SDL_DestroyRenderer(ren);

  SDL_DestroyWindow(win);

  SDL_Quit();


  return ret;
#endif
}

