#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "sc.h"
#include "macros.h"
#include "tui.h"
#include "clipboard.h"
#include "cmds.h"
#include "file.h"
#include "conf.h"
#include "utils/string.h"

int paste_from_clipboard() {
    if (! strlen(get_conf_value("default_paste_from_clipboard_cmd"))) return -1;

    // create tmp file
    char template[] = "/tmp/sc-im-clipboardXXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) {
        sc_error("Error while pasting from clipboard");
        return -1;
    }

    // get temp file pointer based on temp file descriptor
    //FILE * fpori = fdopen(fd, "w");

    // copy content from clipboard to temp file
    char syscmd[PATHLEN + strlen(get_conf_value("default_paste_from_clipboard_cmd")) + 1];
    sprintf(syscmd, "%s", get_conf_value("default_paste_from_clipboard_cmd"));
    sprintf(syscmd + strlen(syscmd), " >> %s", template);
    system(syscmd);

    // traverse the temp file
    FILE * fp = fdopen(fd, "r");
    char line_in[BUFFERSIZE];
    wchar_t line_interp[FBUFLEN] = L"";
    int c, r = currow;
    char * token;
    char delim[2] = { '\t', '\0' } ;

    while ( ! feof(fp) && (fgets(line_in, sizeof(line_in), fp) != NULL) ) {
        // Split string using the delimiter
        token = xstrtok(line_in, delim);
        c = curcol;
        while( token != NULL ) {
            if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
            clean_carrier(token);
            //char * st = str_replace (token, " ", ""); //trim
            char * st = token;
            if (strlen(st) && isnumeric(st))
                swprintf(line_interp, BUFFERSIZE, L"let %s%d=%s", coltoa(c), r, st);
            else
                swprintf(line_interp, BUFFERSIZE, L"label %s%d=\"%s\"", coltoa(c), r, st);
            if (strlen(st)) send_to_interp(line_interp);
            c++;
            token = xstrtok(NULL, delim);
            free(st);
            if (c > maxcol) maxcol = c;
        }
        r++;
        if (r > maxrow) maxrow = r;
        if (r > MAXROWS - GROWAMT - 1 || c > ABSMAXCOLS - 1) break;
    }
    sc_info("Content pasted from clipboard");

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(template);
    return 0;
}

int copy_to_clipboard(int r0, int c0, int rn, int cn) {
    if (! strlen(get_conf_value("default_copy_to_clipboard_cmd"))) return -1;

    // create tmp file
    char template[] = "/tmp/sc-im-clipboardXXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) {
        sc_error("Error while copying to clipboard");
        return -1;
    }

    // get temp file pointer based on temp file descriptor
    FILE * fp = fdopen(fd, "w");

    // save plain text to file
    save_plain(fp, r0, c0, rn, cn);
    fclose(fp);

    // copy to clipboard
    char syscmd[PATHLEN + strlen(get_conf_value("default_copy_to_clipboard_cmd")) + 1];
    sprintf(syscmd, "%s", get_conf_value("default_copy_to_clipboard_cmd"));
    sprintf(syscmd + strlen(syscmd), " %s", template);
    system(syscmd);

    sc_info("Content copied to clipboard");

    // close file descriptor
    close(fd);

    // remove temp file
    unlink(template);

    return 0;
}

// file shall be already open
int save_plain(FILE * fout, int r0, int c0, int rn, int cn) {
    int row, col;
    register struct ent ** pp;
    wchar_t out[FBUFLEN] = L"";
    char num [FBUFLEN] = "";
    char text[FBUFLEN] = "";
    char formated_s[FBUFLEN] = "";
    int res = -1;
    int align = 1;

    for (row = r0; row <= rn; row++) {
        // ignore hidden rows
        //if (row_hidden[row]) continue;

        for (pp = ATBL(tbl, row, col = c0); col <= cn; col++, pp++) {
            // ignore hidden cols
            //if (col_hidden[col]) continue;

            if (*pp) {
                num [0] = '\0';
                text[0] = '\0';
                out [0] = L'\0';
                formated_s[0] = '\0';
                res = -1;
                align = 1;

                // If a numeric value exists
                if ( (*pp)->flags & is_valid) {
                    res = ui_get_formated_value(pp, col, formated_s);
                    // res = 0, indicates that in num we store a date
                    // res = 1, indicates a format is applied in num
                    if (res == 0 || res == 1) {
                        strcpy(num, formated_s);
                    } else if (res == -1) {
                        sprintf(num, "%.*f", precision[col], (*pp)->v);
                    }
                }

                // If a string exists
                if ((*pp)->label) {
                    strcpy(text, (*pp)->label);
                    align = 1;                                // right alignment
                    if ((*pp)->flags & is_label) {            // center alignment
                        align = 0;
                    } else if ((*pp)->flags & is_leftflush) { // left alignment
                        align = -1;
                    } else if (res == 0) {                    // res must ¿NOT? be zero for label to be printed
                        text[0] = '\0';
                    }
                }
                if (! atoi(get_conf_value("copy_to_clipboard_delimited_tab"))) {
                    pad_and_align(text, num, fwidth[col], align, 0, out);
                    fwprintf(fout, L"%ls", out);
                } else if ( (*pp)->flags & is_valid) {
                    fwprintf(fout, L"%s\t", num);
                } else if ( (*pp)->label) {
                    fwprintf(fout, L"%s\t", text);
                }
            } else if (! atoi(get_conf_value("copy_to_clipboard_delimited_tab"))) {
                fwprintf(fout, L"%*s", fwidth[col], " ");
            } else {
                fwprintf(fout, L"\t");
            }
        }
        if (row != rn) fwprintf(fout, L"\n");
    }
    return 0;
}
