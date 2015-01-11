#define BUFFERSIZE       256
#define MAX_MULTIPLIER   100   // efecto multiplicador
#define MAXSC            15    // MAXSC is the max length of a special key expressed as a string. ex. <C-a>
#define MAXMAPITEM       (MAXSC * 20) // max length of mapping part (in / out)
#define TIMEOUT_CURSES   300   // ms  Timeout para esperar una tecla en curses
#define CMDTIMEOUT       3000  // ms  Timeout para esperar un comando valido
#define COMPLETECMDTIMEOUT     (CMDTIMEOUT/8) // ms. Tiempo para completar un comando automáticamente
#define ESC_DELAY        25    // tiempo de espera de curses para caracteres de escape
#define RESCOL           4     // columns reserved for row numbers
#define RESROW           2     // rows reserved for prompt, error, and column numbers
#define NORMAL_MODE      0x01
#define INSERT_MODE      0x02
#define EDIT_MODE        0x04
#define COMMAND_MODE     0x08
#define VISUAL_MODE      0x16
// #define ctl(c) ((c)&037)
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

#ifdef BSD42
#define OKEY_BS          0x7f
#else
#define OKEY_BS          0x107
#endif

#define OKEY_HOME        0x106
#define OKEY_END         0x168
#define OKEY_PGUP        0x153
#define OKEY_PGDOWN      0x152
#define OKEY_PGDOWN      0x152
//#define metak(x) ((x) | 0x80)
#define LEFT             0
#define RIGHT            1

// used for is_single_command function
#define NO_CMD           0
#define EDITION_CMD      1
#define MOVEMENT_CMD     2

#ifndef INFO_MSG
    #define INFO_MSG     8
    #define ERROR_MSG    9
#endif



#include <ncurses.h>
extern WINDOW * input_win;
#define error(...)   set_ucolor(input_win, ERROR_MSG); wmove(input_win, 1, 0); wprintw(input_win, __VA_ARGS__); wclrtoeol(input_win); wrefresh(input_win);
#define info(...)    set_ucolor(input_win, INFO_MSG); mvwprintw(input_win, 1, 0, __VA_ARGS__); wclrtoeol(input_win); wrefresh(input_win);
#define debug(...)   set_ucolor(input_win, INFO_MSG); mvwprintw(input_win, 1, 0, __VA_ARGS__); wclrtoeol(input_win); wtimeout(input_win, -1); wgetch(input_win); wtimeout(input_win, TIMEOUT_CURSES); wrefresh(input_win);
