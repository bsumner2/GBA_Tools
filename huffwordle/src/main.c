#include "huffman.h"
#include "int_ll.h"
#include "filewriter.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define ERR_PREFIX "\x1b[1;31m[Error]:\x1b[0m "
#define perrf(fmt, ...) fprintf(stderr, ERR_PREFIX fmt, __VA_ARGS__)
#define perr(s) fputs(ERR_PREFIX s, stderr)

#define COLOR(clr, text) "\x1b[" #clr "m" text "\x1b[39m"
#define BOLD(text) "\x1b[1m" text "\x1b[22m"
#define COLOR_BOLD(clr, text) "\x1b[1;" #clr "m" text "\x1b[22;39m"
#if 0
#include "huff_codebase.h"
int main(void) {
  char data[] = "There once was a man from Peru! Who dreamed he was eating his "
    "shoe!~ He woke with a fright in the middle of the night to find that his dream came true!";
  HuffTree_t *tree = Huff_Tree_Create(data, sizeof(data)/4, E_DATA_UNITS_4_BITS);
  if (!tree) {
    perrf("Failed to create hufftree. \x1b[1;34mDetails:\x1b[39m %s\n",
        Huff_Strerror());
    return -1;
  }
  CodeBase_t *codebase = Huff_Codebase_Create(tree);
  if (!codebase) {
    perrf("Codebase came back NULL.\nDetails: %s\n", Huff_Codebase_Strerror());
    return -1;
  }
  int cbent_ct;
  const BST_Node_t *cbroot = BST_Get_Root(codebase, &cbent_ct);
  if (cbent_ct != tree->leaf_ct) {
    perrf("Codebase has less entries than expected.\n"
        "Expected \x1b[1;34m%d\x1b[0m, got \x1b[1;34m%d.\x1b[0m\n", tree->leaf_ct,
        cbent_ct);
    return -1;
  }
  const BST_Node_t *stack[1<<(cbroot->height+1)], *cur;
  const CodeEntry_t *codent;
  uint64_t code;
  uint32_t len;
  int top = -1;
  stack[++top] = cbroot;
  do {
    cur = stack[top--];
    if (!cur)
      continue;
    codent = cur->data;
    printf("\x1b[1;34mCode Entry For '%c' (ASCII %d):\x1b[33m\n\t", codent->data, codent->data);
    code = codent->code;
    len = codent->codelen;
    for (int i = len-1; i > -1; --i) {
      if (code&(1<<i))
        putchar('1');
      else
        putchar('0');
    }
    puts("\x1b[0m");
    stack[++top] = cur->l;
    stack[++top] = cur->r;
  } while (top > -1);
  Huff_Codebase_Destroy(codebase);

  uint32_t *compdata;
  int complen = 0;
  compdata = Huff_Compress(data, tree, sizeof(data)/4, &complen);
  if (!compdata) {
    perrf("Failed to compress.\n\t\x1b[1;33mDetails: \x1b[2;31m%s\x1b[0m\n",
        Huff_Strerror());
    return -1;
  }
  uint32_t curr_word = 1;
  printf("\x1b[1;34mCompressed Data:\x1b[0m\n");
  int currlinelen=0;
  for (int i = 0; i < complen; ++i) {
    curr_word = compdata[i];
    for (int j = 31; j > -1; --j) {
      if (!(currlinelen++&63)) {
        putchar(10);
        currlinelen = 1;
      }
      if (curr_word&(1<<j)) {
        putchar('1');
      } else {
        putchar('0');
      }
    }
  }
  putchar(10);
  printf("Data len: \x1b[1;34m%d DWORDS\x1b[39m\n\t"
      "= \x1b[34m%d Bytes\x1b[39m\n\t"
      "= \x1b[34m%d bits\x1b[0m\n", complen, complen<<2, complen<<2<<3);

  free(compdata);
  Huff_Tree_Destroy(tree);

  return 0;
}
#else



#define WARN_PREFIX "\x1b[1;33m[Warning]:\x1b[0m "
#define warnf(fmt, ...) fprintf(stderr, WARN_PREFIX fmt, __VA_ARGS__)
#define warn(s) fputs(WARN_PREFIX s, stderr)

#define OUTFILE_EXTENSION_SUBSTRLEN 2


