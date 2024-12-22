/** Aw shit here we go again.
 * Copyright (C) Burton O Sumner
 * but you can use it if you want. :)
 * Just be sure to credit me if you're using a modified version of my c source.
 * If it's just the header and the lib file, then you dont have to. :)
 * */
#ifndef _BMP_PARSE_H_
#define _BMP_PARSE_H_



#ifdef __cplusplus
#include <cstdio>
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif  /* CXX name mangler guard */

typedef struct {
  uint8_t *pbuf;
  uint32_t width, height;
  uint16_t bpp;
} RGB_BMP_t;

typedef struct {
  uint8_t *pbuf;
  uint16_t bpp;
  uint32_t *pal, width, height, pallen;
} Pal_BMP_t;

#define INITIALIZE_BMP(bmp_type) ((bmp_type##_BMP_t) {0})


const char *BMP_Parse_Strerror(int);

int RGB_BMP_Parse(RGB_BMP_t* dest, const char *path);
int Pal_BMP_Parse(Pal_BMP_t* dest, const char *path);

void RGB_BMP_Close(RGB_BMP_t*);
void Pal_BMP_Close(Pal_BMP_t*);

#ifdef __cplusplus
}
#endif  /* CXX name mangler guard */


#endif  /* _BMP_PARSE_H_ */
