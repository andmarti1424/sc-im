int main (int argc, char ** argv);
int exit_app(int status);
void create_structures();
void delete_structures();
void load_sc(int argc, char ** argv);
void setorder(int i);
void nopipe();
void signals();
//
// SIGINT signal
#ifdef SIGVOID
void
#else
int
#endif
sig_int();

// SIGWINCH signal - resize of terminal
#ifdef SIGVOID
void
#else
int
#endif
winchg();

