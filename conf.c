#include <stdlib.h>

#include "conf.h"
#include "utilities.h"

// The way this parser works is to read the entire file
// and then chop it up into lines
// then it will chop each line into words

void split(char *c, char sep, char ***words_out, int *len_out) {
  char **words;
  int len;
  int alloc;
  
  len = 0;
  alloc = 1;
  words = malloc(alloc*sizeof(char*));
  
  while(1) {
    // skip to the start of a word
    while(*c == sep) { *c = 0; c++; }
    if(*c == 0) {
      *len_out = len;
      *words_out = words;
      return;
    }

    // note down the start of a word
    words[len] = c;
    len++;
    if(len >= alloc) {
      alloc *= 2;
      words = realloc(words, alloc*sizeof(char*));
    }

    // skip the words content
    while(*c && *c != sep) { c++; }
    
  }
}

conf *load_conf_file(char *filename) {
  char *bytes;
  size_t length;

  char **lines;
  int lines_len;

  char **words;
  int words_len;

  int i;
  
  conf *c;

  c = malloc(sizeof(conf));
  
  if(read_from_file(filename, &bytes, &length)) {
    return NULL;
  }
  
  split(bytes, '\n', &lines, &lines_len);
  c->length = lines_len;
  c->lines = malloc(c->length*sizeof(line));
  
  for(i = 0; i < lines_len; i++) {
    split(lines[i], ' ', &c->lines[i].word, &c->lines[i].length);
  }
  
  return c;
}