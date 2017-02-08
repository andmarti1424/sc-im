#include <wchar.h>

int wcswidth(const wchar_t * s, size_t n);
int wcwidth(const wchar_t wc);

int del_char(char * str, int posicion);
int del_wchar(wchar_t * str, int posicion);

int del_range_chars(char * str, int d, int h);
int del_range_wchars(wchar_t * str, int d, int h);

int add_char(char * str, char c, int posicion);
int add_wchar(wchar_t * str, wchar_t c, int posicion);

void subst(char * s, char from, char to);
int is_idchar (int d);
int str_in_str(char * s, char * b);
int wstr_in_wstr(wchar_t * s, wchar_t * b);
char * ltrim(char *string, char junk);
char * rtrim(char * string, char junk);
int isnumeric(char * string);
int clean_carrier(char * string);
char * xstrtok(char * line, char * delims);
int count_word_occurrences(char * s, char * word, int overlap);
char * str_replace ( const char * string, const char * substr, const char * replacement);
void uppercase(char * sPtr);
int sc_isprint(int d);
int count_width_widestring(const wchar_t * s, int p);
