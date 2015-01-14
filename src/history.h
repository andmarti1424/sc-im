struct history {
   int len;
   int pos;
   char mode;
   struct hlist * list;
};

struct hlist {
   char * line;
   struct hlist * pnext;
   struct hlist * pant;
};

struct history * create_history(char mode);
void destroy_history(struct history * h);
void load_history();
int save_history(struct history * h);
void del_item_from_history(struct history * h, int pos);
int move_item_from_history_by_str(struct history * h, char * item, int pos);
void add(struct history * h, char * line);
char * get_line_from_history(struct history * h, int pos);
struct hlist * get_hlist_from_history(struct history * h, int pos);
