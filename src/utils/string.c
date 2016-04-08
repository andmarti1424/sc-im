#include <stdlib.h>
#include <string.h>
#include <ctype.h>   // for isdigit
#include <stdio.h>
#include "string.h"
#include <curses.h>
#include "../sc.h"
#include "../macros.h"

// Remove POSICION character of a cell (zero based)
// returns 0 on success, -1 otherwise
int del_char(char * str, int posicion) {
    int i, slen = strlen(str);

    if ( posicion >= slen || posicion < 0 ) return -1;
    for (i = posicion; i < slen - 1; i++) {
        str[i] = str[i + 1];
    }
    str[--slen] = '\0';
    return 0;
}

// Remove D to H characters range of a cell
// returns 0 on success, -1 otherwise
int del_range_chars(char * str, int d, int h) {
    int i = 0, j = 0, slen = strlen(str);

    if ( h >= slen || h < d ) return -1;

    for (j = 0; j <= (h-d); j++)
    for (i = d; i < slen - 1; i++) {
        str[i] = str[i+1];
    }
    str[slen - (h - d) - 1] = '\0';
    return 0;
}

// Add a C character to a cell in POSICION
// STR should be previously allocated with enough memory
// returns 0 on success, -1 otherwise
int add_char(char * str, char c, int posicion) {
    int slen = strlen(str);
    int len = slen - posicion;
    if (posicion > slen) return -1;
    while (len) {
        str[posicion + len] = str[posicion + len -1];
        len--;
    }
    str[++slen] = '\0';
    str[posicion] = c;
    return 0;
}

// Replace all matches FROM character TO character
void subst(char * s, char from, char to) {
    while (*s == from) *s++ = to;
    return;
}

// Find string B inside string S
// returns S position in B , -1 otherwise
int str_in_str(char * s, char * b) {
    int slen = strlen(s);
    int blen = strlen(b);

    if (! slen || ! blen) return -1;

    int e = 0;
    int i;

    while ( e <= slen-blen ) {
        for (i=0; i<blen; i++) {
            if (s[e+i] != b[i]) {
                break;  
            }
        }
        if (i >= blen) return e;
        else e++;
    }
    return -1;
}

// Returns 1 if a special or control character is found
int is_idchar (int d) {
    switch (d) {
        case OKEY_LEFT:
        case OKEY_RIGHT:
        case OKEY_DOWN:
        case OKEY_UP:
        case OKEY_TAB:
        case OKEY_BS:
        case OKEY_HOME:
        case OKEY_END:
        case OKEY_PGUP:
        case OKEY_PGDOWN:
        case OKEY_DEL:
        //case OKEY_ENTER:
        case OKEY_ESC:
           return 1;
    }
    return 0;
}

char ** split(char *string, const char delimiter, int lastnull) {
    int length = 0, count = 0, i = 0, j = 0;
    while(*(string++)) {
        if (*string == delimiter) count++;
        length++;
    }
    string -= (length + 1); // string was incremented one more than length
    char **array = (char **)malloc(sizeof(char *) * (length + 1 + lastnull));
    char ** base = array;
    for(i = 0; i < (count + 1); i++) {
        j = 0;
        while(string[j] != delimiter) j++;
        j++;
        * array = (char *) malloc(sizeof(char) * j);
        memcpy(*array, string, (j-1));
        (*array)[j-1] = '\0';
        string += j;
        array++;
    }
    if (lastnull) {
        *array = NULL;
        array++;
    }
    *array = '\0';
    return base;  
}

char * rtrim(char * string, char junk) {
    char * original = string + strlen(string);
    while(*--original == junk);
    *(original + 1) = '\0';
    return string;
}

char * ltrim(char * string, char junk) {
    char * original = string;
    char * p = original;
    int trimmed = 0;
    do {
        if (*original != junk || trimmed) {
            trimmed = 1;
            *p++ = *original;
        }
    } while (*original++ != '\0');
    return string;
}

// this function tells is a string represents a numeric value
// returns 1 if that is the case. 0 otherwise
int isnumeric(char * string) {
    int i, len = strlen(string);
    int res = 1;
    for (i=0; i<len; i++) {
        if ( string[i] != '.' && string[i] != '-' && ! isdigit(string[i]) ) {
            res = 0;
            break;
        }
    }
    return res;
}

// clean \r and \n from a string
// returns 1 if changes were made
int clean_carrier(char * string) {
    int i, changes = 0, len = strlen(string);
    for (i=0; i<len; i++) {
        if ( string[i] == '\r' || string[i] == '\n' ) {
            del_char(string, i);
            len--;
            changes = 1;
        }
    }
    return changes;
}

/*
 *  * strtok version that handles null fields
 *  */
char * xstrtok(char * line, char * delims) {
    static char * saveline = NULL;
    char * p;
    int n;
 
    if (line != NULL)
       saveline = line;

/*
 * *see if we have reached the end of the line
 * */
    if (saveline == NULL || *saveline == '\0')
       return(NULL);
/*
 * *return the number of characters that aren't delims
 * */
    n = strcspn(saveline, delims);
    p = saveline; // save start of this token

    saveline += n; // bump past the delim

    if (*saveline != '\0') // trash the delim if necessary
       *saveline++ = '\0';

    return p;
}

// Count number of occurences of word in s
// Not used
int count_word_occurrences(char * s, char * word, int overlap) {
    int c = 0, l = strlen(word);

    while (*s != '\0') {
        if (strncmp(s++, word, l)) continue;
        if (!overlap) s += l - 1;
        c++;
    }
    return c;
}

// Search substr word inside string and replace all its occurrences with replacement
// it returns the resulting string
char * str_replace ( const char * string, const char * substr, const char * replacement){
    char * tok = NULL;
    char * newstr = NULL;
    char * oldstr = NULL;
    char * head = NULL;
     
    /* if either substr or replacement is NULL, duplicate string a let caller handle it */
    if ( substr == NULL || replacement == NULL )
        return strdup (string);
    newstr = strdup (string);
    head = newstr;
    while ( (tok = strstr ( head, substr ))) {
        oldstr = newstr;
        newstr = malloc ( strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 1 );
        /* failed to alloc mem, free old string and return NULL */
        if ( newstr == NULL ){
            free (oldstr);
            return NULL;
        }
        memcpy ( newstr, oldstr, tok - oldstr );
        memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
        memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
        memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) , 0, 1 );
        /* move back head right after the last replacement */
        head = newstr + (tok - oldstr) + strlen( replacement );
        free (oldstr);
    }
    return newstr;
}

void uppercase(char * sPtr) {
    for(; *sPtr != '\0'; ++sPtr)
         *sPtr = toupper(*sPtr);
}

int sc_isprint(int d) {
    if ( ((d > 31) && (d < 127)) || ((d > 127) && (d < 255)) )
        return 1;
    return 0;
}
