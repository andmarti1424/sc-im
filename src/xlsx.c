#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>   // for isdigit
#include <stdlib.h>  // for atoi
#include "macros.h"
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "conf.h"
#include "lex.h"
#include "utils/string.h"

#ifdef XLSX
#include <zip.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "xlsx.h"

// ------------------------------------------------------------------
// requires libzip-dev
// requires libxml2-dev
// for building: gcc -lzip -lxml2 -I/usr/include/libxml2 xlsx.c
// ------------------------------------------------------------------


// this functions takes the DOM of the sharedStrings file
// and based on a position, it returns the according string
// note that 0 is the first string.
char * get_xlsx_string(xmlDocPtr doc, int pos) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * father;
    char * result = NULL;

    while (pos--) cur_node = cur_node->next;

    father = cur_node;
    cur_node = father->xmlChildrenNode;

    while (father != NULL) {  // recorro hijos
        while (cur_node != NULL) {  // recorro hermanos
            if ( ! xmlStrcmp(cur_node->name, (const xmlChar *) "t")
                && cur_node->xmlChildrenNode != NULL
                && cur_node->xmlChildrenNode->content != NULL
               ) {
                result = (char *) cur_node->xmlChildrenNode->content;
                //scdebug("%s %s", cur_node->name, result);
                return result;
            }
            cur_node = cur_node->next;
        }

        father = father->xmlChildrenNode;
        if (father != NULL) cur_node = father->xmlChildrenNode;
    }

    return result;
}

// this functions takes the DOM of the styles file
// and based on a position, it returns the according numFmtId
// IMPORTANT: note that 0 is the first "xf".
char * get_xlsx_styles(xmlDocPtr doc_styles, int pos) {
    // we go forward up to styles data
    xmlNode * cur_node = xmlDocGetRootElement(doc_styles)->xmlChildrenNode;
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "cellXfs")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;
    // we go forward up to desidered numFmtId
    while (pos--) cur_node = cur_node->next;
    char * id = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    return id;
}

char * get_xlsx_number_format_by_id(xmlDocPtr doc_styles, int id) {
    if (doc_styles == NULL || !((id >= 165 && id <= 180) || id == 100)) 
        return NULL;

    // we go forward up to numFmts section
    xmlNode * cur_node = xmlDocGetRootElement(doc_styles)->xmlChildrenNode;
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "numFmts")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;
    // we go forward up to desidered format
    char * idFile = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    while (atoi(idFile) != id) {
        cur_node = cur_node->next;
        free(idFile);
        idFile = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    }
    
    if (atoi(idFile) == id) {
        free(idFile);
        return (char *) xmlGetProp(cur_node, (xmlChar *) "formatCode");
    } else {
        free(idFile);
        return NULL;
    }
}

