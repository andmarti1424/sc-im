#include "input.h"

void do_editmode(struct block * sb);
int start_edit_mode(struct block * buf, char type);

int for_word(int end_of_word, int delete, int big_word);
int look_for(char cb);
int back_word(int big_word);
int for_char();
int back_char();

void del_back_char();
void del_for_char();

int get_key();
