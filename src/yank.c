// yanklist no guarda referencias a ents, sino que se crean nodos nuevos.
// constantemente se va limpiando la yanklist.
// ej.: al borrar ents con dc, los originales se borran y se crean nuevos en la yanklist.
// NOTA importante: cada ent debe guardar dentro suyo su correspondiente row y col

#include "sc.h"
#include "stdlib.h"
#include "marks.h"
#include "cmds.h"
#include "xmalloc.h" // for scxfree

#ifdef UNDO
#include "undo.h"
#endif

extern struct ent * forw_row(int arg);
extern struct ent * back_row(int arg);
extern struct ent * forw_col(int arg);
extern struct ent * back_col(int arg);

int yank_arg;                 // cantidad de filas o columnas copiadas. usado por ej en '4yr'
char type_of_yank;            // tipo de copia. c = col, r = row, a = range, e = cell, '\0' = no se ha realizado yank.
static struct ent * yanklist; // variable estatica

void init_yanklist() {
    type_of_yank = '\0';
    yanklist = NULL;
}

struct ent * get_yanklist() {
    return yanklist;
}

// Funcion que elimina todos los yank ents guardados y libera la memoria asignada
void free_yanklist () {
    if (yanklist == NULL) return;
    int c;
    struct ent * r = yanklist;
    struct ent * e;
    while (r != NULL) {
        e = r->next;

        if (r->format) scxfree(r->format);
        if (r->label) scxfree(r->label);
        if (r->expr) efree(r->expr);

        free(r);
        r = e;
    }

    for (c = 0; c < COLFORMATS; c++)
        if (colformat[c]) {
            scxfree(colformat[c]);
            colformat[c] = NULL;
        }
           
    yanklist = NULL;
    return;
}

// Funcion que cuenta la cantidad de elementos en la yanklist
int count_yank_ents() {
    int i = 0;

    struct ent * r = yanklist;
    while (r != NULL) {
        i++;
        r = r->next;
    }    
    return i;
}

// Funcion agrega un ent a la lista yanklist
void add_ent_to_yanklist(struct ent * item) {
    // creo e inicializo ent
    struct ent * i_ent = (struct ent *) malloc (sizeof(struct ent));
    (i_ent)->label = (char *)0;
    (i_ent)->row = 0;
    (i_ent)->col = 0;
    (i_ent)->flags = may_sync;
    (i_ent)->expr = (struct enode *)0;
    (i_ent)->v = (double) 0.0;
    (i_ent)->format = (char *)0;
    (i_ent)->cellerror = CELLOK;
    (i_ent)->next = NULL;

    // copio contenido de item en i_ent
    (void) copyent(i_ent, item, 0, 0, 0, 0, 0, 0, 0);

    (i_ent)->row = item->row;
    (i_ent)->col = item->col;

    // si la lista esta vacia lo inserto al comienzo
    if (yanklist == NULL) {
        yanklist = i_ent;
        return;
    }
 
    // si la lista no esta vacia, lo inserto al final
    struct ent * r = yanklist;
    struct ent * ant;
    while (r != NULL) {
        ant = r;
        r = r->next;
    }
    ant->next = i_ent;
    return;
}

// yank a range of ents
// arg: cantidad de filas o columnas copiadas. usado por ej en '4yr'
// type: tipo de copia. c = col, r = row, a = range, e = cell, '\0' = no se ha realizado yank.
// this two args are used for pasting..
void yank_area(int tlrow, int tlcol, int brrow, int brcol, char type, int arg) {
    int r,c;
    free_yanklist();
    type_of_yank = type;
    yank_arg = arg;

    for (r = tlrow; r <= brrow; r++)
        for (c = tlcol; c <= brcol; c++) {
            struct ent * elm = *ATBL(tbl, r, c);

            // Importante: Cada ent guarda dentro suyo su correspondiente row y col
            if (elm != NULL) add_ent_to_yanklist(elm);
        }
    return;
}

