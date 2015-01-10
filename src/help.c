#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include "sc.h"
#include "macros.h"
#include "screen.h"
#include "string.h"
#include "color.h"
#include "utils/string.h"

void find_help(char * word, char order);

char * long_help[] = {
" ----------------------------------------------------------------------------------------------",
" This is a simple HELP page. Press «:q<Enter>» to go back to spreadsheet.                      ",
"                                                                                               ",
" You can use <UP> <DOWN> arrows keys, or 'j' 'k' keys to move throw text.                      ",
" <SPACE> key moves forward an entire page, while <C-f> and <C-b> moves half page down or up.   ",
" «G» moves to bottom, and <C-a> to the beginning of the text.                                  ",
" <ENTER> key scrolls one line down, while <DEL> key scrolls one line up, just like in less.    ",
" You can use the «/» command to search for a pattern in the help.                              ",
" ----------------------------------------------------------------------------------------------",
" SCIM modes:",
"       NORMAL_MODE      For navigation and common commands",
"       INSERT_MODE      For entering new cell content (values and expressions)",
"                        Submodes: = < > \\",
"       EDIT_MODE        'e' and 'E' keys are used for entering this mode. It is used for single",
"                        line -Vi-like- edition of cell content and expressions",
"       COMMAND_MODE     ':' key is used for entering this mode. It is used for entering special",
"                        commands and for quiting app and saving files",
"       VISUAL_MODE      'v' key is used for entering this mode. It is used for selecting a range of cells",
"        ----------------  ",
"",
"",
"       *NORMAL MODE*",
"",
"        Navigation commands: ",
"            j k l h     Move cursor down, up, right or left",
"            ^           Up to row 0 of the current column.",
"            #           Down to the last valid row of the current column.",
"            0           Back to column A.",
"            $           Forward to the last valid column of the current row.",
"            b           Back to the previous valid cell.",
"            w           Forward to the next valid cell.",
"            'c          Go to cell or select range marked previously as c. See the m command for details.",
"            gab24       Go to AB24 cell (no need to press ENTER).",
"            g0          moves to first visible column of screen.",
"            g$          moves to last visible column of screen.",
"            gM          moves to the middle column shown in screen.",
"            H           Go to first row seen on screen.",
"            L           Go to last row seen on screen.",
"            M           Go to middle row seen on screen.",
"            gg  , c-a:  Go to home (first cell of spreadsheet).",
"            G           Go to last valid cell of spreadsheet.",
"            gG          Go to last valid cell of spreadsheet.",
"            c-f , c-b:  Scrolls down and up full screen.",
"                        set variable half_page_scroll to 1 to scroll half page,",
"                        instead of full page. see :set command for details.",
"            c-e , c-y:  Scrolls down and up a line.",
"            zh          Scrolls left one column.",
"            zl          Scrolls right one column.",
"            zH          Scrolls left half page.",
"                        set variable half_page_scroll to '1' to scroll half a page, and set it to '0' to scroll a full page",
"            zL          Scrolls right half page.",
"                        set variable half_page_scroll to '1' to scroll half a page, and set it to '0' to scroll a full page",
"            zz          Scrolls up or down, until the selected cell shows up vertically in the center of screen.",
"            zt          Scrolls up or down, until the selected cell shows up vertically in the top of screen.",
"            zb          Scrolls up or down, until the selected cell shows up vertically in the bottom of screen.",
"            zm          Scrolls left or right, until the selected cell shows up in center of screen.",
"            Vir         Selects inner range",
"",
"",
"        Commands for handling cell content:",
"            x , dd      Deletes the current selected cell or range and save its content in yankbuffer",
"            m           Followed by any lowercase letter, marks the current cell or the selected range",
"                        with that letter.",
"                        NOTE: When a mark is changed, all ranges that use that mark are deleted.",
"            c           Copy a marked cell to the current cell, adjusting row and column references in its numeric",
"                        or string expression, if any.",
"            R           Followed by two lowercase letters that represent marks of cell,",
"                        creates a range using those marks and selects it.",
"                        NOTE: If a range already exists, its replaced with new values.",
"            { } |       align the content of a cell to the left, right or to center. if a range is selected, ",
"                        every cell of the range gets aligned.",
"",
"",
"            f+ , fk , f-UP:         Change cell format: Increase decimal precision.",
"            f- , fj , f-DOWN:       Change cell format: Decrease decimal precision.",
"            f< , fh , f-LEFT:       Change cell format: Decrease column width.",
"            f> , fl , f-RIGHT:      Change cell format: Incrementa column width.",
"            ir          inserts a row",
"            ic          inserts a column",
"            sk          shifts up current cell or current range",
"            sj          shifts down current cell or current range",
"            sh          shifts left current cell or current range",
"            sl          shifts right current cell or current range",
"            yy          yanks selected cell",
"            y           if a range is selected, it yanks the selected range",
"            yr          yanks current row",
"            yc          yanks current column",
"            p           paste previous yanked cell or range",
"                        if yr was used to yank a row, 'p' creates a new row below, and paste content there.",
"                        if yc was used to yank a column, 'p' creates a new col at the left of curcol, and paste content there.",
"            P           if yr was used to yank a row, 'P' creates a new row above, and paste content there.",
"                        if yc was used to yank a column, 'P' creates a new col at the right, and paste there.",
"            dr          deletes row under the cursor",
"            dc          deletes column under the cursor",
"            .           repeats last command",
"            -           decrease a numeric value of a cell or range",
"            +           increase a numeric value of a cell or range",
"",
"            u           UNDO last change",
"            c-f         REDO last change",
"                        NOTE: Events implemented for undo / redo:", 
"                        1. delete of cell or range content",
"                        2. cell input",
"                        3. cell edition",
"                        4. cell or range change in alignment",
"                        5. paste of a cell or range",
"                        6. range or cell shift with sh sj sk sl",
"                        7. insert row or column",
"                        8. delete of a row or column",
"                        9. paste of a row or column",
"                        10. zap(hide) / show a row or column",
"                        11. order of a range",
"                        12. change in the format of a range or cell",
"                        13. '-' and '+' commands in normal mode",
"                        14. lock and unlock of cells",
"                        15. datefmt command",
"",
"",
"        Other commands:",
"            ^L          Redraw the screen",
"            Zr          zap (hide) current row",
"            Zc          zap (hide) current column",
"            Sr          if a range is selected, it shows the rows hidden inside the range",
"            Sc          if a range is selected, it shows the cols hidden inside the range",
"            '/'         is an alias for ':int goto '.",
"                        If a number is given, SCIM will search for a cell containing that",
"                        number. Searches for either strings or numbers proceed",
"                        forward from the current cell, wrapping back to a0 at the end of",
"                        the table, and terminate at the current cell",
"                        if the string or number is not found.",
"                        Ex.: You can type '/4' to look for cells that has the value of 4.",
"                        Or you can type '/\"value\"' to look for cells that has",
"                       \"value\" as label content.",
"                        You can also quote a regular expression, and SCIM will search for",
"                        a cell containing a string matching the regular expression.",
"                        You can also search formatted numbers or expressions using regular",
"                        expressions by preceding the opening quotes",
"                        of  the regular expression with a '#' (for formatted numbers) or",
"                        a '%' (for expressions). These are handy",
"                        for searching for dates within a specified range or cells which",
"                        reference a given cell, for example, although",
"                        they are somewhat slower than searching through ordinary strings,",
"                        since all numbers must be formatted or",
"                        expressions decompiled on the fly during the search.",
"",
"            rl          Use this command to lock the current cell or a range of cells, i.e. make them",
"                        immune to any type of editing. A locked cell can't be changed in any way until it",
"                        is unlocked.",
"            ru          This command is the opposite of the rl command and thus unlocks a locked cell and",
"                        makes it editable.",
"            rv          valueize area: remove expressions from the affected range leaving only the values.",
"",
"",
"        *INSERT MODE*",
"            =           Enter a numeric constant or expression.",
"            <           Enter a left justified string or string expression.",
"            \\          Enter a centered label.",
"            >           Enter a right justified string or string expression.",
"            <TAB>       in insert mode, goes back to edit mode",
"            <LEFT>, <RIGHT>",
"                        Cursor movement with arrows keys",
"            Keys <ENTER>",
"                        Input of numbers, letters and operators",
"            <BS>, <DELETE>",
"                        Delete the chars after or before cursor",
"            <ESC>       Goes back to NORMAL MODE",
"            c-r {char}  Being {char} a mark of a cell or range, the range represented by that char gets",
"                        inserted in the command line",
"",
"",
"        *EDIT MODE*",
"            e           in normal mode, enters edit mode for editing numeric value of a cell",
"            E           in normal mode, enters edit mode for editing text value of a cell",
"            h           moves a char left",
"            l           moves a char right",
"            w           moves to beginning of next word",
"            e           if in end of a word, moves to end of next word. otherwise, moves to end of word under the cursor",
"            b           if in beginning of a word, moves to beginning of previous word. otherwise, moves to beginning",
"                        of word under the cursor",
"            0           moves to bol",
"            $           moves to eol",
"            f{char}     moves to next occurrence of {char} to the right",
"            r{char}     replaces char under the cursor with {char}",
"            R{word}     Each character you type replaces an existing character, starting with the character under the",
"                        cursor.",
"                        ESC key or ENTER key must be pressed when finished typing the new word.",
"",
"            de          deletes until end of word",
"            dw          deletes until beginning of next word",
"            db          if in beginning of a word, deletes until beginning of previous word. otherwise, deletes until",
"                        beginning of word under the cursor",
"            daw         deletes word under the cursor",
"            dE          deletes until end of WORD",
"            dW          deletes until beginning of next WORD",
"            dB          if in beginning of a word, deletes until beginning of previous WORD. otherwise, deletes until",
"                        beginning of WORD under the cursor",
"            daW         deletes WORD under the cursor",
"            dl          deletes char under the cursor",
"            d<RIGHT>    deletes char under the cursor",
"            dh          deletes char before the cursor",
"            d<LEFT>     deletes char before the cursor",
"",
"            ce          same as de but then enters insert mode",
"            cw          same as dw but then enters insert mode",
"            cb          same as db but then enters insert mode",
"            caw         same as daw but then enters insert mode",
"            cE          same as dE but then enters insert mode",
"            cW          same as dW but then enters insert mode",
"            cB          same as dB but then enters insert mode",
"            caW         same as daW but then enters insert mode",
"            cl          same as dl but then enters insert mode",
"            c<RIGHT>    same as d<RIGHT> but then enters insert mode",
"            ch          same as dh but then enters insert mode",
"            c<LEFT>     same as d<LEFT> but then enters insert mode",
"",
"            x           deletes char under the cursor",
"            X           deletes char before the cursor",
"            i or =      goes back to insert mode",
"            a           appends a char after the cursor",
"            s           deletes a char under the cursor and enters insert mode",
"            A           append at eol",
"            I           append at bol",
"            D           deletes entire line",
"            <SPACE>     adds a space under the cursor",
"            <ENTER>     confirm changes",
"",
"",
"        *COMMAND MODE*",
"            <LEFT>, <RIGHT>",
"                        moves cursor position with arrow keys",
"            <ENTER>",
"                        confirm a command",
"",
"            <DELETE>, <BS>",
"                        deletes char under the cursor or before cursor position",
"            <HOME>, <END>",
"                        moves cursor to beginning of line or end of line",
"",
"            <TAB>       completes a command that begins with the text already entered in the command line",
"",
"            <C-w>, <C-b>",
"                        moves forward or backwards a word",
"",
"            <C-f>",
"                        current cell format (if exists) gets pasted in commandline.",
"",
"            :w          saves current spreadsheet",
"            :w foo      save current spreadsheet with 'foo' filename",
"            :w! foo     save current spreadsheet with 'foo' filename, forcing rewrite if 'foo' file already exists",
"",
"            :h          show this help",
"            :help       show this help",
"",
"            :q          quit",
"            :quit       same as the previous",
"            :q!         quit ignoring changes since last save",
"            :quit!      same as the previous",
"",
"            :load name.sc   load file called 'name.sc' into SCIM database",
"            :load! name.sc  same as previous, but ignoring changes done over the current loaded spreadsheet.",
"",
"            :x          saves current spreadsheet and quit app",
"            :x foo      saves current spreadsheet with 'foo' filename, and quit app",
"            :x! foo     same as previous, but if 'foo' file already exists, it gets overwritten",
"",
"            :e csv      export current spreadsheet to csv format.",
"                        the name of the file that is created comes from the current open spreadsheet.",
"            :e tab      export current spreadsheet to tab delimited file",
"                        the name of the file that is created comes from the current open spreadsheet.",
"            :e csv name  export current spreadsheet to csv file. 'name' is the name of the file to be generated",
"            :e! csv name same as previous, but forcing the rewrite of file 'name' if it exists",
"            :e tab name  export current spreadsheet to tab delimited file. 'name' is the name of the file to be generated",
"            :e! tab name same as previous, but forcing the rewrite of file 'name' if it exists",
"                         NOTE: if a range is selected, its content are exported. if not, the entire spreadsheet is exported",
"",
"            :i csv name     import a csv delimited file called 'name' to SCIM.",
"            :i tab name     import a tab delimited file called 'name' to SCIM.",
"            :i xls name     import a xls file called 'name' to SCIM.",
"            :i xlsx name    import a xlsx file called 'name' to SCIM.",
"            :i! xlsx name   same as previous, but ignoring changes since last save.",
"",
"            :version    shows SCIM version number",
"",
"            :set        changes a configuration parameter in the format of key value",
"                        example:    :set half_page_scroll=0 other_key=its_value",
"                        if no key is specified, shows all configuration variables and its values",
"",
"            :showmaps   shows all mappings",
"",
"            :nmap string1 string2",
"                        Map the key sequence string1 to string2. This mapping takes effect only in NORMAL_MODE.",
"                        ex. :nmap \"H\" \":h<cr>\"",
"",
"            :imap string1 string2",
"                        Map the key sequence string1 to string2. This mapping takes effect only in INSERT_MODE.",
"                        ex. :imap \"C-f>\" \"format\"",
"",
"            :nnoremap string1 string2",
"                        this is the non recursive version of nmap. See NOTES on MAPPING below",
"",
"            :inoremap string1 string2",
"                        this is the non recursive version of imap. See NOTES on MAPPING below",
"",
"            :nunmap string1",
"                        the map sequence string1 that takes effect on NORMAL_MODE is removed",
"",
"            :iunmap string1",
"                        the map sequence string1 that takes effect on INSERT_MODE is removed",
"",
"            :format \"string\"",
"                        sets current cell format (or cells format of a selected range).",
"                        string parameter contains the format to use to convert the number,",
"                        and can be one or more of these:",
"", 
"                         #    Digit placeholder.  If the number has fewer digits on either",
"                              side of the decimal point than  there are '#' characters in",
"                              the format, the extra '#' characters are ignored.  The number",
"                              is rounded to the number of digit placeholders as there are",
"                              to the right of the decimal point.  If there are more digits",
"                              in the number than there are digit placeholders on the left",
"                              side of the decimal point, then those digits are displayed.",
"",
"                         0    Digit placeholder.  Same as for '#' except that the number",
"                              is padded with zeroes on either side of the decimal point.",
"                              The number of zeroes used in padding is determined by the",
"                              number of digit placeholders after the '0' for digits on",
"                              the left side of the decimal point and by the number of",
"                              digit placeholders before the '0' for digits on the right",
"                              side of the decimal point.",
"",
"                         .    Decimal point.  Determines how many digits are placed on",
"                              the right and left sides of the decimal point in the number.",
"                              Note that numbers smaller than 1 will begin with a decimal",
"                              point if the left side of the decimal point contains only",
"                              a '#' digit placeholder.  Use a '0' placeholder to get a",
"                              leading zero in decimal formats.",
"",
"                         %    Percentage.  For each '%' character in the format, the actual",
"                              number gets multiplied by 100 (only for purposes of formatting",
"                              -- the original number is left unmodified) and the '%' character",
"                              is placed in the same position as it is in the format.",
"",
"                         ,    Thousands separator.  The presence of a ',' in the format",
"                              (multiple commas are treated as one) will cause the number",
"                              to be formatted with a ',' separating each set of three digits",
"                              in the integer part of the number with numbering beginning",
"                              from the right end of the integer.",
"",
"                         d    its used to specify a date format that is applied to the numeric",
"                              value of a cell. (See also the DATE INPUT section below)",
"                              Date format strings are identified by the presence of a 'd'",
"                              in the first position. If this is present, the",
"                              remainder of the string is passed to the strftime() function,",
"                              and therefore uses the same conversion specifiers as strftime().",
"                              For more information on conversion specifiers for date format",
"                              strings, see the man page for strftime(3).",
"",
"                         E- E+ e- e+",
"                              Scientific format.  Causes the number to formatted in scientific",
"                              notation. The case of the 'E' or 'e' given is preserved. If",
"                              the format uses a '+', then the sign is always given for the",
"                              exponent value. If the format uses a '-', then the sign is",
"                              only given when the exponent value is negative. Note that if",
"                              there is no digit placeholder following the '+' or '-', then",
"                              that part of the formatted number is left out. In general,",
"                              there should be one or more digit placeholders after the '+'",
"                              or '-'.",
"",
"                         examples:     :format \"###,###,000\"",
"                                       :format \"d%d/%m/%Y\"",
"                                       :format \"####.####E+3\"",
"",
"",
"            :datefmt string   See DATE INPUT section below",
"",
"",
"            :sort range string",
"                         sort a range of cells with a given criteria.",
"                         The rows in the specified range will be sorted according",
"                         to a criteria given in the form of a string of characters.",
"                         This string, enclosed in double quotes, may comprise a",
"                         single criterion or multiple criteria in decreasing order",
"                         of precedence. Each criterion has three parts, all of which are mandatory.",
"                         The first part is a single character, which must be either + or - , which specifies",
"                         whether the sort should be done in ascending or descending order, respectively.",
"                         The second part, which is also a single character, must be either # or $ ,",
"                         and is used to specify whether the sort should be based",
"                         on the numeric portion or the string portion, respectively,",
"                         of the cells being used for the comparison.",
"                         The third part may be either one or two characters, and",
"                         must be alphabetic (case insensitive), and specifies",
"                         the column to be used when making the comparisons.",
"                         This column must be in the range being sorted.",
"                         Any number of criteria may be concatenated, and will be used in the order specified.",
"",
"                         example:         :sort C10:E13 \"+#D\"",
"",
"            :sort string",
"                         save as the previous but sort is done over a selected range",
"",
"",
"            :int string  sends a command to the interpreter",
"                         example:          :int goto B2",
"",
"",
"            :hidecol C   hide column C",
"            :hiderow 4   hide row 4",
"            :showcol C   show column C that was previously hidden",
"            :showrow 5   show row 5 that was previously hidden",
"            :hidecol A:C",
"                         hide a range of columns",
"            :hiderow 4:8",
"                         hide a range of rows",
"            :showcol A:C",
"                         show a range of columns that were previously hidden",
"            :showrow 5:7",
"                         show a range of rows that were previously hidden",
"",
"",
"            :showrows    if a range is selected, this command shows the rows hidden that are covered by the range",
"",
"",
"            :hiddenrows",
"                         show details of hidden rows",
"            :hiddencols",
"                         show details of hidden cols",
"",
"            :color <str> ",
"                         changes a color definition.",
"                         example of use:  :color type=HEADINGS bold=0 fg=BLACK bg=YELLOW",
"                         color parameters have to be one of the following:",
"                              type, fg, bg, bold, dim, reverse, standout, underline, blink.",
"                         the first three are mandatory",
"                         type has to be one of the following:",
"                              HEADINGS, MODE, NUMBER, STRING, EXPRESSION, CELL_ERROR,",
"                              CELL_NEGATIVE, CELL_SELECTION, CELL_SELECTION_SC,",
"                              INFO_MSG, ERROR_MSG, CELL_ID, CELL_FORMAT,",
"                              CELL_CONTENT, WELCOME, NORMAL, INPUT.",
"                         fg and bg have to be one of the following: WHITE, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN.",
"                         other parameters are booleans and its values have to be 1 or 0.",
"                         Instead of modifing a color at runtime, colors can be specified in:",
"                              a. the .scimrc file stored in your $HOME variable",
"                              b. in current .sc file.",
"                         The format is similar:",
"                         Ex.: color \"type=HEADINGS fg=BLACK bg=YELLOW bold=0\"",
"",
"",
"            :lock        Use this command to lock the current cell or a range of cells, i.e. make them",
"                         immune to any type of editing. A locked cell can't be changed in any way until it",
"                         is unlocked.",
"            :unlock      This command is the opposite of the :lock command and thus unlocks a locked cell and",
"                         makes it editable.",
"",
"            :valueize    valueize area: remove expressions from the affected range leaving only the values.",
"",
"            :! cmd       executes command 'cmd' in shell",
"",
"",
"            c-r {char}   Being {char} a mark of a cell or range, the range represented by that char gets",
"                         inserted in the command line",
"",
"            commandline history is stored in $HOME/.sciminfo",
"            c-p          goes back in command line history",
"            c-n          goes forward in command line history",
"            <UP>         goes back in command line history",
"            <DOWN>       goes forward in command line history",
"",
"",
"",
"        *VISUAL MODE*",
"            This mode is used for selecting a range of cells to do an operation",
"            At entering to this mode, top left limit of the selected range, and the bottom right limit is set to",
"            current row and column.",
"            The following movements commands helps to complete a selection in different ways.",
"            j k l h     Move cursor down, up, right or left",
"            0           Moves to column A",
"            $           Moves forward to the last valid column of the current row.",
"            #           Moves down to the last valid row of the current column.",
"            ^           Moves up to row 0 of the current column.",
"            'a          Moves to cell or select range marked previously as c.",
"                        See the m command for details.",
"            c-f c-b     Increase selection down or up a full screen.",
"                        set variable half_page_scroll to 1 to scroll half a page,",
"                        instead of full page. see :set command for details.",
"            c-a         Moves to first cell of spreadsheet.",
"",
"            y           yanks selected cell or range and exit VISUAL MODE.",
"            x , dd      Deletes the current range and save its content in yankbuffer",
"            H           Moves to to first row seen on screen.",
"            L           Moves to last row seen on screen.",
"            M           Moves to middle row seen on screen.",
"            w           Moves forward to the next valid cell.",
"            b           Moves back to the previous valid cell.",
"            G           Moves to last valid cell of spreadsheet.",
"",
"            :           Enters COMMAND MODE preserving the range selection, so a special command can be entered.",
"            Zr          Zaps (hides) the rows covered by the selected range.",
"            Zc          Zaps (hides) the columns covered by the selected range.",
"            Sr          Shows rows that are hidden and that get covered by the selected range.",
"            Sc          Shows columns that are hidden and that get covered by the selected range.",
"            { } |       Aligns the content of the cells covered by the selected range, to right, left or to center.",
"            rl          Use this command to lock the current cell or a range of cells, i.e. make them",
"                        immune to any type of editing. A locked cell can't be changed in any way until it",
"                        is unlocked.",
"            ru          This command is the opposite of the rl command and thus unlocks a locked cell and",
"                        makes it editable.",
"            rv          valueize area: remove expressions from the affected range leaving only the values.",
"            m           Followed by any lowercase letter, marks the selected range with that letter.",
"                        NOTE: When a mark is changed, all ranges that use that mark are deleted.",
"",
"        *MAPPING*",
"            mapping can be done in any SCIM file or in .scimrc file in current home directory.",
"            maps can be added with nmap and imap commands, and they can be removed with",
"            nunmap, iunmap commands",
"            ex.: ",
"                nmap \"d\" \":h<cr>\"     ->   d maps to ':help<cr>' in normal mode",
"                imap \"f\" \"format\"     ->   f maps to format string in insert mode",
"",
"            Some notes:",
"            Left and Right sequence of a mapping cannot contain numbers.",
"            <ESC> key cannot be mapped. But other special keys can. Those are <cr> <C-x>..",
"            If a map sequence that exists is re-mapped, it gets replaced with the new one.",
"            Mapping is recursive by default. The non recursive versions of nmap and imap are",
"            the nnoremap and inoremap commands. An example of use:",
"                nmap \"a\" \"b\"",
"                nnoremap \"b\" \"j\"",
"                nmap \"j\" \":h<cr>\"",
"            with this, 'a' maps to 'j', and only 'j' maps to ':h<cr>'
"
"",
"",
"        *COMMAND MULTIPLIER*",
"            Commands in NORMAL MODE, VISUAL MODE or EDIT MODE can be multiplied if a number is previously typed.",
"            Ex. '4j' in NORMAL MODE, translates to 4 times 'j'.",
"            Ex. '4yr' in NORMAL MODE, yanks current row and the 3 rows below it.",
"            NOTE: the 'x' command in VISUAL MODE and shift commands in VISUAL MODE and NORMAL MODE, when a range is",
"            selected, cannot be multiplied.", 
"",
"",
"        *DATES INPUT*:",
"            Dates are internally stored in SCIM as numeric values, and they are visually",
"            seen as dates if a date format is applied to the cells that stores them.",
"            They can be entered as a label and then use the :datefmt special command to",
"            convert it to a numeric value and apply a format.",
"            Other option is to enter the numeric value and apply a format with the",
"            :format special command.",
"",
"            The :datefmt command takes the cell's text content that represents a date,",
"            and uses a given format to set the numeric value of the cell.",
"            Its format is ':datefmt \"format\"', being format a strftime compatible one.",
"            This command then aplies that format to the cell (as it could be done with",
"            the :format function) so that it can be visually seen as a date.",
"",
"            example:      :datefmt \"%d/%m/%Y\"",
"",
"",
"        Other configuration variables:",
"            - autocalc",
"            set variable 'autocalc' to recalculate automatically, or set it to '0' to do it manually upon execution of",
"            '@' command",
"",
"            - numeric",
"            setting variable 'numeric' to '1', a digit starts a numeric value",
"            setting the variable to '0', a digit acts as a command multiplier",
"",
"            - newline_action",
"            set variable 'newline_action' to 'j' to move cursor down after an entry.",
"            set it to 'l' to move it right, or set it to '0' to expect no action.",
"",
"",
"",

(char *) 0
};

