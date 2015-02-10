#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <libgen.h>

#include <crypto_box.h>
#include <crypto_hash.h>
#include <crypto_secretbox.h>
#include <randombytes.h>

#include "conf.h"
#include "network.h"
#include "padded_array.h"
#include "protocol.h"

#define push_mode 0
#define pull_mode 1

#define MB(b) (1024*1024*b)
#define BLOCKSIZE MB(4)

#define CIPHERTEXT_LENGTH_A(l) (l + crypto_box_ZEROBYTES - crypto_box_BOXZEROBYTES)
#define CIPHERTEXT_LENGTH_S(l) (l + crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES)
#define FAIL(err) { fprintf(stderr, "%s failure\n", err); close(sock); return EXIT_FAILURE; }

typedef enum {
  HAVE = 0,
  GIVE = 1
} BLOCK_RESPONSE;

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

padded_array plaintext_alloc(int s) {
  return padded_array_alloc(crypto_secretbox_ZEROBYTES, s);
}

padded_array ciphertext_alloc(int s) {
  return padded_array_alloc(crypto_secretbox_BOXZEROBYTES, s + crypto_secretbox_ZEROBYTES - crypto_secretbox_BOXZEROBYTES);
}

int send_e(int sock,
            int flags,
            // plaintext, padding must be cypto_secretbox_ZEROBYTES
            padded_array *p,
            // ciphertext, padding must be crypto_secretbox_BOXZEROBYTES
            padded_array *c,
            unsigned char *n,
            unsigned char *key) {
  if (crypto_secretbox(c->bytes,
                       p->bytes,
                       p->padded_length, n, key)) {
    puts("crypto_secretbox failed in send!");
    return -1;
  }

  increment_nonce(n, crypto_secretbox_NONCEBYTES);
  sendall(sock, c->start, c->length);
  return 0;
}

int recv_e(int sock,
            int flags,
            // plaintext, padding must be cypto_secretbox_ZEROBYTES
            padded_array *p,
            // ciphertext, padding must be crypto_secretbox_BOXZEROBYTES
            padded_array *c,
            unsigned char *n,
            unsigned char *key) {
  recv(sock, c->start, c->length, MSG_WAITALL);
  
  if (crypto_secretbox_open(p->bytes,
                            c->bytes,
                            c->padded_length, n, key)) {
    puts("crypto_secretbox_open failed in recv!");
      return -1;
  }

  increment_nonce(n, crypto_secretbox_NONCEBYTES);
  return 0;
}

