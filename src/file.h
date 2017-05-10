#include "freeze.h"

void erasedb();
void loadrc(void);
int modcheck();
int savefile();
int writefile(char *fname, int r0, int c0, int rn, int cn, int verbose);
void write_fd(register FILE *f, int r0, int c0, int rn, int cn);
void write_cells(register FILE *f, int r0, int c0, int rn, int cn, int dr, int dc);
void write_marks(register FILE *f);
void write_franges(register FILE *f);
int readfile(char *fname, int eraseflg);
int file_exists(const char * fname);
char * findhome(char *path);
int backup_file(char *path);
FILE * openfile(char *fname, int *rpid, int *rfd);
void closefile(FILE *f, int pid, int rfd);
void print_options(FILE *f);
int import_csv(char * fname, char d);
void do_export(int r0, int c0, int rn, int cn);
void export_delim(char * fname, char coldelim, int r0, int c0, int rn, int cn, int verbose);
void export_plain(char * fname, int r0, int c0, int rn, int cn);
void unspecial(FILE * f, char * str, int delim);
int max_length(FILE * f);
int plugin_exists(char * name, int len, char * path);
void * do_autobackup();
void handle_backup();
void remove_backup(char * file);
int backup_exists(char * file);
