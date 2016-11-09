// Dictionary implementation using malloc

#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "dictionary.h"

struct dictionary * create_dictionary() {
   struct dictionary * d = (struct dictionary *) malloc (sizeof (struct dictionary));
   d->len = 0;
   d->list = NULL;

   return d;
}

void put(struct dictionary * d, char * k, char * v) {
   if ( ! strlen (k) || ! strlen(v) ) return;
   //if ( ! strlen (k) ) return;

   struct nlist * nl;

   // Insert the first element
   if (d->list == NULL) {
       nl = (struct nlist *) malloc(sizeof(struct nlist));
       nl->next = NULL;
       d->list = nl;

       d->len++;
       char * key = (char *) malloc(sizeof(char) * strlen(k)+1);
       key[0] = '\0';
       strcpy(key, k);
       nl->key = key;

   // Duplicated keys are not allowed.
   // If an existent key is inserted, the value is overwritten.
   } else if ( get(d, k) != '\0' ) {
       nl = get_nl(d, k);
       free(nl->val);

   // If the key doesn't exists, Create it.
   } else {
       nl = (struct nlist *) malloc(sizeof(struct nlist));

       // Insert at the beginning
       if (strcmp(k, d->list->key) < 0) {
           nl->next = d->list;
           d->list = nl; 
       // Traverse and insert in the middle or at the end
       } else {
           struct nlist * nant = d->list;
           struct nlist * nact = d->list->next;
           while ( nact != NULL && strcmp(k, nact->key) > 0 ) {
               nant = nact;
               nact = nact->next;
           }
           nl->next = nact;
           nant->next = nl;
       }

       d->len++;
       char * key = (char *) malloc(sizeof(char) * strlen(k)+1);
       key[0] = '\0';
       strcpy(key, k);
       nl->key = key;
   }

   // Always save the value
   char * val = (char *) malloc(sizeof(char) * strlen(v)+1);
   val[0] = '\0';
   strcpy(val, v);
   nl->val = val;
   return;
}

void destroy_dictionary(struct dictionary * d) {
   //if (d == NULL) return;
   struct nlist * nl;
   struct nlist * n_next;

   nl = d->list;
   while (nl != NULL) {
       n_next = nl->next;
       free(nl->key);
       free(nl->val);
       free(nl);
       nl = n_next;
   }

   free(d);
   return;
}

struct nlist * get_nl(struct dictionary * d, char * key) {
   int i=0;
   struct nlist * nl = d->list;
   while ( i++ < d->len && strcmp(key, nl->key) >= 0 ) {
       if (strcmp(nl->key, key) == 0)
           return nl;
       nl = nl->next;
   }
   return nl; // just in case d->list == NULL
}

// Get the value for KEY
char * get(struct dictionary * d, char * key) {
   int i=0;
   if (d == NULL || d->list == NULL) return NULL;

   struct nlist * nl = d->list;
   while ( i++ < d->len ) { // && strcmp(key, nl->key) >= 0 ) {
       if (strcmp(nl->key, key) == 0)
           return nl->val;
       nl = nl->next;
   }
   return NULL;
}

/* Get the key name from a value
char * get_key_name(struct dictionary * d, char * value) {
   int i=0;
   if (d == NULL || d->list == NULL) return NULL;
   struct nlist * nl = d->list;
   while ( i++ < d->len ) {
       if (! strcmp(nl->val, value))
           return nl->key;
       nl = nl->next;
   }
   return NULL;
}
*/


// Save key/value pairs in D dictionary from a string STR
void parse_str(struct dictionary * d, char * str) {
    char c = str[0];
    char key[30];
    char value[30];
    key[0] = '\0';
    value[0] = '\0';

    // Create the dictionary
    while (c != '\0') {
        while (c != '=' && c != '\0') {
            add_char(key, c, strlen(key));
            c = *++str;
        }
        if (c == '\0') break;
        c = *++str;
        while (c != ' ' && c != '\0') {
            add_char(value, c, strlen(value));
            c = *++str;
        }
        if (c != '\0') c = *++str;

        put(d, key, value);

        key[0] = '\0';
        value[0] = '\0';
    }
}
