#include <stdlib.h>

#include "padded_array.h"

padded_array padded_array_alloc(int p, int s) {
  padded_array a;
  a.padding = p;
  a.padded_length = p + s;
  a.length = s;
  a.bytes = calloc(p + s, sizeof(char));
  a.start = a.bytes + p;
  return a;
}