// this function takes the sheetfile DOM and builds the tbl spreadsheet (SC-IM format)
void get_sheet_data(xmlDocPtr doc, xmlDocPtr doc_strings, xmlDocPtr doc_styles) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * child_node = NULL;
    char line_interp[FBUFLEN] = "";    
    int r, c;

    // we go forward up to sheet data
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "sheetData")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;       // this is sheetdata
    while (cur_node != NULL) {
        child_node = cur_node->xmlChildrenNode; // this are rows
        while (child_node != NULL) {            // this are cols

            // We get r y c
            char * row = (char *) xmlGetProp(cur_node, (xmlChar *) "r");
            r = atoi(row);
            char * col = (char *) xmlGetProp(child_node, (xmlChar *) "r");
            while (isdigit(col[strlen(col)-1])) col[strlen(col)-1]='\0';
            c = atocol(col, strlen(col));

            char * s = (char *) xmlGetProp(child_node, (xmlChar *) "t"); // type
            char * style = NULL;
            style = (char *) xmlGetProp(child_node, (xmlChar *) "s");    // style
            char * fmtId = style == NULL ? NULL : get_xlsx_styles(doc_styles, atoi(style)); // numfmtId by style number
            char * numberFmt = NULL;
            if (fmtId != NULL && atoi(fmtId) != 0) {
                numberFmt = get_xlsx_number_format_by_id(doc_styles, atoi(fmtId));
            }

            // string
            if ( s != NULL && ! strcmp(s, "s") ) {
                char * st = NULL;
                char * strvalue =  get_xlsx_string(doc_strings, atoi((char *) child_node->xmlChildrenNode->xmlChildrenNode->content));
                if (strvalue != NULL && strvalue[0] != '\0') {
                    st = str_replace (strvalue, "\"", "''");
                    clean_carrier(st); // we handle padding
                    sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, st);
                    send_to_interp(line_interp);
                    free(st);
                }

            // inlinestring
            } else if ( s != NULL && ! strcmp(s, "inlineStr") ) {
                char * st = NULL;
                char * strvalue = (char *) child_node->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode->content;
                if (strvalue != NULL && strvalue[0] != '\0') {
                    st = str_replace (strvalue, "\"", "''");
                    clean_carrier(st); // we handle padding
                    sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, st);
                    send_to_interp(line_interp);
                    free(st);
                }

            // numbers (can be dates, results from formulas or simple numbers)
            } else {
                // date value in v
                if (fmtId != NULL && child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v")
                && (
                (atoi(fmtId) >= 14 && atoi(fmtId) <= 17) || 
                atoi(fmtId) == 278 || atoi(fmtId) == 185 ||
                atoi(fmtId) == 196 ||
                atoi(fmtId) == 217 || atoi(fmtId) == 326 ||
                (((atoi(fmtId) >= 165 && atoi(fmtId) <= 180) ||
                atoi(fmtId) == 100) && numberFmt != NULL // 100,165-180 are user defined formats!!
                && str_in_str(numberFmt, "/") != -1)
                )) {
                    long l = strtol((char *) child_node->xmlChildrenNode->xmlChildrenNode->content, (char **) NULL, 10);

                    sprintf(line_interp, "let %s%d=%.15ld", coltoa(c), r, (l - 25569) * 86400 - atoi(get_conf_value("tm_gmtoff")));
                    send_to_interp(line_interp);
                    struct ent * n = lookat(r, c);
                    n->format = 0;
                    char * stringFormat = scxmalloc((unsigned)(strlen("%d/%m/%Y") + 2));
                    sprintf(stringFormat, "%c", 'd');
                    strcat(stringFormat, "%d/%m/%Y");
                    n->format = stringFormat;

                // time value in v
                } else if (fmtId != NULL && child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v")
                && (
                (atoi(fmtId) >= 18 && atoi(fmtId) <= 21)
                )) {
                    double l = atof((char *) child_node->xmlChildrenNode->xmlChildrenNode->content);
                    sprintf(line_interp, "let %s%d=%.15f", coltoa(c), r, (l - atoi(get_conf_value("tm_gmtoff")) * 1.0 / 60 / 60 / 24) * 86400);
                    send_to_interp(line_interp);
                    struct ent * n = lookat(r, c);
                    n->format = 0;
                    char * stringFormat = scxmalloc((unsigned)(strlen("%H:%M:%S") + 2));
                    sprintf(stringFormat, "%c", 'd');
                    strcat(stringFormat, "%H:%M:%S");
                    n->format = stringFormat;

                // v - straight int value
                } else if (//fmtId != NULL &&
                child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v") ){
                    double l = atof((char *) child_node->xmlChildrenNode->xmlChildrenNode->content);
                    sprintf(line_interp, "let %s%d=%.15f", coltoa(c), r, l);
                    send_to_interp(line_interp);

                // f - numeric value that is a result from formula
                } else if (//fmtId != NULL &&
                child_node->xmlChildrenNode != NULL && ! strcmp((char *) child_node->xmlChildrenNode->name, "f")) {

                    // handle the formula if that is whats desidered!!
                    if (atoi(get_conf_value("xlsx_readformulas"))) {
                        char * formula = (char *) child_node->xmlChildrenNode->xmlChildrenNode->content;
                        char * strf;

                        // we take some excel common function and adds a @ to them
                        // we replace count sum avg with @count, @sum, @prod, @avg, @min, @max 
                        strf = str_replace (formula, "COUNT","@COUNT");
                        strcpy(formula, strf);
                        free(strf);
                        strf = str_replace (formula, "SUM","@SUM");
                        strcpy(formula, strf);
                        free(strf);
                        strf = str_replace (formula, "PRODUCT","@PROD");
                        strcpy(formula, strf);
                        free(strf);
                        strf = str_replace (formula, "AVERAGE","@AVG");
                        strcpy(formula, strf);
                        free(strf);
                        strf = str_replace (formula, "MIN","@MIN");
                        strcpy(formula, strf);
                        free(strf);
                        strf = str_replace (formula, "MAX","@MAX");
                        strcpy(formula, strf);
                        free(strf);
                        strf = str_replace (formula, "ABS","@ABS");
                        strcpy(formula, strf);
                        free(strf);
                        strf = str_replace (formula, "STDEV","@STDDEV");
                        strcpy(formula, strf);
                        free(strf);
                        

                        // we send the formula to the interpreter and hope to resolve it!
                        sprintf(line_interp, "let %s%d=%s", coltoa(c), r, formula);

                    } else {
                        double l = atof((char *) child_node->last->xmlChildrenNode->content);
                        sprintf(line_interp, "let %s%d=%.15f", coltoa(c), r, l);
                    }
                    send_to_interp(line_interp);
                } 
            }

            xmlFree(s);
            xmlFree(fmtId);
            xmlFree(style);
            xmlFree(numberFmt);

            child_node = child_node->next;
            xmlFree(col);
            xmlFree(row);
        }
        cur_node = cur_node->next;
    }
    return;
}

