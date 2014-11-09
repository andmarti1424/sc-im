#include "sc.h"

extern struct ent * yanklist;

void init_yanklist();
struct ent * get_yanklist();
int count_yank_ents();
void free_yanklist ();

void add_ent_to_yanklist(struct ent * item);
void yank_area(int tlrow, int tlcol, int brrow, int brcol, char c, int arg);
void paste_yanked_ents();
