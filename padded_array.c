#include <stdlib.h>

#include "padded_array.h"

padded_array padded_array_alloc(int p, int s) {
  return padded_array_make(calloc(p + s, sizeof(char)), p, s);
}

padded_array padded_array_make(unsigned char *b, int p, int s) {
  padded_array a;
  a.padding = p;
  a.padded_length = p + s;
  a.length = s;
  a.bytes = b;
  a.start = a.bytes + p;
  return a;
}
