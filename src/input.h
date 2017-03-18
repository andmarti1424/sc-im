#include <ncurses.h>
#include <wchar.h>
#include <sys/time.h>                                                 // for struct timeval

extern int multiplier;                                                // Multiplier
extern int command_pending;                                           // Command pending
extern WINDOW * input_win;
extern struct block * lastcmd_buffer;
#ifdef HISTORY_FILE
extern struct history * commandline_history;
#endif
#ifdef INS_HISTORY_FILE
extern struct history * insert_history;
#endif

void fix_timeout(struct timeval * start_tv);                          // Handle timeout of stdin
void handle_input(struct block * buffer);
void break_waitcmd_loop(struct block * buffer);
int has_cmd (struct block * buf, long timeout);
void handle_mult(int * multiplier, struct block * buf, long timeout); // Handle multiplier ef.
void exec_mult (struct block * buf, long timeout);
void exec_single_cmd (struct block * sb);
int wget_wch(WINDOW * win, wint_t * wch);
