#include "bmp_parse.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <alloca.h>

#define DEREF_UINT(width, ptr) (*(uint##width##_t*) (ptr))
#define DEREF_INT(width, ptr) (*(int##width##_t*) (ptr))
#define BI_RGB 0

#define HEADER_IDENTIFIER 0x4D42
#define PBUF_FSEEK_DEST_OFS 10
#define DIB_HDR_LEN_OFS 14
#define WIDTH_OFS 18
#define HEIGHT_OFS 22
#define BPP_OFS 28
#define BI_COMPRESSION_METHOD_OFS 30
#define PAL_COLOR_COUNT_OFS 46

static const char *HEADER_ID = "BM";


enum ErrState {
  FILE_DNE=-7,
  MALFORMED_HEADER,
  UNSUPPORTED_FILE_TYPE,
  UNSUPPORTED_ENCODING,
  BUF_ROW_NOT_BYTE_ALIGNED,
  RGB_TO_PAL_ERR,
  PAL_TO_RGB_ERR,
  NO_ERROR
};

const char *BMP_Parse_Strerror(int error) {
  switch (error) {
  case FILE_DNE:
    return "Could not open file at given path.";
  case MALFORMED_HEADER:
    return "Header malformed. Please check the bitmap for potential "
      "corruption.";
  case UNSUPPORTED_FILE_TYPE:
    return "Header format identifier did not match identifier for supported "
      "BMP type. (I.e. first two bytes of header field must be: \"BM\")";
  case UNSUPPORTED_ENCODING:
    return "Decompression unsupported. Bitmap must not be compressed. "
      "(I.e BMP header field compression method must be (BI_RGB = 0))";
  case BUF_ROW_NOT_BYTE_ALIGNED:
    return "Buffer row must end on byte boundary. "
      "(I.e. the following must hold: width*bpp%8 == 0)";
  case RGB_TO_PAL_ERR:
    return "Conversion of RGB BMP to a paletted BMP struct unsupported.";
  case PAL_TO_RGB_ERR:
    return "Conversion of paletted BMP to an RGB BMP struct unsupported.";
  case NO_ERROR:
    return "No error occurred.";
  default:
    break;
  }

  return "Undefined ErrState given. Please pass value returned by function.";
}


typedef struct {
  size_t pal_ofs, pbuf_ofs, pbuf_len, bmprow_len, pbuf_row_len, bmprow_pad_len;
  int width, height;
  uint32_t pal_color_ct;
  uint16_t bpp;
} HeaderCtx;

static int ParseHeader(HeaderCtx *hdr, FILE *bmp) {
  fseek(bmp, 0, SEEK_END);
  int len = ftell(bmp);
  if (len < 18) {
    return MALFORMED_HEADER;
  }
  fseek(bmp, DIB_HDR_LEN_OFS, SEEK_SET);
  uint32_t hdrlen = 0;
  fread(&hdrlen, 4, 1, bmp);
  hdrlen += 14;
  fseek(bmp, 0, SEEK_SET);
  uint8_t *data = (uint8_t*)alloca(hdrlen*sizeof(uint8_t));
  fread(data, 1, hdrlen, bmp);

  if (DEREF_UINT(16, data) != DEREF_UINT(16, HEADER_ID))
    return UNSUPPORTED_FILE_TYPE;
  if (DEREF_UINT(32, data + BI_COMPRESSION_METHOD_OFS)!=BI_RGB)
    return UNSUPPORTED_ENCODING;
  // Get dimensions
  hdr->width = DEREF_INT(32, data + WIDTH_OFS);
  hdr->height = DEREF_INT(32, data + HEIGHT_OFS);
  // Get color depth
  hdr->bpp = 0xFF&DEREF_UINT(16, data + BPP_OFS);
  // Get color count.
  hdr->pal_color_ct = DEREF_UINT(32, data + PAL_COLOR_COUNT_OFS);
  hdr->pal_ofs = hdrlen;
  hdr->pbuf_ofs = DEREF_UINT(32, data + PBUF_FSEEK_DEST_OFS);
  hdr->bmprow_len = ((hdr->bpp*hdr->width + 31)>>5)<<2;
  hdr->bmprow_pad_len = 
    hdr->bmprow_len - (hdr->pbuf_row_len = ((hdr->bpp*hdr->width)>>3));
  hdr->pbuf_len = hdr->pbuf_row_len*hdr->height;
  return NO_ERROR;
}


