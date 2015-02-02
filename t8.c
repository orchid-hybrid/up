#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <crypto_box.h>
#include <randombytes.h>

#include "network.h"
#include "utilities.h"

#define ENCRYPTED_PUBLIC_KEY_SIZE (crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES)
#define PADDING crypto_box_ZEROBYTES

const char *usage = "Usage: ./t8 (client|server) <sender's public key> <sender's private key> <recipient's public key> <hostname> <port>\n";

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

    // Alice's public and secret keys
    unsigned char *a_pk;
    unsigned char *a_sk;

    // Bob's public key
    unsigned char *b_pk;

    // Alice's ephemeral public and secret keys
    // includes padding for use with crypto_box()
    unsigned char a_epk[crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES];
    unsigned char a_esk[crypto_box_SECRETKEYBYTES];

    // Bob's ephemeral public key
    unsigned char b_epk[crypto_box_PUBLICKEYBYTES];

    // Alice and Bob's encrypted ephemeral public keys
    unsigned char a_epk_e[crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES];
    unsigned char b_epk_e[crypto_box_PUBLICKEYBYTES + crypto_box_ZEROBYTES];

    unsigned char scratch[1024];

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

    // generate an ephemeral keypair
    crypto_box_keypair(a_epk + crypto_box_ZEROBYTES, a_esk);

    // our padding has to be set to 0
    memset(a_epk, 0, crypto_box_ZEROBYTES);

    // encrypt our ephemeral public key with our non-ephemeral private key and
    // Bob's non-ephemeral public key
    crypto_box(a_epk_e, a_epk, crypto_box_PUBLICKEYBYTES +
                               crypto_box_ZEROBYTES, 0, a_sk, b_pk);
  
    if(start_networking(mode, argv[5], argv[6], &sock)) {
        puts("Could not network..");
        return EXIT_FAILURE;
    }
    if        (mode == send_mode) {
        sendall(sock, a_epk_e, ENCRYPTED_PUBLIC_KEY_SIZE);

        if (recv(sock, &b_epk_e, ENCRYPTED_PUBLIC_KEY_SIZE, MSG_WAITALL) !=
            crypto_box_PUBLICKEYBYTES) {
            fprintf(stderr, "recv() failure");
            return EXIT_FAILURE;
        }
    } else if (mode == listen_mode){
        if (recv(sock, &b_epk_e, ENCRYPTED_PUBLIC_KEY_SIZE, MSG_WAITALL) !=
            crypto_box_PUBLICKEYBYTES) {
            fprintf(stderr, "recv() failure");
            return EXIT_FAILURE;
        }

        sendall(sock, a_epk_e, ENCRYPTED_PUBLIC_KEY_SIZE);

        //crypto_box(epk_a_e, esk_a);
    }
}