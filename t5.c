#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <crypto_box.h>
#include <randombytes.h>

#include "utilities.h"

// This program tests decryption with a keypair

void main(int argc, char **argv) {
  char *pubkey_filename, *seckey_filename, *message_plain_filename, *message_enc_filename;
  
  unsigned char pk[crypto_box_PUBLICKEYBYTES];
  unsigned char sk[crypto_box_SECRETKEYBYTES];
  
  unsigned char *pubkey_data;
  size_t pubkey_data_size;

  unsigned char *seckey_data;
  size_t seckey_data_size;
  
  if(argc != 1 + 4) return;
  seckey_filename = argv[1];
  pubkey_filename = argv[2];
  message_enc_filename = argv[3];
  message_plain_filename = argv[4];
  printf("seckey:%s pubkey:%s ciphertext:%s -> decrypted:%s\n", seckey_filename, pubkey_filename, message_enc_filename, message_plain_filename);

  // read public key from a file
  if(read_from_file(seckey_filename, &seckey_data, &seckey_data_size)) return;  
  if(read_from_file(pubkey_filename, &pubkey_data, &pubkey_data_size)) return;

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

  unsigned char n[crypto_box_NONCEBYTES] = { 0 };

  unsigned char *ciphertext;
  size_t ciphertext_size;
  
  unsigned char *ciphertext_padded;

  if(read_from_file(message_enc_filename, &ciphertext, &ciphertext_size)) return;

  unsigned char *m;
  m = calloc(ciphertext_size, sizeof(char));

  // decrypt
  if(crypto_box_open(m, ciphertext, ciphertext_size, n, pubkey_data, seckey_data))
    puts("NO1");
  
  // write to message.enc
  
  if(write_to_file(message_plain_filename, m + crypto_box_BOXZEROBYTES, ciphertext_size))
    puts("NO2");
}
