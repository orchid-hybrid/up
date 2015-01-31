#include <stdio.h>
#include <unistd.h>

#include <crypto_sign.h>

#include "utilities.h"

// This program tests generating a keypair
// as well as saving to and loading from files

void main(void) {
  unsigned char pk[crypto_sign_PUBLICKEYBYTES];
  unsigned char sk[crypto_sign_SECRETKEYBYTES];
  
  unsigned char *pk2;
  size_t big;

  crypto_sign_keypair(pk,sk);

  write_to_file("public_key.bin", pk, crypto_sign_PUBLICKEYBYTES);
  write_to_file("secret_key.bin", sk, crypto_sign_SECRETKEYBYTES);
  if(!read_from_file("public_key.bin", &pk2, &big)) puts("ok!");
  printf("%d %d\n", big, memcmp(pk, pk2, big));
}