int open_xlsx(char * fname, char * encoding) {
    struct zip * za;
    struct zip_file * zf;
    struct zip_stat sb, sb_strings, sb_styles;
    char buf[100];
    int err;
    int len;
    
    // open zip file
    if ((za = zip_open(fname, 0, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        scerror("can't open zip archive `%s': %s", fname, buf);
        return -1;
    }
 
    // open xl/sharedStrings.xml
    char * name = "xl/sharedStrings.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    char * strings = NULL;
    if (zf) {
        // some files may not have strings
        zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_strings);
        strings = (char *) malloc(sb_strings.size);
        len = zip_fread(zf, strings, sb_strings.size);
        if (len < 0) {
            scerror("cannot read file %s.\n", name);
            free(strings);
            return -1;
        }
        zip_fclose(zf);
    }

    // open xl/styles.xml
    name = "xl/styles.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        scerror("cannot open %s file.", name);
        if (strings != NULL) free(strings);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_styles);
    char * styles = (char *) malloc(sb_styles.size);
    len = zip_fread(zf, styles, sb_styles.size);
    if (len < 0) {
        scerror("cannot read file %s.", name);
        if (strings != NULL) free(strings);
        free(styles);
        return -1;
    }
    zip_fclose(zf);


    // open xl/worksheets/sheet1.xml
    name = "xl/worksheets/sheet1.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        scerror("cannot open %s file.", name);
        if (strings != NULL) free(strings);
        free(styles);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb);
    char * sheet = (char *) malloc(sb.size);
    len = zip_fread(zf, sheet, sb.size);
    if (len < 0) {
        scerror("cannot read file %s.", name);
        if (strings != NULL) free(strings);
        free(styles);
        free(sheet);
        return -1;
    }
    zip_fclose(zf);


    // XML parse for the sheet file
    xmlDoc * doc = NULL;
    xmlDoc * doc_strings = NULL;
    xmlDoc * doc_styles = NULL;

    // this initialize the library and check potential ABI mismatches
    // between the version it was compiled for and the actual shared
    // library used.
    LIBXML_TEST_VERSION

    // parse the file and get the DOM
    doc_strings = xmlReadMemory(strings, sb_strings.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);
    doc_styles = xmlReadMemory(styles, sb_styles.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);
    doc = xmlReadMemory(sheet, sb.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);

    if (doc == NULL) {
        scerror("error: could not parse file");
        if (strings != NULL) free(strings);
        free(styles);
        free(sheet);
        return -1;
    }

    get_sheet_data(doc, doc_strings, doc_styles);

    // free the document
    xmlFreeDoc(doc);
    xmlFreeDoc(doc_strings);
    xmlFreeDoc(doc_styles);

    // Free the global variables that may have been allocated by the parser
    xmlCleanupParser();

    // free both sheet and strings variables
    if (strings != NULL) free(strings);
    free(styles);
    free(sheet);

    // close zip file
    if (zip_close(za) == -1) {
        scerror("cannot close zip archive `%s'", fname);
        return -1;
    }
 
    auto_justify(0, maxcol, DEFWIDTH);
    deleterow();
    return 0;
}
#endif
