#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crypto_box.h>
#include <randombytes.h>

#include "utilities.h"
#include "conf.h"

#define encrypt_mode 0
#define decrypt_mode 1

// Addressbook format is a list of lines with form
// name pubkey_file
// or
// name pubkey_file privkey_file

int validate_addressbook(conf *c) {
  int i;

  for(i = 0; i < c->length; i++) {
    if(!(c->lines[i].length == 2 || c->lines[i].length == 3))
      return -1;
  }

  return 0;
}

line *lookup_addressbook(conf *c, char *name) {
  int i;

  for(i = 0; i < c->length; i++) {
    if(!strcmp(name, c->lines[i].word[0]))
      return &c->lines[i];
  }

  return NULL;
}

int load_secret_key(conf *c, char *name, unsigned char *k) {
  line *entry;

  unsigned char *bytes;
  size_t length;

  entry = lookup_addressbook(c, name);
  if(!entry) {
    printf("Could not find <%s> in the addressbook\n", name);
    return -1;
  }
  if(entry->length != 3) {
    printf("Could not find secret key for <%s> in the addressbook\n", name);
    return -1;
  }
  
  if(read_from_file(entry->word[2], &bytes, &length)) {
    printf("Could not load <%s>'s secret keyfile <%s>\n", name, entry->word[2]);
    return -1;
  }

  if(length != crypto_box_SECRETKEYBYTES) {
    printf("<%s>'s secret keyfile <%s> is not the right size\n", name, entry->word[2]);
    return -1;
  }

  printf("Loaded secret key %s\n", entry->word[2]);
  memcpy(k, bytes, crypto_box_SECRETKEYBYTES);
  
  free(bytes);
  
  return 0;
}

int load_public_key(conf *c, char *name, unsigned char *k) {
  line *entry;

  unsigned char *bytes;
  size_t length;
  
  entry = lookup_addressbook(c, name);
  if(!entry) {
    printf("Could not find <%s> in the addressbook\n", name);
    return -1;
  }
  if(!(entry->length == 2 || entry->length == 3)) {
    printf("Could not find <%s> in the addressbook\n", name);
    return -1;
  }
  
  if(read_from_file(entry->word[1], &bytes, &length)) {
    printf("Could not load <%s>'s public keyfile <%s>\n", name, entry->word[1]);
    return -1;
  }

  if(length != crypto_box_PUBLICKEYBYTES) {
    printf("<%s>'s public keyfile <%s> is not the right size\n", name, entry->word[1]);
    return -1;
  }
  
  printf("Loaded public key %s\n", entry->word[1]);
  memcpy(k, bytes, crypto_box_PUBLICKEYBYTES);

  free(bytes);
  
  return 0;
}

int main(int argc, char **argv) {
  conf *c;
  int mode; // 0 for encrypt, 1 for decrypt

  char *sender, *recipient, *input_filename, *output_filename;

  unsigned char *input_bytes;
  size_t input_size;
  
  unsigned char pk[crypto_box_PUBLICKEYBYTES];
  unsigned char sk[crypto_box_SECRETKEYBYTES];
  
  unsigned char n[crypto_box_NONCEBYTES] = { 0 }; // You should not use a nonce of zero in any real situation!
  
  if(argc != 1 + 5 || (strcmp(argv[1],"enc") && strcmp(argv[1],"dec"))) {
    puts("Usage: ./t6 (enc|dec) <sender> <recipient> <input-file> <output-file>\n");
    return EXIT_SUCCESS;
  }
  
  if(!strcmp(argv[1],"enc")) mode = encrypt_mode; else mode = decrypt_mode;
  sender = argv[2];
  recipient = argv[3];
  input_filename = argv[4];
  output_filename = argv[5];
  
  c = load_conf_file("addressbook.conf");
  
  if(!c) {
    puts("Cannot find addressbook.conf!");
    return EXIT_FAILURE;
  }
  
  if(validate_addressbook(c)) {
    puts("Invalid addressbook!");
    return EXIT_FAILURE;
  }

  if(read_from_file(input_filename, &input_bytes, &input_size)) {
    printf("Could not read input file <%s>!\n", input_filename);
    return EXIT_FAILURE;
  }
  
  if(mode == encrypt_mode) {
    if(load_secret_key(c, sender, sk)) return EXIT_FAILURE;
    if(load_public_key(c, recipient, pk)) return EXIT_FAILURE;
  }
  else if(mode == decrypt_mode) {
    if(load_secret_key(c, recipient, sk)) return EXIT_FAILURE;
    if(load_public_key(c, sender, pk)) return EXIT_FAILURE;
  }
  
  if(mode == encrypt_mode) {
    unsigned char *plaintext_padded;
    unsigned char *c;
    
    plaintext_padded = calloc(crypto_box_ZEROBYTES + input_size, sizeof(char));
    memcpy(plaintext_padded + crypto_box_ZEROBYTES, input_bytes, input_size);
    
    c = calloc(crypto_box_ZEROBYTES + input_size, sizeof(char));
    
    if(crypto_box(c, plaintext_padded, crypto_box_ZEROBYTES + input_size, n, pk, sk)) {
      puts("failed to perform encryption");
      return EXIT_FAILURE;
    }
    if(write_to_file(output_filename, c + crypto_box_BOXZEROBYTES, input_size + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES)) {
      puts("failed to writed encrypted data");
      return EXIT_FAILURE;
    }
  }
  else if(mode == decrypt_mode) {
    unsigned char *ciphertext_padded;
    unsigned char *m;
    
    ciphertext_padded = calloc(crypto_box_BOXZEROBYTES + input_size, sizeof(char));
    memcpy(ciphertext_padded + crypto_box_BOXZEROBYTES, input_bytes, input_size);
    
    m = calloc(crypto_box_BOXZEROBYTES + input_size, sizeof(char));
    
    if(crypto_box_open(m, ciphertext_padded, crypto_box_BOXZEROBYTES + input_size, n, pk, sk)) {
      puts("failed to perform decryption");
      return EXIT_FAILURE;
    }
    if(write_to_file(output_filename, m + crypto_box_ZEROBYTES, input_size + crypto_box_BOXZEROBYTES - crypto_box_ZEROBYTES)) {
      puts("failed to writed decrypted data");
      return EXIT_FAILURE;
    }
  }
  
  return EXIT_SUCCESS;
}

