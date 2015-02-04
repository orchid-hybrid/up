
// write to file returns 0 on success, -1 on failure
// it will overwrite a file if it already exists
int write_to_file(char *filename, unsigned char bytes[], size_t bytes_length);

// read from file returns 0 on success, -1 on failure
// it allocates a buffer in 'bytes' which you are expected to free
// the size of the file is written into 'bytes_length'
int read_from_file(char *filename, unsigned char **bytes, size_t *bytes_length);

void printhex(unsigned char *buf, int len);
