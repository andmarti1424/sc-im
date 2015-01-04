#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "macros.h"
#include "xls.h"

// xls.h is part of libxls. make sure its installed and headers are in path.
// build must be done with '-lxlsreader'
#ifdef XLS
#include <xls.h>
#endif

// this functions loads an excel file into tbl.
// As SCIM still does not handle multiple sheets,
// if the excel file has multiple sheets, only the first one is read.
// this function returns -1 in case of error
int open_xls(char * fname, char * encoding) {
#ifdef XLS
    xlsWorkBook * pWB;
    xlsWorkSheet * pWS;
    WORD r, c;
    pWB = xls_open(fname, encoding);
    
    char line_interp[FBUFLEN] = "";    
    struct ent * n;

    if (pWB == NULL) {
        error("Error loading %s", fname);
        return -1;
    }

    pWS = xls_getWorkSheet(pWB, 0); //only the first sheet
    xls_parseWorkSheet(pWS);

    for (r = 0; r <= pWS->rows.lastrow; r++) { // rows
        for (c = 0; c <= pWS->rows.lastcol; c++) { // cols
            xlsCell * cell = xls_cell(pWS, r, c);
            if ((! cell) || (cell->isHidden)) continue;

            // TODO rowspan
            struct st_xf_data * xf=&pWB->xfs.xf[cell->xf];

            if (xf->format == 165) { // date
            // if (cell->id == 0x27e && (cell->xf == 24 || cell->xf == 26
            // || cell->xf == 28 || cell->xf == 30 || cell->xf == 32 )) {
                sprintf(line_interp, "let %s%d=%.15g", coltoa(c), r, (cell->d - 25568) * 86400);
                send_to_interp(line_interp);
                n = lookat(r, c);
                n->format = 0;
                char * s = scxmalloc((unsigned)(strlen("%d/%m/%Y") + 2));
                sprintf(s, "%c", 'd');
                strcat(s, "%d/%m/%Y");
                n->format = s;
                continue;

            // display the value of the cell (either numeric or string)
            } else if (cell->id == 0x27e || cell->id == 0x0BD || cell->id == 0x203) {
                sprintf(line_interp, "let %s%d=%.15g", coltoa(c), r, cell->d);

            } else if (cell->id == 0x06) { // formula
                if (cell->l == 0) {        // its a number
                    sprintf(line_interp, "let %s%d=%.15g", coltoa(c), r, cell->d);
                } else {
                    if (!strcmp((char *) cell->str, "bool")) {          // its boolean, and test cell->d
                        sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, (int) cell->d ? "true" : "false");
                    } else if (! strcmp((char *) cell->str, "error")) { // formula is in error
                        sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, "error"); //FIXME
                    } else {
                        sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, (char *) cell->str);
                    }
                }
            
            } else if (cell->str != NULL) {
                sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, (char *) cell->str);
            } else {
                sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, "");
            }
            send_to_interp(line_interp);
        }
    }
    xls_close_WS(pWS);
    xls_close(pWB);
    auto_justify(0, maxcol, DEFWIDTH);
    return 0;
#else
    return -1;
#endif

}
