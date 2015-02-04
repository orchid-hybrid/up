// sizes of arguments are as follows:
int key_exchange(unsigned char *a_pk,   // crypto_box_PUBLICKEYBYTES
                 unsigned char *a_sk,   // crypto_box_SECRETKEYBYTES
                 unsigned char *b_pk,   // crypto_box_PUBLICKEYBYTES
                 unsigned char *key,    // padded, crypto_box_ZEROBYTES + crypto_secretbox_KEYBYTES
                 int mode,
                 int sock);