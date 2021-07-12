/*******************************************************************************
 * Copyright (c) 2013-2021, Andrés Martinelli <andmarti@gmail.com>             *
 * All rights reserved.                                                        *
 *                                                                             *
 * This file is a part of sc-im                                                *
 *                                                                             *
 * sc-im is a spreadsheet program that is based on sc. The original authors    *
 * of sc are James Gosling and Mark Weiser, and mods were later added by       *
 * Chuck Martin.                                                               *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without          *
 * modification, are permitted provided that the following conditions are met: *
 * 1. Redistributions of source code must retain the above copyright           *
 *    notice, this list of conditions and the following disclaimer.            *
 * 2. Redistributions in binary form must reproduce the above copyright        *
 *    notice, this list of conditions and the following disclaimer in the      *
 *    documentation and/or other materials provided with the distribution.     *
 * 3. All advertising materials mentioning features or use of this software    *
 *    must display the following acknowledgement:                              *
 *    This product includes software developed by Andrés Martinelli            *
 *    <andmarti@gmail.com>.                                                    *
 * 4. Neither the name of the Andrés Martinelli nor the                        *
 *   names of other contributors may be used to endorse or promote products    *
 *   derived from this software without specific prior written permission.     *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY ANDRES MARTINELLI ''AS IS'' AND ANY            *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE      *
 * DISCLAIMED. IN NO EVENT SHALL ANDRES MARTINELLI BE LIABLE FOR ANY           *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES  *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;*
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE       *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.           *
 *******************************************************************************/

/**
 * \file xlsx.c
 * \author Andrés Martinelli <andmarti@gmail.com>
 * \date 2017-07-18
 * \brief TODO Write a tbrief file description.
 *
 * \details xlsx import requires:
 * - libzip-dev
 * - libxml2-dev
 *
 * \details xlsx export requires
 * - libxlsxwriter
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>   // for isdigit
#include <stdlib.h>  // for atoi

#include "../macros.h"
#include "../sc.h"
#include "../cmds/cmds.h"
#include "../tui.h"
#include "../conf.h"
#include "../lex.h"
#include "../interp.h"
#include "../utils/string.h"

#ifdef XLSX
#include <zip.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "xlsx.h"

extern struct session * session;

/**
 * \brief TODO Document get_xlsx_string()
 *
 * \details This function takes the DOM of the sharedStrings file
 * and based on position, it returns the according string. Note
 * that 0 is the first string.
 *
 * \param[in] doc
 * \param[in] pos
 *
 * \return none
 */

char * get_xlsx_string(xmlDocPtr doc, int pos) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * father;
    char * result = NULL;

    while (pos--) cur_node = cur_node->next;

    father = cur_node;
    cur_node = father->xmlChildrenNode;

    while (father != NULL) {  // traverse children
        while (cur_node != NULL) {  // traverse relatives
            if ( ! xmlStrcmp(cur_node->name, (const xmlChar *) "t")
                && cur_node->xmlChildrenNode != NULL
                && cur_node->xmlChildrenNode->content != NULL
               ) {
                result = (char *) cur_node->xmlChildrenNode->content;
                //sc_debug("%s %s", cur_node->name, result);
                return result;
            }
            cur_node = cur_node->next;
        }

        father = father->xmlChildrenNode;
        if (father != NULL) cur_node = father->xmlChildrenNode;
    }

    return result;
}

/*
 * this functions takes the DOM of the styles file
 * and based on a position, it returns the according numFmtId
 * IMPORTANT: note that 0 is the first "xf".
 */
/**
 * \brief TODO Document get_xlsx_styles
 *
 * \details This function takes the DOM of the styles file
 * and mased on position, it returns the according numFmtId.
 * IMPORTANT: Note that 0 is the first "xf".
 *
 * \param[in] doc_styles
 * \param[in] pos
 *
 * \return none
 */

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

/**
 * \brief get_xlsx_number_format_by_id()
 * \param[in] doc_styles
 * \param[in] id
 * \return none
 */
char * get_xlsx_number_format_by_id(xmlDocPtr doc_styles, int id) {
    if (doc_styles == NULL)
        return NULL;

    // we go forward up to numFmts section
    xmlNode * cur_node = xmlDocGetRootElement(doc_styles)->xmlChildrenNode;
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "numFmts")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;
    // we go forward up to desidered format
    char * idFile = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    while (idFile && atoi(idFile) != id) {
        cur_node = cur_node->next;
        free(idFile);
        idFile = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    }

    if (idFile && atoi(idFile) == id) {
        free(idFile);
        return (char *) xmlGetProp(cur_node, (xmlChar *) "formatCode");
    } else {
        free(idFile);
        return NULL;
    }
}

