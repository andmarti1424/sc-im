#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "macros.h"

// This is part of libxls. make sure its installed and headers are in path.
// build must be done with '-lxlsreader'
#include <xls.h>

// this functions loads an excel file into tbl.
// As SCIM still does not handle multiple sheets,
// if the excel file has multiple sheets, only the first one is read.
// encoding = "UTF-8";
// this function returns -1 in case of error
int open_xls(char * fname, char * encoding) {
#ifndef XLS
    return -1;
#endif
    xlsWorkBook * pWB;
    xlsWorkSheet * pWS;
    char line_interp[FBUFLEN] = "";    
    WORD cellRow, cellCol;
    pWB = xls_open(fname, encoding);

    if (pWB == NULL) return -1;

    pWS = xls_getWorkSheet(pWB, 0); //only the first sheet
    xls_parseWorkSheet(pWS);

    for (cellRow = 0; cellRow <= pWS->rows.lastrow; cellRow++) { // rows
        for (cellCol = 0; cellCol <= pWS->rows.lastcol; cellCol++) { // cols
            xlsCell * cell = xls_cell(pWS, cellRow, cellCol);
            if ((! cell) || (cell->isHidden)) continue;

            // TODO rowspan
            
            // display the value of the cell (either numeric or string)
            if (cell->id == 0x27e || cell->id == 0x0BD || cell->id == 0x203) {
                //info("%.15g", cell->d);
                sprintf(line_interp, "let %s%d=%.15g", coltoa(cellCol), cellRow, cell->d);

            } else if (cell->id == 0x06) { // formula
                if (cell->l == 0) { // its a number
                    //info("%.15g", cell->d);
                    sprintf(line_interp, "let %s%d=%.15g", coltoa(cellCol), cellRow, cell->d);
                } else {
                    if (!strcmp((char *) cell->str, "bool")) { // its boolean, and test cell->d
                        //info("%s", (int) cell->d ? "true" : "false");
                        sprintf(line_interp, "label %s%d=\"%s\"", coltoa(cellCol), cellRow, (int) cell->d ? "true" : "false");
                    } else if (! strcmp((char *) cell->str, "error")) { // formula is in error
                        //info("*error*");
                        sprintf(line_interp, "label %s%d=\"%s\"", coltoa(cellCol), cellRow, "error"); //FIXME
                    } else {
                        //info("%s", (char *) cell->str);
                        sprintf(line_interp, "label %s%d=\"%s\"", coltoa(cellCol), cellRow, (char *) cell->str);
                    }
                }
            
            } else if (cell->str != NULL) {
                //info((char *)cell->str);
                sprintf(line_interp, "label %s%d=\"%s\"", coltoa(cellCol), cellRow, (char *) cell->str);
            } else {
                //info("");
                sprintf(line_interp, "label %s%d=\"%s\"", coltoa(cellCol), cellRow, "");
            }
            send_to_interp(line_interp);
        }
    }
    xls_close_WS(pWS);
    xls_close(pWB);
    return 0;
}