char *strdupe(const char *str) {
  char *ret;
  int len = strlen(str);
  ret = malloc(sizeof(char)*(len+1));
  ret[len] = '\0';
  for (char *tmp = ret; *str; *tmp++=*str++)
    continue;
  return ret;
}

#define strcatdupe(...) strcatdupe_variadic_def(__VA_ARGS__, NULL)

char *strcatdupe_variadic_def(const char *first, ...) {
  Int_LL_t lens = NEW_INT_LL();
  va_list args;
  const char *cur = first;
  char *csr = NULL, *ret = NULL;
  int argc = 0, len = 0, curlen = 0;
  
  if (first == NULL) {
    return NULL;
  }
  
  va_start(args, first);
  do { 
    len += (curlen = strlen(cur));
    Int_LL_Enqueue(&lens, curlen);
    ++argc;
    cur = va_arg(args, const char*);
  } while (NULL != cur);
  va_end(args);
  
  ret = malloc(sizeof(char)*(len+1));

  if (ret == NULL) {
    Int_LL_Close(&lens);
    return NULL;
  }


  if (argc==1) {
    curlen = lens.head->data;
    assert(curlen == len);
    Int_LL_Close(&lens);
    strncpy(ret, first, curlen);
    ret[curlen] = '\0';
    return ret;
  }

  va_start(args, first);
  for (csr = ret, cur = first; NULL != cur; cur = va_arg(args, const char*)) {
    curlen = Int_LL_Dequeue(&lens);
    strncpy(csr, cur, curlen);
    csr += curlen;
  }

  *csr = '\0';
  assert(lens.nmemb == 0UL);
  assert(((uintptr_t)csr - (uintptr_t)ret) == (unsigned long)len);
  Int_LL_Close(&lens);

  return ret;
}

void print_usage(const char *exename, FILE *ostream) {
  fprintf(ostream,
      "\t\x1b[1;33m[Usage]: \x1b[32m%s "
      "\x1b[34m<input data file path> \x1b[36m[OPTIONS]\n\t"
      "\x1b[1;33m[For help menu]: \x1b[32m%s (-h|--help)\n\t\t"
      "\x1b[33m[Options]:\n\t\t\t"
      "\x1b[1;39m-o \x1b[36m<output file base name> \x1b[0m(Defaults to input file base name)\n\t\t\t"
      "\x1b[1;39m-b \x1b[36m<bits per huffcode (4|8)> \x1b[0m(Defaults to 8)\n\t\t\t"
      "\x1b[1;39m-n \x1b[36m<output src object base name> \x1b[0m(Defaults to output file base name)\n\t\t\t"
      "\x1b[1;39m-t \x1b[36m<output src type (c|C|asm|ASM)> \x1b[0m (Defaults to C source file as output src type)\n\t\t\t"
      "\x1b[1;39m-d \x1b[36m<output directory> \x1b[0m (Defaults to ./)\n\t\t\t"
      "\x1b[1;39m--no-include\x1b[22m \x1b[2mTells program not to generate accompanying C header file if and only if output src type is Assembly\x1b[0m (Generates accompanying C header file by default)\n", 
      exename,
      exename);
}

enum e_cli_opt {
  OUTFILE_NAME='o',
  HUFFCODE_BITDEPTH='b',
  SYMBOL_NAME='n',
  OUTFILE_TYPE='t',
  OUTPUT_DIRECTORY='d',
  HELP_MENU='h'
};

/**
 * @brief Given user-provided symname, validate and fix symbol name when needed.
 * if anything appended or removed, input string freed and new return string 
 * malloced; otherwise, returned string will be input string.
 * @param symname Symbol name (MUST BE A MALLOC'd STRING)
 * @param len strlen(symname)
 * */
char *make_valid_symbolname(char *symname, int len) {
  char *ret = symname, c = *symname;
  if (c != '_' && !isalpha(c)) {
    if (!isdigit(c)) {
      // if isn't a char that is only valid after initial char of symbol 
      // (i.e.: a numeric digit), then just replace initial char with _ and be 
      // done with it.
      *symname = '_';
    } else {
      // otherwise, prepend an underscore instead of replacing initial with one
      ret = malloc(sizeof(char)*(++len+1));
      *ret = '_';
      strncpy(ret+1, symname, len-1);
      ret[len] = '\0';
      free(symname);
      symname = ret+2;  // since prefix, _<digit> is validated already, 
                        // can offset check cursor by +2
    }
  }

  for (c = *symname ; c ; c = *++symname) {
    if ((c == '_') || ((c-'0') < 10))
      continue;
    if (isalpha(c))
      continue;
    *symname = '_';
  }
  return ret;
}

