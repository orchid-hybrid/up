#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <crypto_box.h>
#include <crypto_secretbox.h>
#include <randombytes.h>

#include "conf.h"
#include "protocol.h"
#include "network.h"

#define push_mode 0
#define pull_mode 1

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
  unsigned char key_bytes[crypto_secretbox_KEYBYTES] = { 0 };
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
  
  return EXIT_SUCCESS;
}
