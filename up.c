#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <crypto_box.h>
#include <crypto_secretbox.h>
#include <randombytes.h>

#include "conf.h"
#include "network.h"
#include "padded_array.h"
#include "protocol.h"

#define push_mode 0
#define pull_mode 1

#define CIPHERTEXT_LENGTH_A(l) (l + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES)
#define CIPHERTEXT_LENGTH_S(l) (l + crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES)
#define FAIL(err) { fprintf(stderr, "%s failure\n", err); close(sock); return EXIT_FAILURE; }

void usage() {
  puts(
       "USAGE: set up ~/.config/up/addressbook.conf and then\n"
       "\n"
       "Alice running a server, sending a file to Bob:\n"
       " alice$ ./up server push alice bob foo.txt\n"
       " bobby$ ./up client pull bob alice\n"
       "\n"
       "Alice running a server, receiving a file from Bob:\n"
       " alice$ ./up server pull alice bob\n"
       " bobby$ ./up client push bob alice foo.txt\n"
       );
}

void send_e(int sock,
            padded_array *buffer,
            int flags,
            unsigned char *key) {
  unsigned char n[crypto_secretbox_NONCEBYTES] = { 0 };
  padded_array buffer_e = padded_array_alloc(crypto_secretbox_BOXZEROBYTES, buffer->padded_length + crypto_secretbox_BOXZEROBYTES - crypto_secretbox_ZEROBYTES);
  if (crypto_secretbox(buffer_e.bytes,
                       buffer->bytes,
                       buffer->padded_length, n, key)) {
    puts("crypto_secretbox failed in send!");
    return;
  }
  
  sendall(sock, buffer_e.start, buffer_e.length);
}

void recv_e(int sock,
            padded_array *buffer,
            int flags,
            unsigned char *key) {
  unsigned char n[crypto_secretbox_NONCEBYTES] = { 0 };
  padded_array buffer_e = padded_array_alloc(crypto_secretbox_BOXZEROBYTES, buffer->length + crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES);
  
  recv(sock, buffer_e.start, buffer_e.length, MSG_WAITALL);
  
  if (crypto_secretbox_open(buffer->bytes,
                            buffer_e.bytes,
                            buffer_e.padded_length, n, key)) {
    puts("crypto_secretbox_open failed in recv!");
    return;
  }
}

