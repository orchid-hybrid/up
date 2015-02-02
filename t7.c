#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>

#include "utilities.h"
#include "conf.h"

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

#define send_mode 0
#define listen_mode 1

const char *usage = "Usage: ./t7 send hostname port\n"
                    "       ./t7 listen hostname port\n";

int start_networking(int mode, char *hostname, char *port, int *sock_out) {
  int sock, conn;
  int status;

  unsigned char buffer[512];
  int *addrlen[sizeof(struct sockaddr_storage)];

  struct addrinfo hints;
  struct addrinfo *servinfo, *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname,
		       port,
		       &hints,
		       &servinfo);

  for (p = servinfo; p != NULL; p = p->ai_next) {
    puts("tick");
    if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1 ) {
      perror("socket");
      return -1;
    }

    if(mode == send_mode) {
      if (connect(sock, p->ai_addr, p->ai_addrlen)) {
	perror("connect");
	return -1;
      }
      
      *sock_out = sock;
      return 0;
    }
    else if (mode == listen_mode) {
      if (!bind(sock, p->ai_addr, p->ai_addrlen)) {
	// should now listen until it gets a connection

	// this listens for a new connection
	puts("waiting for a connection...");
	if (listen(sock, 0)) {
	  perror("listen");
	  fprintf(stderr, "listen() call failed\n");
	  return -1;
	}
	
	// this socket is for the new connection
	// p->ai_addr will now hold information about the host that made the connection
	if ((conn = accept(sock, NULL, NULL)) == -1) {
	  perror("accept");
	  fprintf(stderr, "accept() call failed: error %d\n", errno);
	  //fprintf(stderr, "EAGAIN: %d\n EWOULDBLOCK: %d\n EBADF: %d\n ECONNABORTED: %d\n EINTR: %d\n EINVAL: %d\n EMFILE: %d\n ENFILE: %d\n ENOBUFS: %d\n ENOMEM: %d\n ENOTSOCK: %d\n EOPNOTSUPP: %d\n EPROTO", EAGAIN, EWOULDBLOCK, EBADF, ECONNABORTED, EINTR, EINVAL, EMFILE, ENFILE, ENOBUFS, ENOMEM, ENOTSOCK, EOPNOTSUPP, EPROTO);
	  return -1;
	}
	
	*sock_out = sock;
	return 0;
      }
    }
  }

  return -1;
}

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
  
  return EXIT_SUCCESS;
}
