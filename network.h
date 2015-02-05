#define client_mode 0
#define server_mode 1

// returns 0 on success, -1 on failure
int start_networking(int mode, char *hostname, char *port, int *sock_out);

// returns 0 on success, -1 on failure
int sendall(int s, char *buf, int len);
