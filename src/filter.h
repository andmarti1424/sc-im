struct filter_item {
    char * eval;
} * filters;

void show_filters();
void add_filter(char * criteria);
void enable_filters(struct ent * left, struct ent * right);
void disable_filters();
void free_filters();
void del_filter(int id);
int exists_freed_filter();