#define OPT_TO_CHECKLIST_IDX(opt) (opt-'a')

int parse_opts(const int argc, const char *argv[], char **outfile, 
    char **outobjname, char **output_dir, char *type, DataSize_e *data_size,_Bool *generate_include) {
  int *lens = malloc(sizeof(int)*argc), i;
  _Bool opts_parsed['t'-'a'+1];
  memset(opts_parsed, 0, sizeof(opts_parsed));
  lens[0] = -1;  // dont care about len of argv[0]
  {
    const char *tmp;
    for (i = 1; i < argc; ++i) {
      lens[i] = strlen(argv[i]);
      if (*(uint16_t*)argv[i] == ((uint16_t)('-'<<8)|'-')) {
        tmp = &argv[i][2];
        if (i!=1 && !strcmp("help", tmp)) {
          perrf("Invalid opt args. Cannot just hamfist help opt flag in middle of opts.\n"
              "To access help menu, simply run:\n\t"
              "\x1b[1;34m%s \x1b[39m-h\x1b[22m or \x1b[1;34m%s \x1b[39m--help\x1b[0m\n",
              *argv, *argv);
          free(lens);
          return -1;
        } else if (!strcmp("no-include", tmp)) {
          *generate_include = false;
          continue;
        }
        if (i==1) {
          perrf("Invalid input file name arg. Input file name, " BOLD("%s") ", cannot contain prefix, " BOLD("--")
              ", which is reserved for option flags.\n"
              "If this is the name of the file, please rename it and retry command\n"
              "To access help menu, simply run:\n\t"
              "\x1b[1;34m%s \x1b[39m-h\x1b[22m or \x1b[1;34m%s \x1b[39m--help\x1b[0m\n",
        argv[1], *argv, *argv);
          free(lens);
          return -1;
        } else {
          if (i!=1 && argv[i-1][0] == '-') {
            char *argtype, flagdenoter = argv[i-1][1];
            if (flagdenoter == 'o' || flagdenoter == 'n') {
              argtype = flagdenoter == 'o' ? "file basename" : "symbol names prefix";
              perrf("Invalid output %s opt arg. Output %s, " BOLD("%s") ", cannot be contain prefix, " BOLD("--")
                  ", which is reserved for option flags.\n"
                  "Please rename it and retry the command.\n"
                  "To access help menu, simply run:\n\t"
                  "\x1b[1;34m%s \x1b[39m-h\x1b[22m or \x1b[1;34m%s \x1b[39m--help\x1b[0m\n",
                  argtype, argtype, argv[i], *argv, *argv);

            }

          }
          perrf("Invalid opt args. " BOLD("%s") "is not a valid option flag\n",
              argv[i]);
          free(lens);
          return -1;
        }
      }
    }
  }

  if (lens[1] == 2 || lens[1] == 6) {
    if (!strcmp(argv[1], "--help") || (*(uint16_t*)argv[1] == *(uint16_t*)"-h")) {
      printf("\x1b[1;34m%s\x1b[39m Written by Burt Sumner 2024\n\x1b[22m", *argv);
      print_usage(*argv, stdout);
      puts(COLOR_BOLD(34, "\n[NOTE]: ") "If linker is giving you issues about "
          "fitting your compressed data when compiling and linking your GBA Project,\n"
          "Try rerunning this with the option args, " COLOR_BOLD(33, "-t ASM") ", as this has happened to me:\n" 
          BOLD("linker complains about same array declared in C, but links no problem with same array declared in ASM...\n")
          "Good luck and happy coding!");
      free(lens);
      exit(0);
    }
  }
  char *in_basename = NULL;
  uintptr_t in_basename_len = 0;
  {
    const char *ifname = argv[1], *ifname_start = ifname+lens[1], *ifname_end = NULL;
    char c;
    for (c = *--ifname_start; ifname_start!=ifname && c!='/'; c = *--ifname_start) {
      if (NULL != ifname_end || '.' != c)
        continue;
      ifname_end = ifname_start;
    }
    if (NULL == ifname_end)
      ifname_end = ifname + lens[1];

    if ('/' == c) {
      ++ifname_start;
    }
    in_basename_len = (uintptr_t)ifname_end - (uintptr_t)ifname_start;
    in_basename = malloc(sizeof(char)*(in_basename_len+1));
    strncpy(in_basename, ifname_start, in_basename_len);
    in_basename[in_basename_len] = '\0';
  }
  if (argc==2) {
    char *tmp;
    int tmplen = in_basename_len+OUTFILE_EXTENSION_SUBSTRLEN;
    tmp = *outfile = calloc(tmplen+1, sizeof(char));
    snprintf(tmp, tmplen+1, "%s.c", in_basename);
    tmp[tmplen] = '\0';
    *outobjname = in_basename;
    *data_size = E_DATA_UNIT_8_BITS;
    *type = 'c';
    free(lens);
    return 0;
  }

  if (argc&1 && *generate_include) {
    perrf("Invalid argument count (%d). Expect even arg count unless:\n\t"
        "\x1b[1;34m - \x1b[0mSpecifying not to generate a companion C-style header file with \x1b[1m--no-include\x1b[0m.\n\t\t"
        "(Program generates a header by default)\n\t"
        "\x1b[1;34m - \x1b[0mRunning program with help arg, as shown below\n\t\t"
        "\x1b[1m$ %s -h\x1b[22m or \x1b[1m $ %s --help\x1b[22m to see usage.\n", argc,
        *argv, *argv);
    free(lens);
    free(in_basename);
    return -1;
  }

  {
    const char *cur;
    *type = 'c';
    *data_size = E_DATA_UNIT_8_BITS;
    char *ofname = NULL, *symname = NULL;
    i = 2;
    while (i+1 < argc) {
      cur = argv[i];
      if (cur[0] != '-' || lens[i] != 2) {
        if (!strcmp("--no-include", cur)) {
          if (++i + 1 == argc) {
            perrf("Invalid opt args. Final arg, " BOLD("%s") ", is an opt flag"
                "without an accompanying opt parameter.\n", argv[i]);
          } else {
            continue;
          }
        } else {
          perrf("Invalid opt args. Expected flag argument, received %s\n", cur);
        }
        free(lens);
        free(in_basename);
        return -1;
      }

      if (argv[i+1][0]=='-') {
        if (argv[i+1][1] == 'h' || cur[1] == 'h') {
          perrf("Invalid opt args. Cannot just hamfist help opt flag in middle of opts.\n"
              "To access help menu, simply run:\n\t"
              "\x1b[1;34m%s \x1b[39m-h\x1b[22m or \x1b[1;34m%s \x1b[39m--help\x1b[0m\n",
              *argv, *argv);
        } else {
          perrf("Invalid opt args. Two flags back to back: \x1b[1m%s %s\x1b[0m\n", cur, argv[i+1]);
        }
        goto ERR_HANDLE;
      }

      switch((enum e_cli_opt)cur[1]) {
        case OUTFILE_NAME:
          if (opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])]) {
            warnf("Args for opt, " COLOR_BOLD(34, "-%c") ", have already been "
                "Parsed. Ignoring duplicate args, " COLOR_BOLD(31, "-%c %s") "\n", 
                cur[1], cur[1], argv[++i]);
          } else {
            opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])] = true;
            ofname = strdupe(argv[++i]);
          }
          ++i;
          continue;
        case HUFFCODE_BITDEPTH:
          if (opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])]) {
            warnf("Args for opt, " COLOR_BOLD(34, "-%c") ", have already been "
                "Parsed. Ignoring duplicate args, " COLOR_BOLD(31, "-%c %s") "\n", 
                cur[1], cur[1], argv[++i]);
            ++i;
            continue;
          }
          opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])] = true;

          cur = argv[++i];
          if (lens[i] != 1) {
            perrf("Invalid opt args. \x1b[1m%s\x1b[22m is not a param for opt flag, \x1b[1m%c\x1b[22m\n", cur, HUFFCODE_BITDEPTH);
            free(lens);
            free(in_basename);
            break;
          }
          {
            DataSize_e tmp = cur[0] - '0';
            if (((tmp & E_DATA_UNIT_VALIDITY_MASK) != tmp) || (tmp == E_DATA_UNIT_VALIDITY_MASK)) {
              perrf("Invalid opt args. \x1b[1m%s\x1b[22m is not a param for opt flag, \x1b[1m%c\x1b[22m\n", cur, HUFFCODE_BITDEPTH);
              free(lens);
              free(in_basename);
              break;
            }
            *data_size = tmp;
          }
          ++i;
          continue;
        case SYMBOL_NAME:
          if (opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])]) {
            warnf("Args for opt, " COLOR_BOLD(34, "-%c") ", have already been "
                "Parsed. Ignoring duplicate args, " COLOR_BOLD(31, "-%c %s") "\n", 
                cur[1], cur[1], argv[++i]);
            ++i;
            continue;
          }
          opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])] = true;

          cur = argv[++i];
          symname = strdupe(cur);
          symname = make_valid_symbolname(symname, lens[i]);
          ++i;
          continue;
        case OUTFILE_TYPE:
          if (opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])]) {
            warnf("Args for opt, " COLOR_BOLD(34, "-%c") ", have already been "
                "Parsed. Ignoring duplicate args, " COLOR_BOLD(31, "-%c %s") "\n", 
                cur[1], cur[1], argv[++i]);
            ++i;
            continue;
          }
          opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])] = true;

          cur = strdupe(argv[++i]);
          if (lens[i] == 1) {
            char c = *cur;
            free((void*)cur);
            if (c!='c' && c!='C') {
              perrf("Invalid opt args. \x1b[1m%s\x1b[22m is an invalid param "
                  "for opt flag, \x1b[1m-%c\n"
                  "Valid params for output src type flag, \x1b[34m-%c\x1b[22m:\n\t"
                  "\x1b[32mc\x1b[39m, \x1b[32mC\x1b[39m, \x1b[32masm\x1b[39m, \x1b[32mASM\x1b[0m\n",
                  argv[i], OUTFILE_TYPE, OUTFILE_TYPE);
              free((void*)cur);
              break;
            }
            *type = 'c';
            ++i;
            continue;
          }
          for (char *csr = (char*)cur, c = *csr; c; c = *++csr) {
            if (!isalpha(c)) {
              free((void*)cur);
              perrf("Invalid opt args. \x1b[1m%s\x1b[22m is an invalid param "
                  "for opt flag, \x1b[1m-%c\n"
                  "Valid params for output src type flag, \x1b[34m-%c\x1b[22m:\n\t"
                  "\x1b[32mc\x1b[39m, \x1b[32mC\x1b[39m, \x1b[32masm\x1b[39m, \x1b[32mASM\x1b[0m\n",
                  argv[i], OUTFILE_TYPE, OUTFILE_TYPE);
        goto ERR_HANDLE;
            }
            *csr = tolower(c);
          }
          if (strcmp(cur, "asm")) {
              perrf("Invalid opt args. \x1b[1m%s\x1b[22m is an invalid param "
                  "for opt flag, \x1b[1m-%c\n"
                  "Valid params for output src type flag, \x1b[34m-%c\x1b[22m:\n\t"
                  "\x1b[32mc\x1b[39m, \x1b[32mC\x1b[39m, \x1b[32masm\x1b[39m, \x1b[32mASM\x1b[0m\n"
                  , argv[i], OUTFILE_TYPE, OUTFILE_TYPE);
              free((void*)cur);
              break;
          }
          free((void*)cur);
          *type = 's';
          ++i;
          continue;
        case HELP_MENU:
          perrf("Invalid opt args. Cannot just hamfist help opt flag in middle of opts.\n"
              "To access help menu, simply run:\n\t"
              "\x1b[1;34m%s \x1b[39m-h\x1b[22m or \x1b[1;34m%s \x1b[39m--help\x1b[0m\n",
              *argv, *argv);
          break;
        case OUTPUT_DIRECTORY:
          if (opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])]) {
            warnf("Args for opt, " COLOR_BOLD(34, "-%c") ", have already been "
                "Parsed. Ignoring duplicate args, " COLOR_BOLD(31, "-%c %s") "\n", 
                cur[1], cur[1], argv[++i]);
            ++i;
            continue;
          }
          opts_parsed[OPT_TO_CHECKLIST_IDX(cur[1])] = true;
          
          cur = argv[++i];
          if (cur[lens[i-1]] != '/') {
            *output_dir = strcatdupe(cur, "/");
          } else {
            *output_dir = strdupe(cur);
          }
          ++i;
          continue;
        default:
          perrf("Invalid opt args. \x1b[1m%s\x1b[22m is not a valid arg.\n", cur);
          break;
      }
