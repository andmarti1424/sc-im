#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zip.h>
#include <ctype.h>   // for isdigit
#include "macros.h"
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "lex.h"

#ifdef XLSX
#include <libxml/parser.h>
#include <libxml/tree.h>

//requires libzip-dev
//requires libxml2-dev
//for building: gcc -lzip -lxml2 -I/usr/include/libxml2 t.c


// this functions takes the DOM of the sharedStrings file
// and returs based on a position, it returns the according string
// note that 0 is the first string.
char * get_xlsx_string(xmlDocPtr doc, int pos) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;

    while (pos--) cur_node = cur_node->next;
    cur_node = cur_node->xmlChildrenNode; return (char *) cur_node->xmlChildrenNode->content;
}

// this function takes the sheetfile DOM and builds the tbl spreadsheet (SCIM format)
void get_sheet_data(xmlDocPtr doc, xmlDocPtr doc_strings) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * child_node = NULL;
    char line_interp[FBUFLEN] = "";    
    int r, c;

    // we go forward up to sheet data
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "sheetData")))
        cur_node = cur_node->next;

    //printf("node type:: %d, name: %s\n", cur_node->type, cur_node->name); // this is sheetdata
    cur_node = cur_node->xmlChildrenNode;
    while (cur_node != NULL) {
        //printf("--: %s\n", cur_node->name);                               // this are rows
        child_node = cur_node->xmlChildrenNode;
        while (child_node != NULL) {                                        // this are cols
            char * row = (char *) xmlGetProp(cur_node, (xmlChar *) "r");
            r = atoi(row) - 1;
            //printf("++: %s ", child_node->name);
            char * col = (char *) xmlGetProp(child_node, (xmlChar *) "r");
            while (isdigit(col[strlen(col)-1])) col[strlen(col)-1]='\0';
            c = atocol(col, strlen(col));
            //printf("r=%s ", xmlGetProp(child_node, "r"));
            //printf("s=%s ", xmlGetProp(child_node, "s"));
            if (xmlHasProp(child_node, (xmlChar *) "t")) {

                char * s = (char *) xmlGetProp(child_node, (xmlChar *) "t");

                if ( ! strcmp(s, "n") ) {        // number
                    long l = 0;
                    if (! strcmp((char *) child_node->xmlChildrenNode->name, "v")) {        // v - straight int value
                        //info("%s",  child_node->xmlChildrenNode->name); get_key();
                        //info("%s",  child_node->xmlChildrenNode->content); get_key();
                        l = strtol((char *) child_node->xmlChildrenNode->xmlChildrenNode->content, (char **) NULL, 10);
                    } else if (! strcmp((char *) child_node->xmlChildrenNode->name, "f")) { // f - numeric value is result from formula
                        l = strtol((char *) child_node->last->xmlChildrenNode->content, (char **) NULL, 10);
                        //info("%s", child_node->last->name); get_key();
                        //info("%s", child_node->last->xmlChildrenNode->content); get_key();
                    } 
                    sprintf(line_interp, "let %s%d=%.15ld", coltoa(c), r, l);
                    send_to_interp(line_interp);
                } else if ( ! strcmp(s, "s") ) { // string
                    sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, 
                    get_xlsx_string(doc_strings, atoi((char *) child_node->xmlChildrenNode->xmlChildrenNode->content)));
                    send_to_interp(line_interp);
                }

                xmlFree(s);
                //printf("t=%s ", xmlGetProp(child_node, "t"));
                //printf("%s", child_node->xmlChildrenNode->name); //v
                //printf("=%s", child_node->xmlChildrenNode->xmlChildrenNode->content);
            }
            //printf("\n");
            child_node = child_node->next;
            xmlFree(row);
        }
        cur_node = cur_node->next;
    }
    return;
}

int open_xlsx(char * fname, char * encoding) {
    struct zip * za;
    struct zip_file * zf;
    struct zip_stat sb, sb_strings;
    char buf[100];
    int err;
    int len;
    
    // open zip file
    if ((za = zip_open(fname, 0, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        error("can't open zip archive `%s': %s", fname, buf);
        return -1;
    }
 
    // open xl/sharedStrings.xml
    char * name = "xl/sharedStrings.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if (! zf) {
        error("cannot open %s file.\n", name);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_strings);
    char * strings = (char *) malloc(sb_strings.size);
    len = zip_fread(zf, strings, sb_strings.size);
    if (len < 0) {
        error("cannot read file %s.\n", name);
        free(strings);
        return -1;
    }
    zip_fclose(zf);


    // open xl/worksheets/sheet1.xml";
    name = "xl/worksheets/sheet1.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        error("cannot open %s file.", name);
        free(strings);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb);
    char * sheet = (char *) malloc(sb.size);
    len = zip_fread(zf, sheet, sb.size);
    if (len < 0) {
        error("cannot read file %s.", name);
        free(sheet);
        free(strings);
        return -1;
    }
    zip_fclose(zf);


    // XML parse for the sheet file
    xmlDoc * doc = NULL;
    xmlDoc * doc_strings = NULL;

    // this initialize the library and check potential ABI mismatches
    // between the version it was compiled for and the actual shared
    // library used.
    LIBXML_TEST_VERSION

    // parse the file and get the DOM
    doc_strings = xmlReadMemory(strings, sb_strings.size, "noname2.xml", NULL, 0);
    doc = xmlReadMemory(sheet, sb.size, "noname.xml", NULL, 0);

    if (doc == NULL || doc_strings == NULL) {
        error("error: could not parse file");
        free(sheet);
        free(strings);
        return -1;
    }

    get_sheet_data(doc, doc_strings);

    // free the document
    xmlFreeDoc(doc);
    xmlFreeDoc(doc_strings);

    // Free the global variables that may have been allocated by the parser
    xmlCleanupParser();

    // free both sheet and strings variables
    free(sheet);
    free(strings);

    // close zip file
    if (zip_close(za) == -1) {
        error("cannot close zip archive `%s'", fname);
        return -1;
    }
 
    auto_justify(0, maxcol, DEFWIDTH);
    return 0;
}
#endif
