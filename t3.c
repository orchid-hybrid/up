#include <stdlib.h>
#include <stdio.h>

#include "conf.h"

void main(int argc, char **argv) {
  conf *c;
  
  if(argc != 2) return;

  c = load_conf_file(argv[1]);
  if(!c) return;

  int i;
  int j;

  for(i = 0; i < c->length; i++) {
    for(j = 0; j < c->lines[i].length; j++) {
      printf("<%s>", c->lines[i].words[j]);
    }
    printf("\n");
  }
}
