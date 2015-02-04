#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"

// This program lets you test making a TCP connection from one computer to another
// you can also just make a connection from your own computer to itself.
// to do that pick a four digit port number >1024
//
// run ./t7 listen 127.0.0.1 <PORT>
// and in another one ./t7 send 127.0.0.1 <PORT>
//
// if this works you could test it from one computer to another.
// to do that the listener/server side will have to tell its router to forward
// a port to your local ip. It might be listed in `ip addr` or some other command
// you will also need to find the external IP of the listener, to do that
// google: what is my ip
//
// run ./t7 listen <local-ip> <PORT>
// on the other computer run ./t7 send <external-ip> <PORT>
//
// useful root commands to debug this sort of thing on linux:
// * lsof -i
// * netstat -lptu
// * netstat -tulpn

#include <sys/types.h>
#include <sys/socket.h>

const char *usage = "Usage: ./t7 send hostname port\n"
                    "       ./t7 listen hostname port\n";

int main(int argc, char **argv) {
  int mode;
  char *hostname;
  
  int sock;
  
  if (argc != 1 + 3) {
    fprintf(stderr, usage);
    return EXIT_FAILURE;
  }
  
  if (!strcmp(argv[1], "send")) {
    mode = send_mode;
  } else if (!strcmp(argv[1], "listen")) {
    mode = listen_mode;
  } else {
    fprintf(stderr, usage);
    return EXIT_FAILURE;
  }

  hostname = argv[2];
  
  //if(sscanf(argv[3], "%d", &port) != 1) {
  //  puts("Could not parse port number");
  //  return EXIT_FAILURE;
  //}
  
  if(start_networking(mode, hostname, argv[3], &sock)) {
    puts("Could not network..");
    return EXIT_FAILURE;
  }

  if(mode == send_mode) {
    char buf[4+1] = { 0 };
    printf("send: %d\n", send(sock, "Beej was here!\n", 15, 0));
    recv(sock, &buf, 4, MSG_WAITALL);
    printf("> %s\n", buf);
    sleep(3);
  }
  else if(mode == listen_mode){
    char buf[15+1] = { 0 };
    recv(sock, &buf, 15, MSG_WAITALL);
    printf("> %s\n", buf);
    //while(recv(sock, &buf, 1, 0) != -1) { puts("."); }
    printf("send: %d\n", send(sock, "hi!\n", 4, 0));
    //recv(sock, &buf, 15, 0);
    sleep(3);
  }
  
  close(sock);
  
  return EXIT_SUCCESS;
}
