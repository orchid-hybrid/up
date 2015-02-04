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

  // FIXME !!!!!! zero nonce is bad!
  unsigned char n[crypto_box_NONCEBYTES] = { 0 };

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
  
  unsigned char a_epk_bytes[crypto_box_ZEROBYTES + crypto_box_PUBLICKEYBYTES] = { 0 };
  unsigned char a_esk_bytes[0 + crypto_box_SECRETKEYBYTES] = { 0 };
  unsigned char a_epk_enc_bytes[crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES] = { 0 };
  unsigned char b_epk_enc_bytes[crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES] = { 0 };
  unsigned char b_epk_bytes[crypto_box_ZEROBYTES + crypto_box_PUBLICKEYBYTES] = { 0 };
  
  padded_array a_epk;
  padded_array a_esk;
  padded_array a_epk_enc;
  padded_array b_epk_enc;
  padded_array b_epk;
  
  a_epk = padded_array_make(a_epk_bytes, crypto_box_ZEROBYTES, crypto_box_PUBLICKEYBYTES);
  a_esk = padded_array_make(a_esk_bytes, 0, crypto_box_SECRETKEYBYTES);
  
  a_epk_enc = padded_array_make(a_epk_enc_bytes, crypto_box_BOXZEROBYTES, crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);
  b_epk_enc = padded_array_make(b_epk_enc_bytes, crypto_box_BOXZEROBYTES, crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);
  
  b_epk = padded_array_make(b_epk_bytes, crypto_box_ZEROBYTES, crypto_box_PUBLICKEYBYTES);
  
#define FAIL(er) { fprintf(stderr, "%s failure\n", er); close(sock); return EXIT_FAILURE; }
  
  // 1. Alice and Bob generate new ephemeral keypairs
  crypto_box_keypair(a_epk.start, a_esk.start);
  
  // 2a. Alice encrypts her ephemeral public key with her non-ephemeral private key and Bob's non-ephemeral public key
  // 2b. Bob encrypts his ephemeral public key with his non-ephemeral private key and Alice's non-ephemeral public key
  assert(a_epk_enc.padded_length == a_epk.padded_length);
  if(crypto_box(a_epk_enc.bytes, a_epk.bytes, a_epk.padded_length, n, b_pk, a_sk)) { FAIL("crypto_box 1"); }

  // 3. Alice and Bob send each other their encrypted ephemeral public keys
  if(mode == send_mode) {
    // we begin by sending our encrypted ephemeral public key
    sendall(sock, a_epk_enc.start, a_epk_enc.length);
    
    // next, we recieve Bob's ephemeral public key,
    if (recv(sock, b_epk_enc.start, b_epk_enc.length, MSG_WAITALL) != b_epk_enc.length) { FAIL("recv 2") }
  }
  else if(mode == listen_mode) { 
    // daemon mode is identical to client mode except inverted send/recv
    if (recv(sock, b_epk_enc.start, b_epk_enc.length, MSG_WAITALL) != b_epk_enc.length) { FAIL("recv 3") }
    
    sendall(sock, a_epk_enc.start, a_epk_enc.length);
  }

  // and decrypt it using Bob's non-ephemeral public key, and our non-ephemeral secret key
  if(crypto_box_open(b_epk.bytes, b_epk_enc.bytes, b_epk_enc.padded_length, n, b_pk, a_sk)) { FAIL("crypto_box_open 4") }
  
  unsigned char a_key_bytes[crypto_box_ZEROBYTES + crypto_secretbox_KEYBYTES] = { 0 };
  unsigned char a_key_enc_bytes[crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES] = { 0 };
  unsigned char b_key_bytes[crypto_box_ZEROBYTES + crypto_secretbox_KEYBYTES] = { 0 };
  unsigned char b_key_enc_bytes[crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES] = { 0 };
  
  padded_array a_key;
  padded_array a_key_enc;

  padded_array b_key;
  padded_array b_key_enc;
  
  a_key = padded_array_make(a_key_bytes, crypto_box_ZEROBYTES, crypto_secretbox_KEYBYTES);
  a_key_enc = padded_array_make(a_key_enc_bytes, crypto_box_BOXZEROBYTES, crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);

  b_key = padded_array_make(b_key_bytes, crypto_box_ZEROBYTES, crypto_secretbox_KEYBYTES);
  b_key_enc = padded_array_make(b_key_enc_bytes, crypto_box_BOXZEROBYTES, crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);
  
  // 4. Alice and Bob both generate half of a symmetric key
  randombytes(a_key.start, crypto_secretbox_KEYBYTES);
  
  // 5a. Alice encrypts her half of the symmetric key with her ephemeral private key and Bob's ephemeral public key, and sends this to him
  // 5b. Bob encrypts his half of the symmetric key with his ephemeral private key and Alice's ephemeral public key, and sends this to her
  if(crypto_box(a_key_enc.bytes, a_key.bytes, a_key.padded_length, n, b_epk.start, a_esk.start)) { FAIL("crypto_box 5") }
  if(mode == send_mode) {
    sendall(sock, a_key_enc.start, a_key_enc.length);
    if (recv(sock, b_key_enc.start, b_key_enc.length, MSG_WAITALL) != b_key_enc.length) { FAIL("recv 6") }
  }
  else if(mode == listen_mode) {
    if (recv(sock, b_key_enc.start, b_key_enc.length, MSG_WAITALL) != b_key_enc.length) { FAIL("recv 7") }
    sendall(sock, a_key_enc.start, a_key_enc.length);
  }
  memset(a_key_enc.bytes, 0x00, a_key_enc.padded_length);
  
  // 6. Alice and Bob decrypt their respective halves of the symmetric key
  // now we decrypt Bob's symmetric key using his ephemeral public key and our ephemeral private key
  if(crypto_box_open(b_key.bytes, b_key_enc.bytes, b_key_enc.padded_length, n, b_epk.start, a_esk.start)) { FAIL("crypto_box_open 8") }
  
  int i;
  
  for(i = 0; i < crypto_secretbox_KEYBYTES; i++) {
    a_key.start[i] ^= b_key.start[i];
  }
  
  // 7. Alice and Bob erase their ephemeral keypairs
  memset(a_epk.start, 0x00, a_epk.length);
  memset(a_esk.start, 0x00, a_esk.length);
  
  write_to_file((mode ? "alice.asymm" : "bob.asymm"), a_key.start, crypto_secretbox_KEYBYTES);
  
  return 0;
}
