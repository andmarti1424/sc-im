#ifdef HISTORY_FILE
#include "history.h"
#endif
#include "buffer.h"

extern int shall_quit;

void do_commandmode(struct block * sb);

extern struct dictionary * user_conf_d;
extern struct history * commandline_history;
