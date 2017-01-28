#include "macros.h"
#include <wchar.h>

extern char insert_edit_submode;       // insert or edit submode
extern WINDOW * main_win;
extern wchar_t inputline[BUFFERSIZE];
extern int inputline_pos;
extern int real_inputline_pos;
extern struct block * lastcmd_buffer;

void ins_in_line(wint_t d);
int is_single_command (struct block * buf, long timeout);
void insert_or_edit_cell();
void send_to_interp(wchar_t * oper);   // Send command to interpreter
void chg_mode(char strcmd);            // Change mode function
int modcheck();                        // Verify if open file has been modified
int savefile();                        // Save open file
struct ent;
void copyent(struct ent * n, struct ent * p, int dr, int dc, int r1, int c1, int r2, int c2, int special);
void flush_saved();
void insert_row(int after);
void insert_col(int after);
void deleterow();
void deletecol();
void formatcol(int c);
void del_selected_cells();
struct ent * lookat(int row, int col); // return pointer to 'ent' of cell. Create it if it doesn't exist
void cleanent(struct ent * p);         // Initialize 'ent' to zero. Won't free memory
void clearent(struct ent * v);         // free 'ent' memory.
int locked_cell(int r, int c);
int any_locked_cells(int r1, int c1, int r2, int c2);
void scroll_left (int n);
void scroll_right (int n);
void scroll_down(int n);
void scroll_up(int n);
struct ent * left_limit();
struct ent * right_limit();
struct ent * goto_top();
struct ent * goto_bottom();
struct ent * tick(char c);             // 'tick' ( ' ) command
struct ent * forw_row(int arg);
struct ent * back_row(int arg);
struct ent * forw_col(int arg);
struct ent * back_col(int arg);
struct ent * go_home();
struct ent * go_end();
struct ent * go_forward();
struct ent * go_backward();
struct ent * vert_top();
struct ent * vert_middle();
struct ent * vert_bottom();
struct ent * go_bol();
struct ent * go_eol();
struct ent * horiz_middle();
void select_inner_range(int * vir_tlrow, int * vir_tlcol, int * vir_brrow, int * vir_brcol);
void ljustify(int sr, int sc, int er, int ec);
void rjustify(int sr, int sc, int er, int ec);
void center(int sr, int sc, int er, int ec);
void doformat(int c1, int c2, int w, int p, int r);
struct enode;
int etype(register struct enode *e);
void erase_area(int sr, int sc, int er, int ec, int ignorelock, int mark_ent_as_deleted);
void auto_justify(int ci, int cf, int min);
void valueize_area(int sr, int sc, int er, int ec);
void sync_refs();
int fcopy();
int fsum();
int pad(int n, int r1, int c1, int r2, int c2);
void mark_ent_as_deleted(register struct ent * p);
