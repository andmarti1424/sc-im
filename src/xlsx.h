#ifdef XLSX
#include <libxml/tree.h>
void get_sheet_data(xmlDocPtr doc, xmlDocPtr doc_strings, xmlDocPtr doc_styles);
char * get_xlsx_string(xmlDocPtr doc, int pos);
char * get_xlsx_styles(xmlDocPtr doc_styles, int pos);
char * get_xlsx_number_format_by_id(xmlDocPtr doc_styles, int id);
#endif
int open_xlsx(char * fname, char * encoding);

