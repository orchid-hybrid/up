#include <stdio.h>
#include <stdlib.h>

#define server_mode 0
#define client_mode 1

#define push_mode 0
#define pull_mode 1

void usage() {
  puts(
       "USAGE: set up ~/.config/up/addressbook.conf and then\n"
       "\n"
       "Alice running a server, sending a file to Bob:\n"
       " alice$ ./up server push bob foo.txt\n"
       " bobby$ ./up client pull alice\n"
       "\n"
       "Alice running a server, receiving a file from Bob:\n"
       " alice$ ./up server pull bob\n"
       " bobby$ ./up client push alice foo.txt\n"
       );
}

int main(int argc, char **argv) {
  int network_mode;
  int send_mode;
  char *contact;
  char *filename;

  conf *c;

  char **self_entry;
  char **contact_entry;
  
  if(argc != 1 + 3 && argc != 1 + 4) {
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
  
  contact = argv[3];

  if(send_mode == push_mode) {
    if(argc != 1 + 4) {
      usage();
      return EXIT_FAILURE;
    }

    filename = argv[4];
  }
  else if(send_mode == pull_mode) {
    if(argc != 1 + 3) {
      usage();
      return EXIT_FAILURE;
    }
  }
  
  c = load_conf_file("addressbook.conf");
  
  self_entry = lookup_addressbook_self(c);
  contact_entry = lookup_addressbook_contact(c, contact);
  
  return EXIT_SUCCESS;
}
