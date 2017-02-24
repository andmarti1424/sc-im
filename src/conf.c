#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "conf.h"
#include "utils/dictionary.h"

void store_default_config_values() {
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
    put(user_conf_d, "ignorecase", "0");

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