int main(int argc, char **argv) {
  int network_mode;
  int send_mode;
  char *self_nick;
  char *contact_nick;
  
  char *filename;
  char *filename_base;
  long file_length;
  FILE *fptr;

  conf *c;

  line *self_entry;
  line *contact_entry;
  line *e;

  char *a_sk_filename;
  char *b_pk_filename;

  size_t length;
  
  unsigned char *a_sk; // Alices secret key
  unsigned char *b_pk; // Bob's public key

  // our ephemeral symmetric key, with padding and without
  unsigned char key_bytes[crypto_secretbox_KEYBYTES + crypto_box_ZEROBYTES] = { 0 };
  unsigned char *key = key_bytes + crypto_box_ZEROBYTES;
  
  char *ip_address, *port;
  
  int sock;
  char contact_mode_str[5] = { 0 };
  int contact_mode;
  
  padded_array plaintext;
  padded_array ciphertext;
  
  unsigned char n[crypto_secretbox_NONCEBYTES] = { 0 };
  
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
    filename_base = basename(filename);
    if(strlen(filename_base) > 64) {
      puts("filename is too long! >64 characters");
      return EXIT_FAILURE;
    }
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
    struct stat buf;
    if(stat(filename, &buf)) {
      fprintf(stderr, "could not stat file <%s>\n", filename);
      return EXIT_FAILURE;
    }

    file_length = buf.st_size;
    
    fptr = fopen(filename, "r");
    if(!fptr) {
      fprintf(stderr, "could not open file <%s>\n", filename);
      return EXIT_FAILURE;
    }
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
    return EXIT_FAILURE;
  }
  puts("got a connection from <>!"); // TODO print address
  
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
  
  if(key_exchange(a_sk, b_pk, key_bytes, network_mode, sock)) {
    fprintf(stderr, "key exchange failed");
    return EXIT_FAILURE;
  }
  
  unsigned char network_length[4];

  padded_array plain = plaintext_alloc(4);
  padded_array cipher = ciphertext_alloc(4);
  
  if(send_mode == push_mode) {
    length = file_length;
    
    network_length[0] = length & 0xFF;
    network_length[1] = (length >> 8) & 0xFF;
    network_length[2] = (length >> 16) & 0xFF;
    network_length[3] = (length >> 24) & 0xFF;

    memcpy(plain.start, network_length, 4);
    if(send_e(sock, 0, &plain, &cipher, n, key)) { puts("F1"); return EXIT_FAILURE; }
    
    plain = plaintext_alloc(64);
    cipher = ciphertext_alloc(64);

    strncpy(plain.start, filename_base, 64);
    
    if(send_e(sock, 0, &plain, &cipher, n, key)) { puts("F2"); return EXIT_FAILURE; }
    
    char *buffer = malloc(length);
    fread(buffer, file_length, 1, fptr);
    
    plain = plaintext_alloc(crypto_hash_BYTES);
    cipher = ciphertext_alloc(crypto_hash_BYTES);

    crypto_hash(plain.start, buffer, length);
    if(send_e(sock, 0, &plain, &cipher, n, key)) { puts("F2.5"); return EXIT_FAILURE; }
    
    plain = plaintext_alloc(1);
    cipher = ciphertext_alloc(1);

    recv_e(sock, MSG_WAITALL, &plain, &cipher, n, key);

    if(plain.start[0] == HAVE) {
      printf("got HAVE\n");
      return EXIT_SUCCESS;
    }
    else if(plain.start[0] == GIVE) {
      printf("got GIVE\n");
      plain = plaintext_alloc(length);
      cipher = ciphertext_alloc(length);

      memcpy(plain.start, buffer, length);

      if(send_e(sock, 0, &plain, &cipher, n, key)) { puts("F3"); return EXIT_FAILURE; }
    }
  }
  else if(send_mode == pull_mode) {
    if(recv_e(sock, MSG_WAITALL, &plain, &cipher, n, key)) { puts("F3"); return EXIT_FAILURE; }

    memcpy(network_length, plain.start, 4);

    length = network_length[0];
    length |= network_length[1] << 8;
    length |= network_length[2] << 16;
    length |= network_length[3] << 24;

    plain = plaintext_alloc(64);
    cipher = ciphertext_alloc(64);
    
    if(recv_e(sock, MSG_WAITALL, &plain, &cipher, n, key)) { puts("F3"); return EXIT_FAILURE; }

    char base[65] = { 0 };

    strncpy(base, plain.start, 64);

    filename = base;
    printf("FILENAME IS %s\n", filename);
    
    // check if the file exists
    // if not create it with the given length
    // if it exists, verify it has the correct length, if not bail out

    struct stat buf;
    int have_file;

    if(!stat(filename, &buf)) {
      file_length = buf.st_size;

      if(file_length != length) {
        fprintf(stderr, "incorrect size\n");
        return EXIT_FAILURE;
      }
    
      fptr = fopen(filename, "rw");
      if(!fptr) {
        fprintf(stderr, "could not open file <%s>\n", filename);
        return EXIT_FAILURE;
      }

      have_file = 1;
    }
    else {  // file does not exist
      fptr = fopen(filename, "w");
      if(!fptr) {
        fprintf(stderr, "could not open file <%s>\n", filename);
        return EXIT_FAILURE;
      }
      ftruncate(fptr, length);
      fseek(fptr, 0, SEEK_SET);

      have_file = 0;
    }

    plain = plaintext_alloc(crypto_hash_BYTES);
    cipher = ciphertext_alloc(crypto_hash_BYTES);

    if(recv_e(sock, MSG_WAITALL, &plain, &cipher, n, key)) { puts("F4"); return EXIT_FAILURE; }

    char response;

    if(have_file) {
      char hash[crypto_hash_BYTES];
      char *buffer = malloc(length);

      fread(buffer, length, 1, fptr);
      crypto_hash(hash, buffer, length);

      if(!(strncmp(hash, plain.start, crypto_hash_BYTES))) {
        response = HAVE;
        printf("succeeded hash check\n");
      }
      else {
        response = GIVE;
        printf("failed hash check\n");
      }
    }
    else {
      response = GIVE;
    }

    plain = plaintext_alloc(1);
    cipher = ciphertext_alloc(1);

    plain.start[0] = response;

    send_e(sock, 0, &plain, &cipher, n, key);

    if(response == HAVE) {
      printf("had file!\n");
      return EXIT_SUCCESS;
    }
    
    plain = plaintext_alloc(length);
    cipher = ciphertext_alloc(length);
    
    if(recv_e(sock, MSG_WAITALL, &plain, &cipher, n, key)) { puts("F4"); return EXIT_FAILURE; }

    fseek(fptr, 0, SEEK_SET);
    fwrite(plain.start, plain.length, 1, fptr);
  }

  close(sock);
  
  return EXIT_SUCCESS;
}
