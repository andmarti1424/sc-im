/*******************************************************************************
 * Copyright (c) 2013-2017, Andrés Martinelli <andmarti@gmail.com              *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of SC-IM                                                *
 *                                                                             *
 * SC-IM is a spreadsheet program that is based on SC. The original authors    *
 * of SC are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file dictionary.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief Dictionary implementation using malloc
 */

#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "dictionary.h"
#include "../macros.h"

/**
 * \brief TODO Document create_dictionary()
 *
 * \return dictionary
 */

 struct dictionary * create_dictionary() {
   struct dictionary * d = (struct dictionary *) malloc (sizeof (struct dictionary));
   d->len = 0;
   d->list = NULL;

   return d;
}

/**
 * \brief TODO Document put()
 *
 * \param[in] d
 * \param[in] k
 * \param[in] v
 *
 * \return none
 */

void put(struct dictionary * d, char * k, char * v) {
   if ( ! strlen (k) || ! strlen(v) ) return;
   //if ( ! strlen (k) ) return;

   struct nlist * nl;
   char * cl;

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
   } else if ( (cl = get(d, k)) != NULL && cl[0] != '\0' ) {
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

/**
 * \brief TODO Document destroy_dictionary()
 *
 * \param[in] d
 *
 * \return none
 */

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

/**
 * \brief TODO Document get_nl()
 *
 * \param[in] d
 * \param[in] key
 *
 * \return nl
 */

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

/**
 * \brief Get max length of keys in a dictionary
 *
 * \param[in] d
 *
 * \return count
 */

int get_maxkey_length(struct dictionary * d) {
   int i = 0, len, count = 0;
   if (d == NULL || d->list == NULL) return count;

   struct nlist * nl = d->list;
   while ( i++ < d->len ) {
       if ((len = strlen(nl->key)) > count) count = len;
       nl = nl->next;
   }
   return count;
}

/**
 * \brief Get max length of value of dictionary
 *
 * \param[in] d
 *
 * \return count
 */

int get_maxvalue_length(struct dictionary * d) {
   int i = 0, len, count = 0;
   if (d == NULL || d->list == NULL) return count;

   struct nlist * nl = d->list;
   while ( i++ < d->len ) {
       if ((len = strlen(nl->val)) > count) count = len;
       nl = nl->next;
   }
   return count;
}

/**
 * \brief Get the value for KEY
 *
 * \param[in] d
 * \param[in] key
 *
 * \return value for the key
 */

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


/**
 * \brief Save key/value pairs in D dictionary from a string STR
 *
 * \param[in] d
 * \param[in] str
 * \param[in] blank_space
 *
 * \return dictionary
 */

void parse_str(struct dictionary * d, char * str, int blank_space) {
    char c = str[0];
    char key[90];
    char value[90];
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
        while (c != '\0') {
            if (blank_space && c == ' ') break;
            add_char(value, c, strlen(value));
            c = *++str;
        }
        if (c != '\0') c = *++str;

        put(d, key, value);

        key[0] = '\0';
        value[0] = '\0';
    }
}
