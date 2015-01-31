// A .conf file is a line based format
// each line contains a number of words

typedef struct line {
  int length;
  char **word;
} line;

typedef struct conf {
  int length;
  line *lines;
} conf;

// load_conf_file will return NULL on failure
// it will read a file and parse it into lines/words
conf *load_conf_file(char *filename);