ERR_HANDLE:
      free(lens);
      free(in_basename);
      if (ofname!=NULL)
        free(ofname);
      if (symname!=NULL)
        free(symname);
      return -1;
    }
    if (ofname == NULL) {
      ofname = in_basename;
    }
    if (symname == NULL) {
      symname = make_valid_symbolname(strdupe(ofname), strlen(ofname));
    }

    char ext[3] = { '.', *type, '\0' };
    *outfile = strcatdupe(ofname, ext);
    
    if (ofname == in_basename) {
      free(ofname);
      ofname = NULL;
      in_basename = NULL;
    } else {
      free(ofname);
      ofname = NULL;
    }
    *outobjname = symname;
  }
  
  if (in_basename != NULL) {
    free(in_basename);
  }
  free(lens);

  return 0;
}



int main(int argc, char *argv[]) {
  if (argc < 2) {
    perr("Invalid argument count. See below for usage:\n");
    print_usage(*argv, stderr);
    return 1;
  }


  char *outfile = NULL, *infile = argv[1], *output_objname = NULL, 
       *output_dir = NULL, type = '\0';
  int ofnamelen, oonamelen, odnamelen;
  DataSize_e huffcode_bitdepth;
  _Bool generate_include = true;

  if (0 > parse_opts(argc, (const char**)argv, &outfile, &output_objname, &output_dir, &type, &huffcode_bitdepth, &generate_include)) {
    perr("Failed to parse opts.\n");
    
    if (outfile!=NULL)
      free(outfile);

    if (output_objname!=NULL)
      free(output_objname);

    return -1;
  }

  // I hate having to do this, but this is the easiest way to adapt the below
  // code I've recycled for this GBA Huffman compressor CLI tool.
  ofnamelen = strlen(outfile);
  char of_static[ofnamelen+1];
  strncpy(of_static, outfile, ofnamelen);
  of_static[ofnamelen] = '\0';
  free(outfile);
  outfile = of_static;

  oonamelen = strlen(output_objname);
  char oo_static[oonamelen+1];
  strncpy(oo_static, output_objname, oonamelen);
  oo_static[oonamelen] = '\0';
  free(output_objname);
  output_objname = oo_static;

  if (output_dir == NULL) {
    output_dir = strdupe("./");
  }
  odnamelen = strlen(output_dir);
  char od_static[odnamelen+1];
  strncpy(od_static, output_dir, odnamelen);
  od_static[odnamelen] = '\0';
  free(output_dir);
  output_dir = od_static;


  if (!generate_include && type != 's') {
    warn(COLOR_BOLD(34, "--no-include") " specified, but output source type is " BOLD("C\n")
        COLOR_BOLD(34, "--no-include") " can only be specified if source type is " BOLD("Assembly\n"));
  }
  printf("Compressing " BOLD("%s") " and formatting to GBA BIOS-provided Huffman Decompression SVC:\n"
      COLOR_BOLD(34, "Output file:") "\t\t\t" BOLD("%s\n")
      COLOR_BOLD(34, "Output Directory:") "\t\t" BOLD("%s\n")
      COLOR_BOLD(34, "Output Symbol Prefix:") "\t\t" BOLD("%s\n")
      COLOR_BOLD(34, "Output Source Code Type:") "\t" BOLD("%s\n")
      COLOR_BOLD(34, "Huffcode Bitdepth:") "\t\t" BOLD("%d\n")
      COLOR_BOLD(34, "Generate C Header File:") "\t\t" BOLD("%s\n"), infile,
      outfile, output_dir, output_objname, 
      type?(type=='c'?"C":(type=='s'?"ASM":"[N/A]")):"[ERROR]", huffcode_bitdepth, 
      type == 'c'
            ? (generate_include
                ? COLOR(34, "True")
                : COLOR(34, "True") 
                    "\x1b[0m \x1b[2m(--no-include only valid option when \x1b[0m"
                    "\x1b[1;34m-t asm\x1b[0m\x1b[2m or \x1b[0m\x1b[1;34m-t ASM\x1b[0m"
                    "\x1b[2m is also specified)\x1b[0m")
            : (generate_include?COLOR(34, "True"):COLOR(31, "False")));
  if (!generate_include && type != 's') {
    generate_include = true;
  }
#ifdef _TESTING_ARG_PARSER_
  return 0;
#endif
  void *data = NULL;
  size_t data_size = 0UL;
  {
    FILE *fp = fopen(infile, "r");
    size_t data_size_orig;
    long len;
    if (!fp) {
      int errno_save = errno;
      perrf("Failed to open input file, " COLOR_BOLD(32, "%s") ", for reading.",
          infile);
      if (errno_save != 0) {
        fprintf(stderr, "\n\t" COLOR_BOLD(31, "[Details]: ") "%s\n", 
            strerror(errno_save));
      } else {
        fputc('\n', stderr);
      }
      return -1;
    }

    fseek(fp, 0L, SEEK_END);
    len = ftell(fp);
    if (0UL > len) {
      perrf("Failed to get size of input file, " COLOR_BOLD(32, "%s") ".\n"
          COLOR_BOLD(31, "[Size received]:") " %ld", infile, ftell(fp));
      fclose(fp);
      return -1;
    }
    data_size_orig = data_size = (size_t)len;

    // Make sure data is word-aligned size
    if (data_size&3) {
      data_size &= (~3ULL);
      data_size+=4;
    }
    assert(!(data_size&3));

    data = malloc(data_size);
    if (data==NULL) {
      int errno_save = errno;
      perrf("Failed to allocate buffer for input file, " COLOR_BOLD(32, "%s") 
          ", data to compress.\n" COLOR_BOLD(31, "[Details]: ") "%s\n",
          infile, strerror(errno_save));
      fclose(fp);
      return -1;
    }

    fseek(fp, 0L, SEEK_SET);
    {
      size_t readlen = fread(data, 1, data_size, fp);
      int errno_save = errno;
      if (data_size_orig != readlen) {
        perrf("Failed to read all " COLOR_BOLD(33, "%lu bytes") " from input "
            "file, " COLOR_BOLD(32, "%s") ".\nOnly read " 
            COLOR_BOLD(31, "%lu bytes\n"), data_size_orig, infile, readlen);
        if (errno_save!=0) {
          fprintf(stderr, COLOR_BOLD(31, "[Details]: ") "%s\n", 
              strerror(errno_save));
        }
        fclose(fp);
        free((void*)data);
        return -1;
      }
    }
    fclose(fp);
  }
  size_t data_word_ct = data_size/4;
  HuffTree_t *tree = Huff_Tree_Create(data, data_word_ct, huffcode_bitdepth);
  uint32_t *compdata = NULL;
  HuffNode_GBA_t *gba_treetable = NULL;
  HuffHeader_GBA_t gba_hdr = {0};
  int complen = 0, tablelen=0;
  FILE *ofp = NULL;
  if (!tree) {
    perrf("Failed to create hufftree. \x1b[1;34mDetails:\x1b[39m %s\x1b[0m\n",
        Huff_Strerror());
    return -1;
  }
  compdata = Huff_Compress(data, tree, data_word_ct, &complen);
  if (!compdata) {
    perrf("Failed to compress.\n\t\x1b[1;33mDetails: \x1b[2;31m%s\x1b[0m\n",
        Huff_Strerror());
    Huff_Tree_Destroy(tree);
    return -1;
  }

  if (0 > Huff_GBA_Header_Init(&gba_hdr, data_size, huffcode_bitdepth)) {
    perrf("Failed to create GBA Header.\n\t\x1b[1;34mDetails: \x1b[39m"
        "%s\x1b[0m\n", Huff_Strerror());
    Huff_Tree_Destroy(tree);
    free(compdata);
    return -1;
  }

  gba_treetable = Huff_GBA_Huff_Table_Create(tree, &tablelen);
  if (!gba_treetable) {
    perrf("Failed to create GBA HuffTree table.\n\t\x1b[1;34mDetails: \x1b[39m"
        "%s\x1b[0m\n", Huff_Strerror());
    Huff_Tree_Destroy(tree);
    free(compdata);
    return -1;
  }

  char full_out_path[ofnamelen+odnamelen+1];
  snprintf(full_out_path, sizeof(full_out_path), "%s%s", output_dir, outfile);

  assert(full_out_path[sizeof(full_out_path)-1] == '\0');

  if (NULL == (ofp = fopen(full_out_path, "w"))) {
    int errnosave = errno;
    if (errnosave != 0) {
      perrf("Failed to open output src file, " BOLD("%s\n\t")
          COLOR_BOLD(31, "Details: ") "%s\n", full_out_path, strerror(errnosave));
    } else {
      perrf("Failed to open output src file, " BOLD("%s\n\t"), full_out_path);
    }
    Huff_Tree_Destroy(tree);
    free(compdata);
    free(gba_treetable);
    return -1;
  }
  
  const char *infile_truncated = argv[1] + strlen(argv[1]);
  {
    const char *begin = argv[1];
    do if (*--infile_truncated == '/') {
      ++infile_truncated;
      break;
    } while (infile_truncated != begin);

  }

  if (type == 'c') {
    write_c_src_file(ofp, argv[0], infile_truncated, output_objname, data_size, compdata, complen, gba_hdr, tree, gba_treetable, tablelen, huffcode_bitdepth);
    fclose(ofp);
    full_out_path[sizeof(full_out_path)-2] = 'h';
    if (NULL == (ofp = fopen(full_out_path, "w"))) {
      int errnosave = errno;
      if (errnosave != 0) {
        perrf("Failed to open output header file, " BOLD("%s\n\t")
            COLOR_BOLD(31, "Details: ") "%s\n", full_out_path, strerror(errnosave));
      } else {
        perrf("Failed to open output header file, " BOLD("%s\n\t"), full_out_path);
      }
      Huff_Tree_Destroy(tree);
      free(compdata);
      free(gba_treetable);
      return -1;
    }
    write_header_file(ofp, argv[0], infile_truncated, outfile, output_objname, data_size, complen, tree->node_ct, tablelen, huffcode_bitdepth, false);
  } else {
    do {
      write_asm_src_file(ofp, argv[0], infile_truncated, output_objname, data_size, compdata, complen, gba_hdr, tree, gba_treetable, tablelen, huffcode_bitdepth);
      fclose(ofp);

      if (!generate_include)
        break;
      full_out_path[sizeof(full_out_path)-2] = 'h';
      if (NULL == (ofp = fopen(full_out_path, "w"))) {
        int errnosave = errno;
        if (errnosave != 0) {
          perrf("Failed to open output header file, " BOLD("%s\n\t")
              COLOR_BOLD(31, "Details: ") "%s\n", full_out_path, strerror(errnosave));
        } else {
          perrf("Failed to open output header file, " BOLD("%s\n\t"), full_out_path);
        }
        Huff_Tree_Destroy(tree);
        free(compdata);
        free(gba_treetable);
        return -1;
      }
      write_header_file(ofp, argv[0], infile_truncated, outfile, output_objname, data_size, complen, tree->node_ct, tablelen, huffcode_bitdepth, true);
    } while (0);  // "do ... while (0); is the `Poor Man's` try catch," a wise man once said!
  }

  /*
  if (!ofp) {
    perrf("Failed to open, \x1b[1m\"%s\"\x1b[0m for outwriting.\n\t\x1b[1;34m"
        "Details:\x1b[39m %s\x1b[0m\n", outfile, strerror(errno));
    Huff_Tree_Destroy(tree);
    free(compdata);
    free(gba_treetable);
  }

  
  

  fwrite(&gba_hdr, sizeof(gba_hdr), 1, ofp);
  fwrite(gba_treetable, sizeof(*gba_treetable), tablelen, ofp);
  if (ftell(ofp)&3ULL) {
    warnf("Failed to align compressed data stream to word boundary. :(\n"
        "Compressed data stream's offset within output file is \x1b[1;31m%ld"
        "\x1b[0m,\nWhich \x1b[31mdoes not\x1b[0m fall on a word boundary.\n",
        ftell(ofp));
  }
  fwrite(compdata, sizeof(*compdata), complen, ofp);
  */
  fclose(ofp);
  Huff_Tree_Destroy(tree);
  free(compdata);
  free(gba_treetable);
  return 0;
}



#endif

