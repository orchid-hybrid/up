typedef struct {
  int padding;

  // The array `bytes` is of length `padded_length = padding + length`
  unsigned char *bytes;
  int padded_length;

  // The array `start` is of length `length`
  unsigned char *start;
  int length;
} padded_array;

padded_array padded_array_alloc(int p, int s);
padded_array padded_array_make(unsigned char *b, int p, int s);
padded_array padded_array_convert(unsigned char *b, int p, int s);