/**
 * \brief TODO Document get_sheet_data()
 *
 * \details This function takes the sheetfile DOM and builds the tbl
 * spreadsheet (sc-im format)
 *
 * \return none
 */

void get_sheet_data(xmlDocPtr doc, xmlDocPtr doc_strings, xmlDocPtr doc_styles) {
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * child_node = NULL;
    wchar_t line_interp[FBUFLEN] = L"";
    int r, c;

    // we go forward up to sheet data
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "sheetData")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;       // this is sheetdata
    while (cur_node != NULL) {
        child_node = cur_node->xmlChildrenNode; // these are rows
        while (child_node != NULL) {            // these are cols

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
            char * shared = NULL;
            int i_fmtid = -1;
            if (fmtId != NULL) {
                i_fmtid = atoi(fmtId);
                if (i_fmtid != 0) numberFmt = get_xlsx_number_format_by_id(doc_styles, i_fmtid);
            }

            // try to handle custom formats
            if (i_fmtid == 278 || i_fmtid == 185 ||
                i_fmtid == 196 || i_fmtid == 217 || i_fmtid == 326 ||
                i_fmtid == 100 || (i_fmtid > 163 && i_fmtid < 181)) {
                char * strFormatCode = NULL;
                strFormatCode = get_xlsx_number_format_by_id(doc_styles, i_fmtid);
                if (strFormatCode != NULL && ! strcmp(strFormatCode, "General")) i_fmtid = 0;
                else if (strFormatCode != NULL && str_in_str(strFormatCode, "/") != -1) i_fmtid = 14;
            }

            // string
            if ( s != NULL && ! strcmp(s, "s") ) {
                char * st = NULL;
                char * strvalue = NULL;
                if (child_node->xmlChildrenNode != NULL)
                    strvalue =  get_xlsx_string(doc_strings, atoi((char *) child_node-> xmlChildrenNode-> xmlChildrenNode->content));
                if (strvalue != NULL && strvalue[0] != '\0') {
                    st = str_replace (strvalue, "\"", "''");
                    clean_carrier(st); // we handle padding
                    swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, st);
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
                    swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, st);
                    send_to_interp(line_interp);
                    free(st);
                }

            // numbers (can be dates, results from formulas or simple numbers)
            } else {
                // date value in v
                if (i_fmtid != -1 && child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v") &&
                (i_fmtid > 13 && i_fmtid < 18)) {
                    long l = strtol((char *) child_node->xmlChildrenNode->xmlChildrenNode->content, (char **) NULL, 10);

                    swprintf(line_interp, FBUFLEN, L"let %s%d=%.15ld", coltoa(c), r, (l - 25568) * 86400 - get_conf_int("tm_gmtoff"));
                    send_to_interp(line_interp);
                    struct ent * n = lookat(sh, r, c);
                    n->format = 0;
                    char * stringFormat = scxmalloc((unsigned)(strlen("%d/%m/%Y") + 2));
                    sprintf(stringFormat, "%c", 'd');
                    strcat(stringFormat, "%d/%m/%Y");
                    n->format = stringFormat;

                // time value in v
                } else if (i_fmtid != -1 && child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v")
                && (i_fmtid > 17 && i_fmtid < 22)
                ) {
                    double l = atof((char *) child_node->xmlChildrenNode->xmlChildrenNode->content);
                    swprintf(line_interp, FBUFLEN, L"let %s%d=%.15f", coltoa(c), r, (l - get_conf_int("tm_gmtoff") * 1.0 / 60 / 60 / 24) * 86400);
                    send_to_interp(line_interp);
                    struct ent * n = lookat(sh, r, c);
                    n->format = 0;
                    char * stringFormat = scxmalloc((unsigned)(strlen("%H:%M:%S") + 2));
                    sprintf(stringFormat, "%c", 'd');
                    strcat(stringFormat, "%H:%M:%S");
                    n->format = stringFormat;

                // v - straight int value
                } else if (
                child_node->xmlChildrenNode != NULL &&
                ! strcmp((char *) child_node->xmlChildrenNode->name, "v") ){
                    double l = atof((char *) child_node->xmlChildrenNode->xmlChildrenNode->content);
                    swprintf(line_interp, FBUFLEN, L"let %s%d=%.15f", coltoa(c), r, l);
                    send_to_interp(line_interp);

                // f - numeric value that is a result from formula
                } else if (
                child_node->xmlChildrenNode != NULL && ! strcmp((char *) child_node->xmlChildrenNode->name, "f")) {

                    // handle the formula if that is whats desidered!!
                    if (get_conf_int("xlsx_readformulas") &&
                        // dont handle shared formulas right now
                        ! (xmlHasProp(child_node->xmlChildrenNode, (xmlChar *) "t") &&
                        ! strcmp((shared = (char *) xmlGetProp(child_node->xmlChildrenNode, (xmlChar *) "t")), "shared"))
                    ) {
                        char * formula = (char *) child_node->xmlChildrenNode->xmlChildrenNode->content;
                        char * strf;
                        uppercase(formula);

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
                        swprintf(line_interp, FBUFLEN, L"let %s%d=%s", coltoa(c), r, formula);

                    } else {
                        double l = atof((char *) child_node->last->xmlChildrenNode->content);
                        swprintf(line_interp, FBUFLEN, L"let %s%d=%.15f", coltoa(c), r, l);
                    }
                    send_to_interp(line_interp);
                }
            }

            xmlFree(s);
            xmlFree(fmtId);
            xmlFree(style);
            xmlFree(numberFmt);
            xmlFree(shared);

            child_node = child_node->next;
            xmlFree(col);
            xmlFree(row);
        }
        cur_node = cur_node->next;
    }
    return;
}

