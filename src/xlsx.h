#ifdef XLSX
#include <libxml/tree.h>
void get_sheet_data(xmlDocPtr doc, xmlDocPtr doc_strings, xmlDocPtr doc_styles);
char * get_xlsx_string(xmlDocPtr doc, int pos);
char * get_xlsx_styles(xmlDocPtr doc_styles, int pos);
#endif
int open_xlsx(char * fname, char * encoding);