// paste yanked ents:
// this function is used for paste ents that were yanked with yr yc dr dc..
// it is also used for sorting.
// if above == 1, paste is done above current row or to the right of current col.
// ents that were yanked using yy or yanked ents of a range, are always pasted in currow and curcol positions.
// diffr es la diferencia de filas entre la posicion actual y el ent copiado.
// diffc es la diferencia de columnas entre la posicion actual y el ent copiado.
// cuando se hace sort, los valores de row y col pueden variar desde el momento de copia al momento de pegado.
// por tal razón, para el sort, el valor de diffr debe ser cero.
// Cuando se implemente el ordenamiento de columnas, en vez de por filas, diffc también deberá ser cero!
void paste_yanked_ents(int above) {
    if (! count_yank_ents()) return;

    struct ent * yl = yanklist;
    int diffr = 0, diffc = 0, ignorelock = 0;

    #ifdef UNDO
    create_undo_action();
    #endif

    if (type_of_yank == 's') {                               // paste a range that was yanked in the sort function
        diffr = 0;
        diffc = curcol - yl->col;
        ignorelock = 1;

    } else if (type_of_yank == 'a' || type_of_yank == 'e') { // paste cell or range
        diffr = currow - yl->row;
        diffc = curcol - yl->col;

    } else if (type_of_yank == 'r') {                        // paste row
        int c = yank_arg;
        #ifdef UNDO
        copy_to_undostruct(currow + ! above, 0, currow + ! above - 1 + yank_arg, maxcol, 'd');
        #endif
        while (c--) above ? insert_row(0) : insert_row(1);
        if (! above) currow = forw_row(1)->row;              // paste below
        diffr = currow - yl->row;
        diffc = yl->col;
        fix_marks(yank_arg, 0, currow, maxrow, 0, maxcol);
        #ifdef UNDO
        save_undo_range_shift(yank_arg, 0, currow, 0, currow - 1 + yank_arg, maxcol);
        #endif

    } else if (type_of_yank == 'c') {                        // paste col
        int c = yank_arg;
        #ifdef UNDO
        copy_to_undostruct(0, curcol + above, maxrow, curcol + above - 1 + yank_arg, 'd');
        #endif
        while (c--) above ? insert_col(1) : insert_col(0);   // insert cols to the right if above or to the left
        //if (above) curcol = back_col(1)->col; NO
        diffr = yl->row;
        diffc = curcol - yl->col;
        fix_marks(0, yank_arg, 0, maxrow, curcol, maxcol);
        #ifdef UNDO
        save_undo_range_shift(0, yank_arg, 0, curcol, maxrow, curcol - 1 + yank_arg);
        #endif
    }

    // por cada ent en yanklist
    while (yl != NULL) {
        #ifdef UNDO
        copy_to_undostruct(yl->row + diffr, yl->col + diffc, yl->row + diffr, yl->col + diffc, 'd');
        #endif

        // here we delete current content of "destino" ent
        erase_area(yl->row + diffr, yl->col + diffc, yl->row + diffr, yl->col + diffc, ignorelock);
        /*struct ent **pp = ATBL(tbl, yl->row + diffr, yl->col + diffc);
        if (*pp && (!((*pp)->flags & is_locked) )) {
            mark_ent_as_deleted(*pp);
            *pp = NULL;
        }*/

        struct ent * destino = lookat(yl->row + diffr, yl->col + diffc);

        (void) copyent(destino, yl, 0, 0, 0, 0, 0, 0, 0);

        destino->row += diffr;
        destino->col += diffc;

        #ifdef UNDO
        copy_to_undostruct(yl->row + diffr, yl->col + diffc, yl->row + diffr, yl->col + diffc, 'a');
        #endif

        yl = yl->next;
    }

    #ifdef UNDO
    end_undo_action();
    #endif
    return;
}
