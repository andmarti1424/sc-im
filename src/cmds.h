#include "macros.h"

extern char insert_edit_submode;       // insert or edit submode
extern WINDOW * main_win;

void ins_in_line(int d);
extern char inputline[BUFFERSIZE];
extern int inputline_pos;

int is_single_command (struct block * buf, long timeout);

extern struct block * lastcmd_buffer;

void insert_or_edit_cell();

void send_to_interp(char * oper); // Envio comando a interprete
void chg_mode(char strcmd);       // Funcion para cambio de modo
int modcheck();                   // Chequeo si el archivo abierto sufrio modificaciones
int savefile();                   // Grabo archivo abierto

void insert_row(int after);
void insert_col(int after);
void delete_row();
void delete_col();

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
struct ent * tick(char c);         // Comando '
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
