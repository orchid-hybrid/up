#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>

#include "network.h"

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

      close(sock);
      return -1;
    }

    if(mode == send_mode) {
      if (connect(sock, p->ai_addr, p->ai_addrlen)) {
	    perror("connect");

        close(sock);
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

      close(sock);
	  return -1;
	}
	
	// this socket is for the new connection
	// p->ai_addr will now hold information about the host that made the connection
	if ((conn = accept(sock, NULL, NULL)) == -1) {
	  perror("accept");

      close(sock);
	  return -1;
	}
	
	*sock_out = conn;
	return 0;
      }
    }
  }

  close(sock);
  return -1;
}
