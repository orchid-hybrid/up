#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <crypto_box.h>
#include <randombytes.h>

#include "utilities.h"

// This program tests encryption with a keypair

void main(int argc, char **argv) {
  char *pubkey_filename, *seckey_filename, *message_plain_filename, *message_enc_filename;
  
  unsigned char pk[crypto_box_PUBLICKEYBYTES];
  unsigned char sk[crypto_box_SECRETKEYBYTES];
  
  unsigned char *pubkey_data;
  size_t pubkey_data_size;

  unsigned char *seckey_data;
  size_t seckey_data_size;
  
  if(argc != 1 + 4) return;
  pubkey_filename = argv[1];
  seckey_filename = argv[2];
  message_plain_filename = argv[3];
  message_enc_filename = argv[4];

  printf("pubkey:%s seckey:%s plaintext:%s -> ciphertext:%s\n", pubkey_filename, seckey_filename, message_plain_filename, message_enc_filename);

  // read public key from a file
  if(read_from_file(pubkey_filename, &pubkey_data, &pubkey_data_size)) return;
  if(read_from_file(seckey_filename, &seckey_data, &seckey_data_size)) return;  

  // check that it is the right size
  if(pubkey_data_size != crypto_box_PUBLICKEYBYTES) {
    puts("Public key is wrong size!");
    return;
  }
  
  // check that it is the right size
  if(seckey_data_size != crypto_box_SECRETKEYBYTES) {
    puts("Public key is wrong size!");
    return;
  }

  unsigned char *plaintext;
  size_t plaintext_size;
  
  unsigned char *plaintext_padded;

  if(read_from_file(message_plain_filename, &plaintext, &plaintext_size)) return;

  unsigned char *c;
  plaintext_padded = calloc(crypto_box_ZEROBYTES + plaintext_size, sizeof(char));
  c = calloc(crypto_box_ZEROBYTES + plaintext_size, sizeof(char));
  
  memcpy(plaintext_padded + crypto_box_ZEROBYTES, plaintext, plaintext_size);

  unsigned char n[crypto_box_NONCEBYTES] = { 0 };
  //randombytes(n, crypto_box_NONCEBYTES);
  
  // encrypt

  if(crypto_box(c, plaintext_padded, crypto_box_ZEROBYTES + plaintext_size, n, pubkey_data, seckey_data))
    puts("NO1");

  

  // write to message.enc

  if(write_to_file(message_enc_filename, c, plaintext_size + crypto_box_ZEROBYTES))
    puts("NO2");
}
