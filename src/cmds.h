#include "macros.h"

extern char insert_edit_submode;       // insert or edit submode
extern WINDOW * main_win;
extern char inputline[BUFFERSIZE];
extern int inputline_pos;
extern struct block * lastcmd_buffer;

void ins_in_line(int d);
int is_single_command (struct block * buf, long timeout);
void insert_or_edit_cell();
void send_to_interp(char * oper);      // Envio comando a interprete
void chg_mode(char strcmd);            // Funcion para cambio de modo
int modcheck();                        // Chequeo si el archivo abierto sufrio modificaciones
int savefile();                        // Grabo archivo abierto
struct ent;
void copyent(struct ent * n, struct ent * p, int dr, int dc, int r1, int c1, int r2, int c2, int special);
void flush_saved();
void insert_row(int after);
void insert_col(int after);
void deleterow();
void deletecol();
void formatcol(int c);
void del_selected_cells();
struct ent * lookat(int row, int col); // devuelvo puntero a un ent de una celda. se crea si no existe.
void cleanent(struct ent * p);         // pongo en cero contenido de ent. no libera memoria.
void clearent(struct ent * v);         // libero memoria de ent.
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
struct ent * tick(char c);             // Comando '
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
void erase_area(int sr, int sc, int er, int ec, int ignorelock);
void auto_justify(int ci, int cf, int min);
void valueize_area(int sr, int sc, int er, int ec);
void sync_refs();
