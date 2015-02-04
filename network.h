#define send_mode 0
#define listen_mode 1

int start_networking(int mode, char *hostname, char *port, int *sock_out);

int sendall(int s, char *buf, int len);

