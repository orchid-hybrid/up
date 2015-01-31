#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// this tests splitting a string into words
void split(char *c, char ***words_out, int *len_out) {
  char **words;
  int len;
  int alloc;
  
  len = 0;
  alloc = 1;
  words = malloc(alloc*sizeof(char*));

  while(1) {
    // skip to the start of a word
    while(*c == ' ') { *c = 0; c++; }
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
    while(*c && *c != ' ') { c++; }
    
  }
}


void main(void) {
  char *f = "   foo bar baz  quux   a bb ccc ddd eeee     adsfad sfd dafsfdsfddffd   ";
  char *c;
  
  c = malloc((strlen(f)+1)*sizeof(char)); // the +1 here is very important
  strcpy(c, f);
  
  int len;
  char **words;
  
  split(c, &words, &len);

  int i;

  for(i = 0; i < len; i++) {
    printf("<%s>\n", words[i]);
  }
}
