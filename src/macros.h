#define BUFFERSIZE       256
#define MAX_MULTIPLIER   100
#define MAXSC            15    // MAXSC is the max length of a special key expressed as a string. ex. <C-a>
#define MAXMAPITEM       (MAXSC * 20) // max length of mapping part (in / out)
#define TIMEOUT_CURSES   300   // ms  curses input timeout
#define CMDTIMEOUT       3000  // ms  valid command timeout
#define COMPLETECMDTIMEOUT     (CMDTIMEOUT/8)
#define ESC_DELAY        25    // Escape timeout
#define RESCOL           4     // columns reserved for row numbers
#define RESROW           2     // rows reserved for prompt, error, and column numbers
#define NORMAL_MODE      0x01
#define INSERT_MODE      0x02
#define EDIT_MODE        0x04
#define COMMAND_MODE     0x08
#define VISUAL_MODE      0x16
#define ctl(x)           ((x) & 0x1f)
#define uncl(x)           (0x60 | ((x) & 0x1f))
#define OKEY_ESC         '\033'
#define OKEY_TAB         '\011'
#define OKEY_ENTER       10
#define OKEY_LEFT        0x104
#define OKEY_RIGHT       0x105
#define OKEY_DOWN        0x102
#define OKEY_UP          0x103
#define OKEY_DEL         0x14a
#define OKEY_BS          0x107
#define OKEY_BS2         0x7f   // some BSDs, Linux distros, SSH/tmux configs
#define OKEY_HOME        0x106
#define OKEY_END         0x168
#define OKEY_PGUP        0x153
#define OKEY_PGDOWN      0x152

//#define metak(x) ((x) | 0x80)
#define LEFT             0
#define RIGHT            1

// used for is_single_command function
#define NO_CMD           0
#define EDITION_CMD      1
#define MOVEMENT_CMD     2

#include <ncurses.h>
extern WINDOW * input_win;

#define N_INIT_PAIRS      19

#define BLACK             COLOR_BLACK
#define RED               COLOR_RED
#define GREEN             COLOR_GREEN
#define YELLOW            COLOR_YELLOW
#define BLUE              COLOR_BLUE
#define MAGENTA           COLOR_MAGENTA
#define CYAN              COLOR_CYAN
#define WHITE             COLOR_WHITE

#define DEFAULT            0
#define HEADINGS           1
#define WELCOME            2
#define CELL_SELECTION     3
#define CELL_SELECTION_SC  4
#define NUMB               5
#define STRG               6
#define DATEF              7
#define EXPRESSION         8
#define INFO_MSG           9
#define ERROR_MSG         10
#define MODE              11
#define CELL_ID           12
#define CELL_FORMAT       13
#define CELL_CONTENT      14
#define INPUT             15
#define NORMAL            16
#define CELL_ERROR        17
#define CELL_NEGATIVE     18
#define DEBUG_MSG         19

#define FG                 0
#define BG                 1
#define ATTR               2


#define sc_error(x, ...)     sc_msg(x, ERROR_MSG, ##__VA_ARGS__)
#define sc_debug(x, ...)     sc_msg(x, DEBUG_MSG, ##__VA_ARGS__)
#define sc_info(x, ...)     sc_msg(x, INFO_MSG, ##__VA_ARGS__)
