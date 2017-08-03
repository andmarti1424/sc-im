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
 * \file string.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a brief file description.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>   // for isdigit
#include <wctype.h>  // for iswprint
#include <wchar.h>
#include <stdio.h>
#include "string.h"
#include <curses.h>
#include "../sc.h"
#include "../macros.h"

/**
 * \brief Remove POSICION character of a cell (zero based)
 *
 * \param[in] str
 * \param[in] posicion
 *
 * \return 0 on success
 * \return -1 otherwise
 */

int del_char(char * str, int posicion) {
    int i, slen = strlen(str);

    if ( posicion >= slen || posicion < 0 ) return -1;
    for (i = posicion; i < slen - 1; i++) {
        str[i] = str[i + 1];
    }
    str[--slen] = '\0';
    return 0;
}

/**
 * \brief Remove POSICION character of a cell (zero based) (Wide char version)
 *
 * \param[in] srt
 * \param[in] posicion
 *
 * \return 0 on success
 * \return -1 otherwise
 */

int del_wchar(wchar_t * str, int posicion) {
    int i, slen = wcslen(str);

    if ( posicion >= slen || posicion < 0 ) return -1;
    for (i = posicion; i < slen - 1; i++) {
        str[i] = str[i + 1];
    }
    str[--slen] = '\0';
    return 0;
}

/**
 * \brief Remove D to H characters range of a cell
 *
 * \param[in] str
 * \param[in] d
 * \param[in] h
 *
 * \return 0 on success
 * \return -1 otherwise
 */

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

/**
 * \brief Remove D to H characters range of a cell. Wide char version.
 *
 * \param[in] str
 * \param[in] d
 * \param[in] h
 *
 * \return 0 on success
 * \return -1 otherwise
 */

int del_range_wchars(wchar_t * str, int d, int h) {
    int i = 0, j = 0, slen = wcslen(str);

    if ( h >= slen || h < d ) return -1;

    for (j = 0; j <= (h-d); j++)
    for (i = d; i < slen - 1; i++) {
        str[i] = str[i+1];
    }
    str[slen - (h - d) - 1] = L'\0';
    return 0;
}

/**
 * \brief Add a C character to a cell in POSICION.
 *
 * \details Wide char version. STR should be
 * previously allocated with enough memory.
 *
 * \param[in] str
 * \param[in] c
 * \param[in] posicion
 *
 * \return 0 on success
 * \return -1 otherwise
 */

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

/**
 * \brief Add a C character to a cell in POSICION. Wade char version.
 *
 * \details STR should be previously allocated with enough memory.
 *
 * \param[in] str
 * \param[in] c
 * \param[in] posicion
 *
 * \return 0 on success
 * \return -1 otherwise
 */

int add_wchar(wchar_t * str, wchar_t c, int posicion) {
    int slen = wcslen(str);
    int len = slen - posicion;
    if (posicion > slen) return -1;
    while (len) {
        str[posicion + len] = str[posicion + len -1];
        len--;
    }
    str[++slen] = L'\0';
    str[posicion] = c;
    return 0;
}

/**
 * \brief Replace all matches FROM character TO character
 *
 * \param[in] s
 * \param[in] from
 * \param[in] to
 *
 * \return none
 */

void subst(char * s, char from, char to) {
    while (*s == from) *s++ = to;
    return;
}

/**
 * \brief Rind string B inside string S
 *
 * \param[in]
 * \param[in[ b
 *
 * \return S position in B
 * \return -1 otherwise
 */

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

/**
 * \brief Find string B inside string S. Wide char version.
 *
 * \param[in] s
 * \param[in] b
 *
 * \return S position in B
 * \return -1 otherwise
 */

int wstr_in_wstr(wchar_t * s, wchar_t * b) {
    int slen = wcslen(s);
    int blen = wcslen(b);

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

/**
 * \brief Returns 1 if special control character is found
 *
 * \param[in] d
 *
 * \return 1 if special control character is found
 */

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

/**
 * \brief TODO Document this function
 *
 * \param[in] string
 * \param[in] junk
 *
 * \return string
 */

char * rtrim(char * string, char junk) {
    char * original = string + strlen(string);
    while(*--original == junk);
    *(original + 1) = '\0';
    return string;
}

/**
 * \brief TODO Document this function
 *
 * \param[in] string
 * \param[in] junk
 *
 * \return TODO returns
 */

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

/**
 * \brief Tells if a string represents a numeric value
 *
 * \param[in] string
 *
 * \return 1 if string represents a numeric value
 * \return 0 otherwise
 */

int isnumeric(char * string) {
    int i, len = strlen(string);
    int res = true;
    bool has_dot = false;
    bool has_dash = false;
    bool has_digit = false;
    for (i=0; i<len; i++) {

        if ( string[i] == '.' && ! has_dot ) {
            has_dot = true;
            continue;
        }

        if ( string[i] == '-' ) {
            if (has_digit) {
                res = false;
                break;
            }
            if (! has_dash) {
                has_dash = true;
            } else {
                res = false;
                break;
            }
            continue;
        }

        if ( isdigit(string[i]) ) {
            has_digit = true;
            continue;
        }

        res = false;
        break;
    }
    return res;
}

/**
 * \brief Clean \r and \n from a string
 *
 * \param[in] string
 *
 * \return 1 of changes were made
 * \return 0 otherwise
 */

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

/**
 * \brief strtok version that handles null fields
 *
 * \param[in] line
 * \param[in] delims
 *
 * \return TODO returns
 */

char * xstrtok(char * line, char * delims) {
    static char * saveline = NULL;
    char * p;
    int n;

    if (line != NULL)
       saveline = line;

 // see if we have reached the end of the line
    if (saveline == NULL || *saveline == '\0')
       return(NULL);
 // return the number of characters that aren't delims
    n = strcspn(saveline, delims);
    p = saveline; // save start of this token

    saveline += n; // bump past the delim

    if (*saveline != '\0') // trash the delim if necessary
       *saveline++ = '\0';

    return p;
}

/**
 * \brief Change the number of occurences of a word in s. Not used.
 *
 * \param[in] s
 * \param[in] word
 * \param[in] overlap
 *
 * \return c
 */

int count_word_occurrences(char * s, char * word, int overlap) {
    int c = 0, l = strlen(word);

    while (*s != '\0') {
        if (strncmp(s++, word, l)) continue;
        if (!overlap) s += l - 1;
        c++;
    }
    return c;
}

/**
 * \brief Search substr word inside string and replace all its occurances with replacement
 *
 * \param[in] string
 * \param[in] substr
 * \param[in] replacement
 *
 * \return resulting string
 */

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

/**
 * \brief TODO Document uppercase()
 *
 * \param[in] sPtr
 *
 * \return none
 */

void uppercase(char * sPtr) {
    for(; *sPtr != '\0'; ++sPtr)
         *sPtr = toupper(*sPtr);
}

/**
 * \brief TODO Document sc+isprint()
 *
 * \param[in] d
 *
 * \return TODO returns
 */

int sc_isprint(int d) {
    if ( ((d > 31) && (d < 127)) || ((d > 127) && (d < 255)) || iswprint(d) )
        return 1;
    return 0;
}

/**
 * \brief Return the number of wide chars of wchar_t * s string. Needed to 
 * fill p column positions.
 *
 * \param[in] s
 * \param[in] p
 *
 * \return TODO returns
 */

int count_width_widestring(const wchar_t * s, int p) {
    int n;
    int c_width = 0;

    for (n=0; n<wcslen(s); n++) {
        c_width += wcwidth( s[n] );
        if (c_width > p) break;
    }
    return n;
}
