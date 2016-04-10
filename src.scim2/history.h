struct history {
   int len;
   int pos;
   char mode;
   struct hlist * list;
};

struct hlist {
   wchar_t * line;
   struct hlist * pnext;
   struct hlist * pant;
};

struct history * create_history(char mode);
void destroy_history(struct history * h);
void load_history();
int save_history(struct history * h);
void del_item_from_history(struct history * h, int pos);
int move_item_from_history_by_str(struct history * h, wchar_t * item, int pos);
void add(struct history * h, wchar_t * line);
wchar_t * get_line_from_history(struct history * h, int pos);
struct hlist * get_hlist_from_history(struct history * h, int pos);

// current command before tab completion
void copy_to_curcmd(wchar_t * inputline);
wchar_t * get_curcmd();

// tab completion mark
int get_comp();
void set_comp(int);