static int pscreen(char *screen[]);
static int delta = 0;
static int max = (sizeof(long_help) / sizeof(char *));
static int look_result = -1;
static char word_looked[50] = "";

void help() {
    delta = 0;

    wmove(input_win, 0,0);
    wclrtobot(input_win);
    wrefresh(input_win);

    set_ucolor(main_win, NORMAL);
    wtimeout(input_win, -1);
    noecho();
    curs_set(0);
    int quit_help_now = 0;

    int option;

    //while( (option = pscreen(long_help)) != 'q' && option != 'Q' && option != OKEY_ESC ) {
    while( ! quit_help_now ) {
        option = pscreen(long_help);
        look_result = -1;
        switch (option) {

        case OKEY_ENTER:
        case OKEY_DOWN:
        case 'j':
            if (max > delta + LINES + 1) delta++;
            break;

        case OKEY_DEL:
        case OKEY_UP:
        case 'k':
            if (delta) delta--;
            break;

        case ' ':
            if (max > delta + LINES + LINES) delta += LINES;
            else if (max > delta + LINES) delta = max - 1 - LINES;
            break;

        case ctl('b'):
            if (delta - LINES/2 > 0) delta -= LINES/2;
            else if (delta) delta = 0;
            break;

        case ctl('f'):
            if (delta + LINES + LINES/2 < max) delta += LINES/2;
            else if (max > delta + LINES) delta = max - 1 - LINES;
            break;

        case 'n':
            if (strlen(word_looked)) find_help(word_looked, 'f');
            break;

        case 'N':
            if (strlen(word_looked)) find_help(word_looked, 'b');
            break;

        case ':':
            curs_set(1);
            char hline [100];
            hline[0]='\0';
            mvwprintw(input_win, 0, rescol, ":%s", hline);
            wclrtoeol(input_win);
            wrefresh(input_win);

            int d = wgetch(input_win);
            while (d != OKEY_ENTER && d != OKEY_ESC) {
                if (d == OKEY_BS) {
                    del_char(hline, strlen(hline) - 1);
                } else {
                    sprintf(hline, "%s%c", hline, d);
                }
                mvwprintw(input_win, 0, rescol, ":%s", hline);
                wclrtoeol(input_win);
                wrefresh(input_win);
                d = wgetch(input_win);
            }
            if (d == OKEY_ENTER && ( strcmp(hline, "q") == 0 || strcmp(hline, "quit") == 0 )) {
                quit_help_now = TRUE;
            }
            break;

        case '/':
            curs_set(1);
            word_looked[0]='\0';
            mvwprintw(input_win, 0, rescol, "/%s", word_looked);
            wrefresh(input_win);
            d = wgetch(input_win);
            while (d != OKEY_ENTER && d != OKEY_ESC) {
                if (d == OKEY_BS) {
                    del_char(word_looked, strlen(word_looked) - 1);
                } else {
                    sprintf(word_looked, "%s%c", word_looked, d);
                }
                mvwprintw(input_win, 0, rescol, "/%s", word_looked);
                wclrtoeol(input_win);
                wrefresh(input_win);
                d = wgetch(input_win);
            }

            if (d == OKEY_ENTER && strlen(word_looked)) {
                find_help(word_looked, 'f');
            }
            mvwprintw(input_win, 0, rescol, "");
            wclrtoeol(input_win);
            wrefresh(input_win);
            curs_set(0);
            break;

        case 'G':
            delta = max - 1 - LINES;
            break;

        case ctl('a'):
            delta = 0;
            break;
        }
    }
    wtimeout(input_win, TIMEOUT_CURSES);
    wmove(input_win, 0,0);
    wclrtobot(input_win);
    wmove(main_win, 0,0);
    wclrtobot(main_win);
    wrefresh(main_win);
    //wrefresh(input_win);
    //show_header(input_win);
    //update();
}

