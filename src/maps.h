#include "input.h"

struct map {
    int mode;
    short recursive;
    struct block * in;
    struct block * out;
    struct map * psig;
};
typedef struct map map;

extern unsigned int curmode;

int replace_maps (struct block * b);
struct block * get_mapbuf_str (char * str);
void del_maps ();
map * get_last_map();
int exists_map(char * in, int mode);
void add_map(char * in, char * out, int mode, short recursive);
void del_map(char * in, int mode);
void get_mapstr_buf (struct block * b, char * str);
void get_mappings(char * salida);
