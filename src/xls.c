#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "macros.h"
#include "xls.h"
#include "utils/string.h"

/*
 * xls.h is part of libxls. make sure its installed and headers are in path.
 * build must be done with '-lxlsreader'
 */
#ifdef XLS
#include <xls.h>
#endif

/*
 * this functions loads an excel file into tbl.
 * As SC-IM still does not handle multiple sheets,
 * if the excel file has multiple sheets, only the first one is read.
 * this function returns -1 in case of error
 */
int open_xls(char * fname, char * encoding) {
#ifdef XLS

    // Set date format reading LOCALE
    char fmt[15] = "%d/%m/%Y";

    #ifdef USELOCALE
    #include <locale.h>
    #include <langinfo.h>
    char * loc = NULL;
    char * f = NULL;
    loc = setlocale(LC_TIME, "");

    if (loc != NULL) {
        f = nl_langinfo(D_FMT);
        strcpy(fmt, f);
    }
    #endif

    // Read XLS file
    xlsWorkBook * pWB;
    xlsWorkSheet * pWS;
    WORD r, c;
    pWB = xls_open(fname, encoding);

    wchar_t line_interp[FBUFLEN] = L"";
    struct ent * n;

    if (pWB == NULL) {
        sc_error("Error loading %s", fname);
        return -1;
    }

    pWS = xls_getWorkSheet(pWB, 0); //only the first sheet
    if (pWS == NULL) return -1;
    xls_parseWorkSheet(pWS);

    for (r = 0; r <= pWS->rows.lastrow; r++) { // rows
        for (c = 0; c <= pWS->rows.lastcol; c++) { // cols
            xlsCell * cell = xls_cell(pWS, r, c);
            if ((! cell) || (cell->isHidden)) continue;

            // TODO enable rowspan ?
            //if (cell->rowspan > 1) continue;

            struct st_xf_data * xf = &pWB->xfs.xf[cell->xf];

            //sc_debug("%d %d fmt:%d id:%x %d %d", r, c, xf->format, cell->id, cell->d, cell->l);

            // these are dates
            if (((xf->format >= 14 && xf->format <= 22) ||
                (xf->format >= 165 && xf->format <= 180) ||
                xf->format == 278 || xf->format == 185 || xf->format == 196 || xf->format
                == 217 || xf->format == 326 )
               && cell->id != 0x06
               //&& cell->id != 0x27e
               && cell->id != 0x0BD
               && cell->id != 0x203 ) {

                swprintf(line_interp, FBUFLEN, L"let %s%d=%.15g", coltoa(c), r, (cell->d - 25569) * 86400);
                send_to_interp(line_interp);
                n = lookat(r, c);
                n->format = 0;
                char * s = scxmalloc((unsigned)(strlen(fmt) + 2));
                sprintf(s, "%c", 'd');
                strcat(s, "%d/%m/%Y");
                n->format = s;
                continue;

            // display the value of the cell (either numeric or string)
            } else if (cell->id == 0x27e || cell->id == 0x0BD || cell->id == 0x203) {
                swprintf(line_interp, FBUFLEN, L"let %s%d=%.15g", coltoa(c), r, cell->d);

            } else if (cell->id == 0x06) { // formula
                if (cell->l == 0) {        // its a number
                    swprintf(line_interp, FBUFLEN, L"let %s%d=%.15g", coltoa(c), r, cell->d);
                } else {
                    if (!strcmp((char *) cell->str, "bool")) {          // its boolean, and test cell->d
                        swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, (int) cell->d ? "true" : "false");
                    } else if (! strcmp((char *) cell->str, "error")) { // formula is in error
                        swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, "error"); //FIXME
                    } else {
                        swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, (char *) cell->str);
                    }
                }

            } else if (cell->str != NULL) {
                int pad_pos;
                if ((pad_pos = str_in_str((char *) cell->str, "\n")) != -1) ((char *) cell->str)[pad_pos] = '\0'; // For spanning
                // clean_carrier((char *) cell->str); // For spanning
                swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, (char *) cell->str);
            } else {
                swprintf(line_interp, FBUFLEN, L"label %s%d=\"%s\"", coltoa(c), r, "");
            }
            send_to_interp(line_interp);
        }
    }
    xls_close_WS(pWS);
    xls_close_WB(pWB);
    auto_justify(0, maxcols, DEFWIDTH);
    return 0;
#else
    return -1;
#endif
}
