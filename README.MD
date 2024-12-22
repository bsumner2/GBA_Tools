# GBA Development Tools

- A bitmap parsing library for extracting image data for a bitmap
- A font glyphset creation tool that uses the bmpparse library to parse a bmp with the glyphset data on it outputs C source files with the font data
- A Huffman Compression tool that takes an input file (text or raw bin data) and uses huffman compression to compress it and outputs C or ASM source files with the compressed data and header that you pass to the GBA's BIOS SVC, using SVC 0x13 (SVC 0x00130000 if not in THUMB mode)

