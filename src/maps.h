#include "input.h"

struct map {
    int mode;
    struct block * in;
    struct block * out; 
    struct map * psig;
};
typedef struct map map;

extern unsigned int curmode;

map * get_last_map();
int replace_maps (struct block * b);
struct block * get_mapbuf_str (char * str);
void add_map(char * in, char * out, char mode);
void del_maps ();
