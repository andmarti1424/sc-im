#define TRG_READ  1
#define TRG_WRITE 2
#define TRG_LUA   4
#define TRG_SH    8
#define TRG_C    16

struct trigger {
    int flag;       /* Read + Write + interface */
    char * file;
    char * function;
    void * handle;  /* this is only for C Triggers */
    int (*c_function) (struct ent *, int);
};

void do_trigger( struct ent *p , int rw);
void set_trigger(int r, int c, int rf, int cf, char * str);
void del_trigger(int r, int c, int rf, int cf );
void do_C_Trigger_cell(struct ent * p, int rw);
int plugin_exists(char *name, int len, char *path);