int main(int argc, char **argv) {
  int network_mode;
  int send_mode;
  char *self_nick;
  char *contact_nick;
  
  char *filename;
  FILE *fptr;
  long file_length;

  conf *c;

  line *self_entry;
  line *contact_entry;
  line *e;

  char *a_sk_filename;
  char *b_pk_filename;

  int length;
  
  unsigned char *a_sk; // Alices secret key
  unsigned char *b_pk; // Bob's public key

  // our ephemeral symmetric key, with padding and without
  unsigned char key_bytes[crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES] = { 0 };
  unsigned char *key = key_bytes + crypto_box_ZEROBYTES;
  
  char *ip_address, *port;
  
  int sock;
  char contact_mode_str[5] = { 0 };
  int contact_mode;
  
  ////////////////////////////////
  // Argument parsing

  if(argc != 1 + 4 && argc != 1 + 5) {
    usage();
    return EXIT_FAILURE;
  }

  if(!strcmp(argv[1],"server") || !strcmp(argv[1],"s")) {
    network_mode = server_mode;
  }
  else if(!strcmp(argv[1],"client") || !strcmp(argv[1],"c")) {
    network_mode = client_mode;
  }
  else {
    usage();
    return EXIT_FAILURE;
  }
  
  if(!strcmp(argv[2],"push")) {
    send_mode = push_mode;
  }
  else if(!strcmp(argv[2],"pull")) {
    send_mode = pull_mode;
  }
  else {
    usage();
    return EXIT_FAILURE;
  }
  
  self_nick = argv[3];
  
  contact_nick = argv[4];

  if(send_mode == push_mode) {
    if(argc != 1 + 5) {
      usage();
      return EXIT_FAILURE;
    }

    filename = argv[5];
  }
  else if(send_mode == pull_mode) {
    if(argc != 1 + 4) {
      usage();
      return EXIT_FAILURE;
    }
  }

  ////////////////////////////////
  // Loading things

  // Load the configuration file
  
  c = load_conf_file("addressbook.conf");
  if(!c) {
    puts("Could not load addressbook.conf!");
    return EXIT_FAILURE;
  }
  if(validate_addressbook(c)) {
    puts("The addressbook.conf appears invalid!");
    return EXIT_FAILURE;
  }

  // Lookup entries inside it
  
  self_entry = lookup_addressbook(c, self_nick);
  if(!self_entry) {
    printf("Could not find <%s> in addressbook.conf!\n", self_nick);
    return EXIT_FAILURE;
  }
  
  contact_entry = lookup_addressbook(c, contact_nick);
  if(!contact_entry) {
    printf("Could not find <%s> in addressbook.conf!\n", contact_nick);
    return EXIT_FAILURE;
  }

  if(send_mode == push_mode) {
    // Check that the file we want to send exists
    // get it's size, open it

    // TODO
  }
  
  // we need to know our own secret key
  if(!(self_entry->length == 3 || self_entry->length == 5)) {
    printf("There is no secret key listed for <%s> in addressbook.conf!\n", self_nick);
    return EXIT_FAILURE;
  }
  a_sk_filename = self_entry->word[2];
  if (read_from_file(a_sk_filename, &a_sk, &length)) {
    fprintf(stderr, "Failed to read <%s> secret key\n", self_nick);
    return EXIT_FAILURE;
  }
  if (length != crypto_box_SECRETKEYBYTES) {
    fprintf(stderr, "Failed to read <%s>'s private key: incorrect size\n", self_nick);
    return EXIT_FAILURE;
  }
  
  // we need to know our contacts public key
  b_pk_filename = contact_entry->word[1];
  if (read_from_file(b_pk_filename, &b_pk, &length)) {
    fprintf(stderr, "Failed to read <%s> public key\n", contact_nick);
    return EXIT_FAILURE;
  }
  if (length != crypto_box_PUBLICKEYBYTES) {
    fprintf(stderr, "Failed to read <%s>'s public key: incorrect size\n", contact_nick);
    return EXIT_FAILURE;
  }
  
  if(network_mode == server_mode) {
    // if we are acting as a server then
    // we need an ip and port for our self

    e = self_entry;
  }
  else if(network_mode == client_mode) {
    // if we are acting as a client then
    // we need an ip and port for our contact
    
    e = contact_entry;
  }
  
  if(!(e->length == 4 || e->length == 5)) {
    printf("There is no ip/port listed for <%s> in addressbook.conf!\n", ((network_mode == server_mode) ? self_nick : contact_nick));
    return EXIT_FAILURE;
  }
  
  if(e->length == 4) {
    ip_address = e->word[2];
    port = e->word[3];
  }
  else if(e->length == 5) {
    ip_address = e->word[3];
    port = e->word[4];
  }

  ////////////////////////////////
  // Everything is loaded
  //
  // At this point everything we need has been parsed from the
  // argument list, and loaded and checked:
  //
  // modes:             network_mode, send_mode
  // Alices secret key: a_sk
  // Bobs public key:   b_pk
  // ip address:        ip_address
  // Port:              port
  //
  // and in the case of send_mode, we also have:
  // 
  // Filename:          filename
  // file handle:       fptr
  // Filesize:          file_length

  // TODO: Verify that this really holds

  // start networking and figure out whose-who and whether this
  // transaction will work

  if(start_networking(network_mode, ip_address, port, &sock)) {
    printf("Could not open networking to <%s:%s>\n", ip_address, port);
  }

  if(network_mode == server_mode) {
    if(sendall(sock, (send_mode == push_mode) ? "PUSH" : "PULL", 4)) {
      puts("network error 1");
      return EXIT_FAILURE;
    }
    
    recv(sock, &contact_mode_str, 4, MSG_WAITALL);
  }
  else if(network_mode == client_mode) {
    recv(sock, &contact_mode_str, 4, MSG_WAITALL);
    
    if(sendall(sock, (send_mode == push_mode) ? "PUSH" : "PULL", 4)) {
      puts("network error 2");
      return EXIT_FAILURE;
    }
  }

  if(!strcmp("PUSH", contact_mode_str)) {
    contact_mode = push_mode;
  }
  else if(!strcmp("PULL", contact_mode_str)) {
    contact_mode = pull_mode;
  }
  else {
    puts("contact sent garbage mode string during handshake");
    return EXIT_FAILURE;
  }

  if(send_mode == contact_mode) {
    printf("mode error: both parties trying to <%s> a file to the other.\n", (send_mode == push_mode) ? "PUSH" : "PULL");
    return EXIT_FAILURE;
  }
  
  key_exchange(a_sk, b_pk, key_bytes, network_mode, sock);
  
  if(send_mode == push_mode) {
    //padded_array send_buf = padded_array_convert("hello", crypto_secretbox_ZEROBYTES, 5);
    //send_e(sock, &send_buf, 0, key);

    padded_array plaintext;
    padded_array ciphertext;

    unsigned char n[crypto_secretbox_NONCEBYTES] = { 0 };

    plaintext = padded_array_alloc(crypto_secretbox_ZEROBYTES, 5);
    memcpy(plaintext.start, "HELLO", plaintext.length);
    
    ciphertext = padded_array_alloc(crypto_secretbox_BOXZEROBYTES, 5 + crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES);
    
    if (crypto_secretbox(ciphertext.bytes, plaintext.bytes, plaintext.padded_length, n, key)) {
      puts("crypto_secretbox failed in send!");
      return;
    }
    
    sendall(sock, ciphertext.start, ciphertext.length);

    //printhex(key, crypto_secretbox_KEYBYTES);
    //printhex(ciphertext.start, ciphertext.length);
  }
  else if(send_mode == pull_mode) {
    //padded_array recv_buf = padded_array_alloc(crypto_secretbox_BOXZEROBYTES, CIPHERTEXT_LENGTH_S(5));
    //recv_e(sock, &recv_buf, MSG_WAITALL, key);

    padded_array ciphertext;
    padded_array plaintext;

    unsigned char n[crypto_secretbox_NONCEBYTES] = { 0 };
    
    ciphertext = padded_array_alloc(crypto_secretbox_BOXZEROBYTES, 5 + crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES);
    recv(sock, ciphertext.start, ciphertext.length, MSG_WAITALL);

    //printhex(key, crypto_secretbox_KEYBYTES);
    //printhex(ciphertext.start, ciphertext.length);
    
    plaintext = padded_array_alloc(crypto_secretbox_ZEROBYTES, 5);
    
    if (crypto_secretbox_open(plaintext.bytes, ciphertext.bytes, ciphertext.padded_length, n, key)) {
      puts("crypto_secretbox_open failed in recv!");
      return;
    }

    printf("%c%c%c%c%c\n", plaintext.start[0], plaintext.start[1], plaintext.start[2], plaintext.start[3], plaintext.start[4]);
  }
  
  return EXIT_SUCCESS;
}
