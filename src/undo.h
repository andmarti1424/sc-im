struct undo {
    struct undo * p_ant;
    struct ent * added;
    struct ent * removed;
    struct undo_range_shift * range_shift;
    struct undo * p_sig;
    int * row_hidded;
    int * row_showed;
    int * col_hidded;
    int * col_showed;
};

struct undo_range_shift {
    int delta_rows;
    int delta_cols;
    int tlrow;
    int tlcol;
    int brrow;
    int brcol;
};

void create_undo_action();
void end_undo_action();
void copy_to_undostruct (int row_desde, int col_desde, int row_hasta, int col_hasta, char type);
void save_undo_range_shift(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol);
void undo_hide_show(int row, int col, char type, int arg);

void add_to_undolist(struct undo u);
void do_undo();
void do_redo();

void clear_undo_list ();
void clear_from_here();
int len_undo_list();
