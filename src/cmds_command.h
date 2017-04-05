#ifdef HISTORY_FILE
#include "history.h"
#endif
#include "buffer.h"

extern int shall_quit;
extern struct dictionary * user_conf_d;
extern struct history * commandline_history;

void do_commandmode(struct block * sb);
void ins_in_line(wint_t d);
