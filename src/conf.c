#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "conf.h"
#include "utils/dictionary.h"

char * config_path() {
  char * dirname = getenv("XDG_CONFIG_HOME");
  if (dirname == NULL) {
    dirname = getenv("HOME");
    char * new_dirname = malloc(strlen(dirname) + strlen(DEFAULT_CONFIG_DIR) + 1);
    strcpy(new_dirname, dirname);
    strcat(new_dirname, DEFAULT_CONFIG_DIR);
    dirname = new_dirname;
  }
  char * path = malloc(strlen(dirname) + strlen(CONFIG_LOCATION) + 1);
  strcpy(path, dirname);
  strcat(path, CONFIG_LOCATION);
  return path;
}

enum {
  PARSE_KEY = 0,
  PARSE_EQUAL = 1,
  PARSE_VALUE = 2
};

static int translate_color(const char * value) {
  if (strcmp(value, "BLACK") == 0 || strcmp(value, "0") == 0) {
    return 0;
  } else if (strcmp(value, "RED") == 0 || strcmp(value, "1") == 0) {
    return 1;
  } else if (strcmp(value, "GREEN") == 0 || strcmp(value, "2") == 0) {
    return 2;
  } else if (strcmp(value, "YELLOW") == 0 || strcmp(value, "3") == 0) {
    return 3;
  } else if (strcmp(value, "BLUE") == 0 || strcmp(value, "4") == 0) {
    return 4;
  } else if (strcmp(value, "MAGENTA") == 0 || strcmp(value, "5") == 0) {
    return 5;
  } else if (strcmp(value, "CYAN") == 0 || strcmp(value, "6") == 0) {
    return 6;
  } else if (strcmp(value, "WHITE") == 0 || strcmp(value, "7") == 0) {
    return 7;
  } else {
    return -1;
  }
}

static void parse_line(const char * line, char ** key, char ** value) {
  free(*key);
  free(*value);
  const size_t len = strlen(line);
  size_t i = 0;
  int state = PARSE_KEY;
  size_t key_end;
  size_t value_start, value_end;

  while (i < len) {
    const char c = line[i];
    switch (state) {
    case PARSE_KEY:
      if (c == ' ' || c == '=') {
        state = PARSE_EQUAL;
        key_end = i;
      }
      break;
    case PARSE_EQUAL:
      if (c != ' ' && c != '=') {
        state = PARSE_VALUE;
        value_start = i;
      }
      break;
    case PARSE_VALUE:
      /*if (c < 33 || c > 126) {*/
        /*++i;*/
      /*}*/
      break;
    }
    ++i;
  }

  value_end = i;

  *key = malloc(key_end);
  strncpy(*key, line, key_end);
  (*key)[key_end] = '\0';

  size_t value_len = value_end - value_start;
  *value = malloc(value_len + 1);
  strncpy(*value, line + value_start, value_len - 1);
  (*value)[value_len - 1] = '\0';
}

void load_config_file() {
  char * file_path = config_path();

  FILE * fp;
  char * line = NULL;
  size_t len = 0;

  fp = fopen(file_path, "r");
  if (!fp) {
    // Exit early as we can't read a nonexistent config file
    return;
  }
  free(file_path);

  char * key = NULL;
  char * value = NULL;
  while (getline(&line, &len, fp) != -1) {
    if (strlen(line) <= 1 || line[0] == COMMENT_CHAR) {
      continue;
    }
    parse_line(line, &key, &value);

    put(user_conf_d, key, value);
  }

  free(key);
  free(value);
  free(line);
}

const int get_config_color(char * key, const int default_color) {
   if (!user_conf_d) {
     return default_color;
   }

   char * val = get(user_conf_d, key);
   if (!val || strcmp(val, "\n") == 0) {
     return default_color;
   }
   int color = translate_color(val);
   if (color < 0) {
     return default_color;
   }
   return color;
}

void setup_user_conf() {
    put(user_conf_d, "half_page_scroll", "0");
    put(user_conf_d, "autocalc", "1");
    put(user_conf_d, "numeric", "0");
    put(user_conf_d, "nocurses", "0");
    put(user_conf_d, "newline_action", "0");
    put(user_conf_d, "external_functions", "0");
    put(user_conf_d, "xlsx_readformulas", "0");
    put(user_conf_d, "quit_afterload", "0");
    put(user_conf_d, "numeric_zero", "0");
    put(user_conf_d, "numeric_decimal", "0");
    put(user_conf_d, "overlap", "0");
    put(user_conf_d, "debug", "0");

    // we calc get gmtoffset
    #ifdef USELOCALE
    time_t t = time(NULL);
    struct tm * lt = localtime(&t);
    char strgmtoff[7];
    sprintf(strgmtoff, "%ld", lt->tm_gmtoff);
    put(user_conf_d, "tm_gmtoff", strgmtoff);
    #else
    put(user_conf_d, "tm_gmtoff", "0");
    #endif

    load_config_file();
}

char * get_conf_values(char * salida) {
   if (user_conf_d == NULL) return NULL;
   struct nlist * nl;

   nl = user_conf_d->list;
   salida[0]='\0';
   while (nl != NULL) {
       sprintf(salida + strlen(salida), "%s=%s\n", nl->key, nl->val);
       nl = nl->next;
   }
   return salida;
}

char * get_conf_value(char * key) {
   char * val = get(user_conf_d, key);

   if ( val != '\0' )
       return val;
   else
       return get(predefined_conf_d, key);
}