/**
 * \brief TODO Document open_xlsx()
 *
 * \param[in] fname
 * \param[in] encoding
 *
 * \return none
 */

int open_xlsx(char * fname, char * encoding) {
    //struct roman * roman = session->cur_doc;
    //struct sheet * sh = roman->cur_sh;
    struct zip * za;
    struct zip_file * zf;
    struct zip_stat sb_sheet, sb_strings, sb_styles, sh_strings;
    char buf[100];
    int err;
    int len;

    // open zip file
    if ((za = zip_open(fname, 0, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        sc_error("can't open zip archive `%s': %s", fname, buf);
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
            sc_error("cannot read file %s.\n", name);
            free(strings);
            return -1;
        }
        zip_fclose(zf);
    }

    // open xl/styles.xml
    name = "xl/styles.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        sc_error("cannot open %s file.", name);
        if (strings != NULL) free(strings);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_styles);
    char * styles = NULL;
    styles = (char *) malloc(sb_styles.size);
    len = zip_fread(zf, styles, sb_styles.size);
    if (len < 0) {
        sc_error("cannot read file %s.", name);
        if (strings != NULL) free(strings);
        free(styles);
        return -1;
    }
    zip_fclose(zf);

    //open xml file with sheet names
    name = "xl/workbook.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( zf ) {
        zip_stat(za, name, ZIP_FL_UNCHANGED, &sh_strings);
        char * wb_strings = (char *) malloc(sh_strings.size);
        len = zip_fread(zf, wb_strings, sh_strings.size);
        if (len < 0) {
            sc_error("cannot read file %s.", name);
            if (strings != NULL) free(strings);
            if (styles != NULL) free(styles);
            free(wb_strings);
            return -1;
        }
        zip_fclose(zf);

        // search workbook xml for sheet names
        xmlDoc * sheet_search = xmlReadMemory(wb_strings, sh_strings.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);
        xmlNode * cur_node = xmlDocGetRootElement(sheet_search)->xmlChildrenNode;
        while (cur_node != NULL && strcmp((char *) cur_node->name,"sheets"))
            cur_node = cur_node->next;
        cur_node = cur_node->xmlChildrenNode;

        char * sheet_name = NULL;
        char * sheet_id = NULL;
        wchar_t cline [BUFFERSIZE];
        char sheet_filename [BUFFERSIZE];
        // here traverse for the sheets
        while (cur_node != NULL) {
            sheet_name = (char *) xmlGetProp(cur_node, (xmlChar *) "name");

            swprintf(cline, BUFFERSIZE, L"newsheet \"%s\"", sheet_name);
            send_to_interp(cline);

            sheet_id = (char *) xmlGetProp(cur_node, (xmlChar *) "sheetId");

            snprintf(sheet_filename, BUFFERSIZE, "xl/worksheets/sheet%s.xml", sheet_id);

            // open the sheet and load it
            //sheet_filename = name = "xl/worksheets/sheet1.xml";
            //open each xml sheet file and parse it
            zf = zip_fopen(za, sheet_filename, ZIP_FL_UNCHANGED);
            if ( ! zf ) {
                sc_error("cannot open %s file.", sheet_filename);
                if (strings != NULL) free(strings);
                if (styles != NULL) free(styles);
                return -1;
            }
            zip_stat(za, sheet_filename, ZIP_FL_UNCHANGED, &sb_sheet);
            char * sheet = NULL;
            sheet = (char *) malloc(sb_sheet.size);
            len = zip_fread(zf, sheet, sb_sheet.size);
            if (len < 0) {
                sc_error("cannot read file %s.", sheet_filename);
                if (strings != NULL) free(strings);
                if (styles != NULL) free(styles);
                if (sheet != NULL) free(sheet);
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
            doc = xmlReadMemory(sheet, sb_sheet.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);

            if (doc == NULL) {
                sc_error("error: could not parse file");
                if (strings != NULL) free(strings);
                if (styles != NULL) free(styles);
                if (sheet != NULL) free(sheet);
                return -1;
            }

            get_sheet_data(doc, doc_strings, doc_styles);

            // free the document
            xmlFreeDoc(doc);
            xmlFreeDoc(doc_strings);
            xmlFreeDoc(doc_styles);

            // now free and iterate to other sheet
            xmlFree(sheet_name);
            sheet_name = NULL;
            xmlFree(sheet_id);
            sheet_id = NULL;
            if (sheet != NULL) free(sheet);

            struct roman * roman = session->cur_doc;
            struct sheet * sh = roman->cur_sh;
            auto_fit(sh, 0, sh->maxcols, DEFWIDTH);
            deleterow(sh, sh->currow, 1);

            cur_node = cur_node->next;
        }
        xmlFreeDoc(sheet_search);
        if (wb_strings != NULL) free(wb_strings);
    }

    // Free the global variables that may have been allocated by the parser
    xmlCleanupParser();

    // free both styles and strings variables
    if (strings != NULL) free(strings);
    if (styles != NULL) free(styles);

    // close zip file
    if (zip_close(za) == -1) {
        sc_error("cannot close zip archive `%s'", fname);
        return -1;
    }

    return 0;
}
#endif