int Pal_BMP_Parse(Pal_BMP_t *dest, const char *path) {
  FILE *bmp;
  int outcome;
  if (!(bmp = fopen(path, "r"))) {
    return FILE_DNE;
  }
  HeaderCtx hdr;
  if (0 > (outcome = ParseHeader(&hdr, bmp))) {
    fclose(bmp);
    return outcome;
  }


  if (!hdr.pal_color_ct) {
    fclose(bmp);
    return RGB_TO_PAL_ERR;
  }

  if ((hdr.bpp*hdr.width)&7) {
    fclose(bmp);
    return BUF_ROW_NOT_BYTE_ALIGNED;
  }
  dest->width = hdr.width;
  dest->height = hdr.height;
  dest->bpp = hdr.bpp;
  dest->pallen = hdr.pal_color_ct*4;
  fseek(bmp, hdr.pal_ofs, SEEK_SET);

  dest->pal = (uint32_t*)malloc(sizeof(uint32_t)*hdr.pal_color_ct);
  fread(dest->pal, 4, hdr.pal_color_ct, bmp);
  
  fseek(bmp, hdr.pbuf_ofs, SEEK_SET);
  dest->pbuf = (uint8_t*)malloc(sizeof(uint8_t)*hdr.pbuf_len);
  uint8_t *tmp = dest->pbuf;

  tmp = dest->pbuf + (hdr.height - 1)*hdr.pbuf_row_len;
  for (int i = 0; i < hdr.height; ++i) {
    fread(tmp, hdr.pbuf_row_len, 1, bmp);
    tmp -= hdr.pbuf_row_len;
    if (hdr.bmprow_pad_len)
      fseek(bmp, hdr.bmprow_pad_len, SEEK_CUR);
  }
  fclose(bmp);
  return NO_ERROR;
}

int RGB_BMP_Parse(RGB_BMP_t *dest, const char *path) {
  FILE* bmp;
  int outcome;
  if (!(bmp = fopen(path, "r"))) {
    return FILE_DNE;
  }
  HeaderCtx hdr;
  if (0 > (outcome = ParseHeader(&hdr, bmp))) {
    fclose(bmp);
    return outcome;
  }

  if (hdr.pal_color_ct) {
    fclose(bmp);
    return PAL_TO_RGB_ERR;
  }
  
  dest->width = hdr.width;
  dest->height = hdr.height;
  dest->bpp = hdr.bpp;
  fseek(bmp, hdr.pbuf_ofs, SEEK_SET);
  dest->pbuf = (uint8_t*)malloc(sizeof(uint8_t)*hdr.pbuf_len);
  uint8_t *tmp = dest->pbuf + (hdr.height-1)*hdr.pbuf_row_len;
  for (int i = 0; i < hdr.height; ++i) {
    fread(tmp, hdr.pbuf_row_len, 1, bmp);
    tmp -= hdr.pbuf_row_len;
    if (hdr.bmprow_pad_len)
      fseek(bmp, hdr.bmprow_pad_len, SEEK_CUR);
  }
  fclose(bmp);
  return outcome;
}


void RGB_BMP_Close(RGB_BMP_t* bmp) {
  if (!(bmp->pbuf))
    return;
  free((void*)(bmp->pbuf));
  memset(bmp, 0, sizeof(RGB_BMP_t));
}

void Pal_BMP_Close(Pal_BMP_t* bmp) {
  if (!(bmp->pbuf))
    return;
  free((void*)(bmp->pbuf));
  free((void*)(bmp->pal));
  memset(bmp, 0, sizeof(Pal_BMP_t));
}

