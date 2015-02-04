#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <crypto_box.h>
#include <crypto_secretbox.h>
#include <randombytes.h>

#include "network.h"
#include "utilities.h"
#include "padded_array.h"
#include "protocol.h"

const char *usage =
  "Usage: ./t8 server <sender's public key> <sender's private key> <recipient's public key> <hostname> <port>\n"
  "       ./t8 client <recipient's public key> <recpients secret key> <senders public key> <hostname> <post>\n";

int sendall(int s, char *buf, int len)
{
  int total = 0;        // how many bytes we've sent
  int bytesleft = len; // how many we have left to send
  int n;

  while(total < len) {
    //puts("...");
    n = send(s, buf+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }
  
  //*len = total; // return number actually sent here

  return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int main(int argc, char **argv) {
  int mode, sock;
  size_t length;

  // Alice's public and secret keys
  unsigned char *a_pk;
  unsigned char *a_sk;

  // Bob's public key
  unsigned char *b_pk;

  if (argc != 1 + 6) {
    fprintf(stderr, usage);
    return EXIT_FAILURE;
  }

  // read all the keys off the disk
  if (read_from_file(argv[2], &a_pk, &length)) {
    fprintf(stderr, "Failed to read sender's public key\n");
    return EXIT_FAILURE;
  }
  if (length != crypto_box_PUBLICKEYBYTES) {
    fprintf(stderr, "Failed to read sender's public key: incorrect size\n");
  }

  if (read_from_file(argv[3], &a_sk, &length)) {
    fprintf(stderr, "Failed to read sender's private key\n");
    return EXIT_FAILURE;
  }
  if (length != crypto_box_SECRETKEYBYTES) {
    fprintf(stderr, "Failed to read sender's private key: incorrect size\n");
  }

  if (read_from_file(argv[4], &b_pk, &length)) {
    fprintf(stderr, "Failed to read recipient's public key\n");
    return EXIT_FAILURE;
  }
  if (length != crypto_box_PUBLICKEYBYTES) {
    fprintf(stderr, "Failed to read recipient's public key: incorrect size\n");
  }

  if        (!strcmp(argv[1], "client")) {
    mode = send_mode;
  } else if (!strcmp(argv[1], "server")) {
    mode = listen_mode;
  } else {
    fprintf(stderr, usage);
    return EXIT_FAILURE;
  }

  // Connect
  if(start_networking(mode, argv[5], argv[6], &sock)) {
    puts("Could not network..");
    close(sock);
    return EXIT_FAILURE;
  }

  unsigned char key[crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES] = {0};
  key_exchange(a_pk, a_sk, b_pk, key, mode, sock);
  
  write_to_file((mode ? "alice.asymm" : "bob.asymm"), key + crypto_box_ZEROBYTES, crypto_secretbox_KEYBYTES);
  
  return 0;
}
