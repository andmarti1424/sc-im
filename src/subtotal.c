#include <wchar.h>
#include "sc.h"
#include "macros.h"
#include "cmds.h"
#include "shift.h"
#include "tui.h"

/*
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "yank.h"
#include "conf.h"
#include "color.h"
#include "xmalloc.h" // for scxfree
*/

/*
 *  r, c, rf, cf define the range of the data to be rearranged with subtotals.
 *  group_col is the column to be grouped by.
 *
 *  operation to be done over the group can be one of the following:
 *  @sum, @prod, @avg, @count, @stddev, @max, @min
 *
 *  ope_col is the operation column.
 *
 *  example command: :subtotal A @sum C
 *  if you want to replace a preexistent subtotals you should use: :rsubtotal A @sum C
 */
int subtotal(int r, int c, int rf, int cf, int group_col, char * operation, int ope_col, int replace_subtotals) {
    // check ope_col and group_col are valid
    if (ope_col < c || ope_col > cf || group_col < c || group_col > cf) return -1;

    // check if they are headers in first row
    struct ent * p, * q;
    int headers_in_first_row = 0;
    if ((p = *ATBL(tbl, r, ope_col)) && p->label &&
        (q = *ATBL(tbl, r+1, ope_col)) && ! q->label) headers_in_first_row=1;

    // group operation shall be done over text content !
    wchar_t cline [BUFFERSIZE];
    p = *ATBL(tbl, r + headers_in_first_row, group_col);
    swprintf(cline, BUFFERSIZE, L"+$%s", coltoa(group_col));

    // sort the range
    extern wchar_t interp_line[BUFFERSIZE];
    swprintf(interp_line, BUFFERSIZE, L"sort %s%d:", coltoa(c), r + headers_in_first_row);
    swprintf(interp_line + wcslen(interp_line), BUFFERSIZE, L"%s%d \"%ls\"", coltoa(cf), rf, cline);
    send_to_interp(interp_line);

    // traverse the range and replace subtotals
    //
    // TODO replace subtotals only if replace_subtotals is set
    int i, j, is_subtotal_row;
    extern int cmd_multiplier;
    //if (replace_subtotals) {
    for (i=r+headers_in_first_row; i <= rf; i++) {
        is_subtotal_row=0;
        for (j=c; j<cf; j++) {
            p = *ATBL(tbl, i, j);
            if (p && p->label && p->label[0] == '+' && p->label[1] == '@') { is_subtotal_row=1; break; }
        }
        if (is_subtotal_row) {
           cmd_multiplier = 1;
           shift(i, c, i, cf, L'k');
           i--;
           rf--;
        }
    }
    //}

    // traverse the range and add subtotals
    int new_rows = 0;
    wchar_t cmd[BUFFERSIZE];
    int row_start_range = r + headers_in_first_row;
    for (i=r+headers_in_first_row+1; i <= rf + new_rows + 1; i++) {
        p = *ATBL(tbl, i-1, group_col);
        q = *ATBL(tbl, i, group_col);

        // TODO ignore preexistance subtotals by default

        if ( (p && q && p->label && q->label && strcmp(q->label, p->label) != 0)
           || i == rf + new_rows + 1) {
           cmd_multiplier = 1;
           shift(i, c, i, cf, L'j');

           swprintf(cmd, BUFFERSIZE, L"rightstring %s%d = \"+%s(%s)\"", coltoa(group_col), i, operation, p->label);
           send_to_interp(cmd);

           swprintf(cmd, BUFFERSIZE, L"let %s%d = %s(%s%d:%s%d)", coltoa(ope_col), i, operation,
           coltoa(ope_col), row_start_range, coltoa(ope_col), i-1);
           send_to_interp(cmd);
           valueize_area(i, ope_col, i, ope_col);

           new_rows++;
           i++;
           row_start_range = i;
        }
    }

    return 0;
}

