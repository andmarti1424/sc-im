#include "sc.h"

struct sheet * search_sheet(struct roman * doc, char * name);
struct sheet * new_sheet(struct roman * doc, char * name);
int get_num_sheets(struct roman * doc);
void free_session(struct session * session);
