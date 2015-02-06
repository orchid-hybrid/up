#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utilities.h"

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

  *bytes = malloc(*bytes_length + 1);
  if(!*bytes) {
    fclose(fptr);
    return -1;
  }
  (*bytes)[*bytes_length] = '\0';
  
  size = fread(*bytes, *bytes_length, 1, fptr);
  if(size != 1) {
    fclose(fptr);
    return -1;
  }

  fclose(fptr);
  
  return 0;
}

void printhex(unsigned char *buf, int len) {
  int i;
  for(i = 0; i < len; i++) {
    printf("%02x ", buf[i]);
  }
  printf("\n");
}

void increment_nonce(unsigned char *n, int length) {
  int p;

  for(p = 0; p < length; p++) {
    n[p]++;
    if(!n[p]) {
      p++;
      continue;
    }
    else {
      break;
    }
  }
}
