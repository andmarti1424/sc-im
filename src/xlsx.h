#ifdef XLSX
#include <libxml/tree.h>
void get_sheet_data(xmlDocPtr doc, xmlDocPtr doc_strings);
#endif
int open_xlsx(char * fname, char * encoding);

