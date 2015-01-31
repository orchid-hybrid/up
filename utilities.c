#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int write_to_file(char *filename, unsigned char bytes[], size_t bytes_length) {
  FILE *fptr;
  size_t written;

  fptr = fopen(filename, "wb");
  if(!fptr) return -1;

  written = fwrite(bytes, bytes_length, 1, fptr);
  if(written != 1) {
    fclose(fptr);
    return -1;
  }
  
  fclose(fptr);
  
  return 0;
}

int read_from_file(char *filename, unsigned char **bytes, size_t *bytes_length) {
  FILE *fptr;
  size_t size;

  fptr = fopen(filename, "rb");
  if(!fptr) return -1;

  fseek(fptr, 0, SEEK_END);
  *bytes_length = ftell(fptr);
  fseek(fptr, 0, SEEK_SET);

  *bytes = malloc(*bytes_length);
  if(!*bytes) {
    fclose(fptr);
    return -1;
  }
  
  size = fread(*bytes, *bytes_length, 1, fptr);
  if(size != 1) {
    fclose(fptr);
    return -1;
  }

  fclose(fptr);
  
  return 0;
}
