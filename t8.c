#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
      puts("...");
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

    // generate an ephemeral keypair giving the public key
    // the padding to encrypt it
    padded_array a_epk;
    padded_array a_esk;
    padded_array a_epk_enc;
    padded_array b_epk_enc;
    padded_array b_epk;
    
    a_epk = padded_array_alloc(crypto_box_ZEROBYTES, crypto_box_PUBLICKEYBYTES);
    a_esk = padded_array_alloc(0, crypto_box_SECRETKEYBYTES);

    a_epk_enc = padded_array_alloc(crypto_box_BOXZEROBYTES, crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);
    b_epk_enc = padded_array_alloc(crypto_box_BOXZEROBYTES, crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);
    
    b_epk = padded_array_alloc(crypto_box_ZEROBYTES, crypto_box_PUBLICKEYBYTES);
    
    crypto_box_keypair(a_epk.start, a_esk.start);
    
    // encrypt our ephemeral public key with our non-ephemeral private key and
    // Bob's non-ephemeral public key
    crypto_box(a_epk_enc.bytes, a_epk.bytes, a_epk.padded_length, n, b_pk, a_sk);
    
    if(start_networking(mode, argv[5], argv[6], &sock)) {
        puts("Could not network..");
        close(sock);
        return EXIT_FAILURE;
    }
    if        (mode == send_mode) {
      // we begin by sending our encrypted ephemeral public key
      sendall(sock, a_epk_enc.start, a_epk_enc.length);
      
      // next, we recieve Bob's ephemeral public key,
      if (recv(sock, b_epk_enc.start, b_epk_enc.length, MSG_WAITALL) != b_epk_enc.length) {
	fprintf(stderr, "recv() failure");
	close(sock);
	return EXIT_FAILURE;
      }
      
      // and decrypt it using Bob's non-ephemeral public key, and our non-
      // ephemeral secret key
      if (crypto_box_open(b_epk.bytes, b_epk_enc.bytes, b_epk_enc.padded_length, n, b_pk, a_sk)) {
	fprintf(stderr, "crypto_box_open() failed in client\n");
	close(sock);
	return EXIT_FAILURE;
      }
    } else if (mode == listen_mode) {
      // daemon mode is identical to client mode except inverted send/recv
      
      if (recv(sock, b_epk_enc.start, b_epk_enc.length, MSG_WAITALL) != b_epk_enc.length) {
	fprintf(stderr, "recv() failure");
	close(sock);
	return EXIT_FAILURE;
      }
      
      sendall(sock, a_epk_enc.start, a_epk_enc.length);
      
      if (crypto_box_open(b_epk.bytes, b_epk_enc.bytes, b_epk_enc.padded_length,
			  n, b_pk, a_sk)) {
	fprintf(stderr, "crypto_box_open() failed in server\n");
	close(sock);
	return EXIT_FAILURE;
      }
    }
    
    //write_to_file((mode ? "alice.eph" : "bob.eph"), b_epk.start, b_epk.length);

    // first we allocate space for our asymmetric keys
    padded_array a_key;
    padded_array a_key_enc;

    padded_array b_key;
    padded_array b_key_enc;

    a_key = padded_array_alloc(crypto_box_ZEROBYTES, crypto_secretbox_KEYBYTES);
    a_key_enc = padded_array_alloc(crypto_box_BOXZEROBYTES, crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);

    b_key = padded_array_alloc(crypto_box_ZEROBYTES, crypto_secretbox_KEYBYTES);
    b_key_enc = padded_array_alloc(crypto_box_BOXZEROBYTES, crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES);

    // now we generate our asymmetric key
    randombytes(a_key.start, crypto_secretbox_KEYBYTES);

    // and encrypt it with Alice's ephemeral secret key and Bob's ephemeral public key
    crypto_box(a_key_enc.bytes, a_key.bytes, a_key.padded_length, n, b_epk.start, a_esk.start);
    
    // not encrypted yet
    if        (mode == send_mode) {
      sendall(sock, a_key_enc.start, a_key_enc.length);
      
      if (recv(sock, b_key_enc.start, b_key_enc.length, MSG_WAITALL) != b_key_enc.length) {
        fprintf(stderr, "recv() failure");
        close(sock);
        return EXIT_FAILURE;
      }
    }
    else if(mode == listen_mode) {
      if (recv(sock, b_key_enc.start, b_key_enc.length, MSG_WAITALL) != b_key_enc.length) {
        fprintf(stderr, "recv() failure");
        close(sock);
        return EXIT_FAILURE;
      }

      sendall(sock, a_key_enc.start, a_key_enc.length);
    }

    // now we decrypt Bob's symmetric key using his ephemeral public key and our ephemeral private key
    crypto_box_open(b_key.bytes, b_key_enc.bytes, b_key_enc.padded_length, n, b_epk.start, a_esk.start);
    
    int i;
    
    for(i = 0; i < crypto_secretbox_KEYBYTES; i++) {
      a_key.start[i] ^= b_key.start[i];
    }

    write_to_file((mode ? "alice.asymm" : "bob.asymm"), a_key.start, crypto_secretbox_KEYBYTES);
    
    return 0;
}
