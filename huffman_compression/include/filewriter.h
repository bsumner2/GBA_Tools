#ifndef _FILEWRITER_H_
#define _FILEWRITER_H_

#include "huffman.h"
#include <stdio.h>
void write_c_src_file(FILE *fp, const char *exename, const char *infile, const char *output_objname, size_t uncompressed_data_size, uint32_t *compdata, uint32_t comp_word_ct, HuffHeader_GBA_t gba_header, HuffTree_t *hufftree, HuffNode_GBA_t *gba_hufftree, uint32_t gba_table_len, DataSize_e huffcode_bitdepth);
void write_asm_src_file(FILE *fp, const char *exename, const char *infile, const char *output_objname, size_t uncompressed_data_size, uint32_t *compdata, uint32_t comp_word_ct, HuffHeader_GBA_t gba_header, HuffTree_t *hufftree, HuffNode_GBA_t *gba_hufftree, uint32_t gba_table_len, DataSize_e huffcode_bitdepth);

void write_header_file(FILE *fp, const char *exename, const char *infile, const char *outfile_name, const char *output_objname, size_t uncompressed_data_size, uint32_t comp_word_ct, uint32_t node_ct, uint32_t gba_table_len, DataSize_e huffcode_bitdepth, _Bool src_is_asm);

#endif  /* _FILEWRITER_H_ */
