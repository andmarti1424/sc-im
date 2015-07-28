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

   // se inserta primer elemento
   if (d->list == NULL) {
       nl = (struct nlist *) malloc(sizeof(struct nlist));
       nl->next = NULL;
       d->list = nl;

       d->len++;
       char * key = (char *) malloc(sizeof(char) * strlen(k)+1);
       key[0] = '\0';
       strcpy(key, k);
       nl->key = key;

   // no están permitidas las claves duplicadas.
   // si se intenta insertar una clave existente,
   // se sobreescribe su valor
   } else if ( get(d, k) != '\0' ) {
       nl = get_nl(d, k);
       free(nl->val);
   
   // si la clave no existe, se crea
   } else {
       nl = (struct nlist *) malloc(sizeof(struct nlist));

       // inserto al principio
       if (strcmp(k, d->list->key) < 0) {
           nl->next = d->list;
           d->list = nl; 
       // recorro e inserto en el medio o al final
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

   // para todos los casos, guardo el valor
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
   return nl; // por si d->list == NULL
}

// Obtener el valor de una clave
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

/* Obtener el nombre de una clave a partir de un valor
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


// Funcion que guarda en un diccionario claves y valores que son contenidos en un string
void parse_str(struct dictionary * d, char * str) {
    char c = str[0];
    char key[30];
    char value[30];
    key[0] = '\0';
    value[0] = '\0';

    // creo un diccionario para guardar las claves y valores que vinieron en el string
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
