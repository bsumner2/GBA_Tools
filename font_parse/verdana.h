#ifndef _VERDANA_H_
#define _VERDANA_H_

#ifdef __cplusplus
extern "C" {
#endif  /* CXX Name Mangler Guard */

#define verdana_CellWidth 8
#define verdana_CellHeight 16
#define verdana_GlyphHeight 12
#define verdana_GlyphCount 224

/* Uniform cell size (in bytes) for each glyph.
 * (Use as offset for glyph indexing): */
#define verdana_CellSize 16

/* Glyph pixel data. 1bpp. */
extern const unsigned short verdana_GlyphData[1792];
extern const unsigned char verdana_GlyphWidths[224];
#ifdef __cplusplus
}
#endif  /* CXX Name Mangler Guard */

#endif  /* _VERDANA_H_ */