void find_help(char * word, char order) {
    int i;
    if (order == 'f') {        // forward
        for (i = delta + 1; i < max - 1; i++) {
            if ((look_result = str_in_str(long_help[i], word)) >= 0) {
                delta = i;
                info("");
                break;
            }
        }
    } else if (order == 'b') { // backwards
        for (i = delta - 1; i > 0; i--) {
            if ((look_result = str_in_str(long_help[i], word)) >= 0) {
                delta = i;
                info("");
                break;
            }
        }
    }

    if (look_result == -1) {
        info("Pattern not found.");
    }
    set_ucolor(input_win, NORMAL);
    return;
}

static int pscreen(char * screen[]) {
    int lineno, c = 0, bold = 0;

    for (lineno = 0; screen[lineno + delta] && lineno < LINES; lineno++) {
        if (strlen(word_looked)) look_result = str_in_str(screen[lineno + delta], word_looked);

        wmove(main_win, lineno, 0);
        wclrtoeol(main_win);

        for (c=0; c < strlen(screen[lineno + delta]); c++)  {
            if (screen[lineno + delta][c] == '*') bold = ! bold;
            bold ? set_ucolor(main_win, CELL_SELECTION_SC) : set_ucolor(main_win, NORMAL);

            if (screen[lineno + delta][c] == '*') {
                  set_ucolor(main_win, NORMAL);
            //    //mvwprintw(main_win, lineno+2, c, " ");
                continue;
            } else if (lineno == 0 && look_result != -1 && c >= look_result &&
                c < look_result + strlen(word_looked) ) {
                  set_ucolor(main_win, CELL_SELECTION_SC);
            }
            mvwprintw(main_win, lineno, c, "%c", screen[lineno + delta][c]);
        }
        
        //mvwprintw(main_win, lineno, 0, "%s", (screen[lineno + delta]));
        wclrtoeol(main_win);
    }
    if (lineno < LINES) {
        wmove(main_win, lineno+1, 0);
        wclrtobot(main_win);
    }
     
    (void) wrefresh(main_win);
    return wgetch(input_win);
}
