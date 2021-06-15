#include "sc.h"

struct sheet * search_sheet(struct roman * doc, char * name);
struct sheet * new_sheet(struct roman * doc, char * name);
int get_num_sheets(struct roman * doc);
void free_session(struct session * session);
void delete_sheet(struct roman * roman, struct sheet * sh, int flg_free);
void delete_doc(struct session * session, struct roman * doc);
