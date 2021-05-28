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
 * \date 2021-03-27
 * \brief file that contains the functions to support ods file import
 *
 * \details ods import requires:
 * - libzip-dev
 * - libxml2-dev
 */

#ifdef ODS
#include <errno.h>
#include <zip.h>
#include <libxml/parser.h>

#include "../tui.h"
#include "../cmds/cmds.h"
#include "../sc.h"
#include "../utils/string.h"
#endif

extern struct session * session;

/**
 * \brief open_ods() files
 *
 * \param[in] fname
 * \param[in] encoding
 *
 * \return none
 */

int open_ods(char * fname, char * encoding) {
#ifdef ODS
    struct roman * roman = session->cur_doc;
    struct sheet * sh = roman->cur_sh;
    struct zip * za;
    struct zip_file * zf;
    struct zip_stat sb_content;

    char buf[100];
    int err;
    int len;

    // open zip file
    if ((za = zip_open(fname, 0, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        sc_error("can't open zip archive `%s': %s", fname, buf);
        return -1;
    }

    // open content.xml
    char * name = "content.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    char * content = NULL;
    if (zf) {
        // some files may not have strings
        zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_content);
        content = (char *) malloc(sb_content.size);
        len = zip_fread(zf, content, sb_content.size);
        if (len < 0) {
            sc_error("cannot read file %s.\n", name);
            free(content);
            return -1;
        }
        zip_fclose(zf);
    }

    // XML parse for the sheet file
    xmlDoc * doc = NULL;

    // this initialize the library and check potential ABI mismatches
    // between the version it was compiled for and the actual shared
    // library used.
    LIBXML_TEST_VERSION


    doc = xmlReadMemory(content, sb_content.size, "noname.xml", NULL, XML_PARSE_NOBLANKS);

    if (doc == NULL) {
        sc_error("error: could not parse ods file");
        if (content != NULL) free(content);
        return -1;
    }

    // parse here
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * child_node = NULL;
    wchar_t line_interp[FBUFLEN] = L"";
    int r=0, c=-1;
    while (cur_node != NULL && strcmp((char *) cur_node->name, "body")) cur_node = cur_node->next; // forward until reach body
    cur_node = cur_node->xmlChildrenNode;
    while (cur_node != NULL && strcmp((char *) cur_node->name, "spreadsheet")) cur_node = cur_node->next; // forward until reach spreadsheet
    cur_node = cur_node->xmlChildrenNode;
    while (cur_node != NULL && strcmp((char *) cur_node->name, "table")) cur_node = cur_node->next; // forward until reach table
    cur_node = cur_node->xmlChildrenNode;

    char * strvalue = NULL;
    char * st = NULL;
    char * strtype = NULL;
    char * value = NULL;
    char * strf;
    char * value_type = NULL;

    // here traverse table content
    while (cur_node != NULL) {
        if (! strcmp((char *) cur_node->name, "table-row")) {
            // we are inside a table-row
            // each of these is a row
            child_node = cur_node->xmlChildrenNode;
            r++;
            c=-1;

            while (child_node != NULL) {
               c++;
               if ((value_type = (char *) xmlGetProp(child_node, (xmlChar *) "value-type")) == NULL) { child_node = child_node->next; continue; };
               // each of these is table-cell (a column)

               strtype = value_type; // type

               //if (!strcmp(strtype, "time") //get time-value
               //TODO
               //if (!strcmp(strtype, "date") //get date-value
               //TODO
               if (!strcmp(strtype, "float")) {
                   char * formula = (char *) xmlGetProp(child_node, (xmlChar *) "formula");
                   if (formula != NULL) {
                       strf = str_replace (formula, "of:=","");
                       strcpy(formula, strf);
                       free(strf);
                       strf = str_replace (formula, "[.","");
                       strcpy(formula, strf);
                       free(strf);
                       strf = str_replace (formula, ";",",");
                       strcpy(formula, strf);
                       free(strf);
                       strf = str_replace (formula, ":.",":");
                       strcpy(formula, strf);
                       free(strf);
                       strf = str_replace (formula, "]","");
                       strcpy(formula, strf);
                       free(strf);
                       // we take some common function and adds a @ to them
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
                       swprintf(line_interp, FBUFLEN, L"let %s%d=%s", coltoa(c), r, formula);
                       xmlFree(formula);
                       formula = NULL;
                   } else {
                       value = (char *) xmlGetProp(child_node, (xmlChar *) "value"); // type
                       double l = atof((char *) value);
                       swprintf(line_interp, FBUFLEN, L"let %s%d=%.15f", coltoa(c), r, l);
                       xmlFree(value);
                       value = NULL;
                   }
                   send_to_interp(line_interp);
               } else if (!strcmp(strtype, "string") && !strcmp((char *) child_node->xmlChildrenNode->name, "p")) {
                   strvalue = (char *) xmlNodeGetContent(child_node->xmlChildrenNode);
                   st = str_replace (strvalue, "\"", "''");
                   clean_carrier(st); // we handle padding
                   swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, st);
                   send_to_interp(line_interp);
                   free(st);
                   xmlFree(strvalue);
                   strvalue = NULL;
               }
               child_node = child_node->next;

               xmlFree(value_type);
               value_type = NULL;
            }
        }
        cur_node = cur_node->next; // forward until reach table
    }
    int_deleterow(sh, sh->currow, 1); /* delete the first row */

    // free the document
    xmlFreeDoc(doc);

    // Free the global variables that may have been allocated by the parser
    xmlCleanupParser();

    free(content);

    // close zip file
    if (zip_close(za) == -1) {
        sc_error("cannot close zip archive `%s'", fname);
        return -1;
    }
#endif
    return 0;
}