#ifdef XLSX_EXPORT
#include "xlsxwriter.h"
/**
 * \brief export_xlsx()
 * \param[in] filename
 * \return none
 */
int export_xlsx(char * filename) {
    int row, col;
    struct ent ** pp;
    struct roman * roman = session->cur_doc;
    lxw_workbook  * workbook  = workbook_new(filename);

    struct sheet * sh = roman->first_sh;
    while (sh != NULL) {

        lxw_worksheet * worksheet = workbook_add_worksheet(workbook, NULL);
        int bkp_currow = sh->currow;
        sh->currow = 0;
        insert_row(sh, 0); //add a row so that scim formulas apply to excel

        for (row = 0; row <= sh->maxrow+1; row++)
            for (pp = ATBL(sh, sh->tbl, row, col = 0); col <= sh->maxcol; col++, pp++)
                if (*pp) {
                    // Check format here
                    lxw_format * format = workbook_add_format(workbook);

                    // handle alignment
                    if ((*pp)->label && (*pp)->flags & is_label)          // center align
                        format_set_align(format, LXW_ALIGN_CENTER);
                    else if ((*pp)->label && (*pp)->flags & is_leftflush) // left align
                        format_set_align(format, LXW_ALIGN_LEFT);
                    else if ((*pp)->label)                                // right align
                        format_set_align(format, LXW_ALIGN_RIGHT);

                    // handle bold, italic and underline
                    if ((*pp)->ucolor != NULL && (*pp)->ucolor->bold)
                        format_set_bold(format);
                    else if ((*pp)->ucolor != NULL && (*pp)->ucolor->italic)
                        format_set_italic(format);
                    else if ((*pp)->ucolor != NULL && (*pp)->ucolor->underline)
                        format_set_underline(format, LXW_UNDERLINE_SINGLE);

                    // handle fg color
                    if ((*pp)->ucolor != NULL && (*pp)->ucolor->fg) {
                        int fgcolor;
                        switch ((*pp)->ucolor->fg) {
                            case BLACK:
                                fgcolor = LXW_COLOR_BLACK;
                                break;
                            case RED:
                                fgcolor = LXW_COLOR_RED;
                                break;
                            case GREEN:
                                fgcolor = LXW_COLOR_GREEN;
                                break;
                            case YELLOW:
                                fgcolor = LXW_COLOR_YELLOW;
                                break;
                            case BLUE:
                                fgcolor = LXW_COLOR_BLUE;
                                break;
                            case MAGENTA:
                                fgcolor = LXW_COLOR_MAGENTA;
                                break;
                            case CYAN:
                                fgcolor = LXW_COLOR_CYAN;
                                break;
                            case WHITE:
                                fgcolor = LXW_COLOR_WHITE;
                                break;
                        }
                        format_set_font_color(format, fgcolor);
                    }

                    // handle bg color
                    if ((*pp)->ucolor != NULL && (*pp)->ucolor->bg) {
                        int bgcolor;
                        switch ((*pp)->ucolor->bg) {
                            case BLACK:
                                bgcolor = LXW_COLOR_BLACK;
                                break;
                            case RED:
                                bgcolor = LXW_COLOR_RED;
                                break;
                            case GREEN:
                                bgcolor = LXW_COLOR_GREEN;
                                break;
                            case YELLOW:
                                bgcolor = LXW_COLOR_YELLOW;
                                break;
                            case BLUE:
                                bgcolor = LXW_COLOR_BLUE;
                                break;
                            case MAGENTA:
                                bgcolor = LXW_COLOR_MAGENTA;
                                break;
                            case CYAN:
                                bgcolor = LXW_COLOR_CYAN;
                                break;
                            case WHITE:
                                bgcolor = LXW_COLOR_WHITE;
                                break;
                        }
                        format_set_bg_color(format, bgcolor);
                    }

                    // dateformat
                    if ((*pp) && (*pp)->format && (*pp)->format[0] == 'd') {
                        char sc_format[BUFFERSIZE];
                        char * st = NULL;
                        strcpy(sc_format, &((*pp)->format[1]));

                        st = str_replace(sc_format, "%Y", "yyyy");
                        strcpy(sc_format, st);
                        free(st);
                        st = str_replace(sc_format, "%y", "yy");
                        strcpy(sc_format, st);
                        free(st);
                        st = str_replace(sc_format, "%m", "mm");
                        strcpy(sc_format, st);
                        free(st);
                        st = str_replace(sc_format, "%d", "dd");
                        strcpy(sc_format, st);
                        free(st);
                        format_set_num_format(format, sc_format);
                        worksheet_write_number(worksheet, row-1, col, (((*pp)->v + get_conf_int("tm_gmtoff")) / 86400 + 25568) , format);

                        // formula
                    } else if ((*pp) && (*pp)->expr && get_conf_int("xlsx_readformulas"))  {
                        linelim = 0;
                        editexp(sh, (*pp)->row, (*pp)->col);
                        linelim = -1;

                        char * strf;
                        char formula[BUFFERSIZE];
                        strcpy(formula, line);

                        strf = str_replace(formula, "@count","count");
                        strcpy(formula, strf);
                        free(strf);

                        strf = str_replace(formula, "@sum","sum");
                        strcpy(formula, strf);
                        free(strf);

                        strf = str_replace(formula, "@prod","product");
                        strcpy(formula, strf);
                        free(strf);

                        strf = str_replace(formula, "@avg","average");
                        strcpy(formula, strf);
                        free(strf);

                        strf = str_replace(formula, "@min","min");
                        strcpy(formula, strf);
                        free(strf);

                        strf = str_replace(formula, "@max","max");
                        strcpy(formula, strf);
                        free(strf);

                        strf = str_replace(formula, "@abs","abs");
                        strcpy(formula, strf);
                        free(strf);

                        strf = str_replace(formula, "@stddev","stdev");
                        strcpy(formula, strf);
                        free(strf);

                        add_char(formula, '=', 0);
                        worksheet_write_formula(worksheet, row-1, col, formula, NULL);
                        worksheet_write_formula_num(worksheet, row-1, col, formula, NULL, (*pp)->v);

                        // If a numeric value exists
                    } else if ( (*pp)->flags & is_valid) {
                        worksheet_write_number(worksheet, row-1, col, (*pp)->v, format);

                    } else if ((*pp)->label) {
                        worksheet_write_string(worksheet, row-1, col, (*pp)->label, format);
                    }
                    /* TODO: handle hidden rows and columns? */
                }
        sh->currow = 0;
        int_deleterow(sh, 0, 1); /* delete the added row */
        sh->currow = bkp_currow;

        sh = sh->next;
    }

    return workbook_close(workbook);
}
#endif
