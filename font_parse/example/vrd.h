#ifndef _VRD_H_
#define _VRD_H_

#ifdef __cplusplus
extern "C" {
#endif  /* CXX Name Mangler Guard */

#define vrd_CellWidth 8
#define vrd_CellHeight 16
#define vrd_GlyphHeight 12
#define vrd_GlyphCount 21

/* Uniform cell size (in bytes) for each glyph.
 * (Use as offset for glyph indexing): */
#define vrd_CellSize 16

/* Glyph pixel data. 1bpp. */
extern const unsigned short vrd_GlyphData[168];
extern const unsigned char vrd_GlyphWidths[21];
#ifdef __cplusplus
}
#endif  /* CXX Name Mangler Guard */

#endif  /* _VRD_H_ */

