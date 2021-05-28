# Specify the name of the resulting executable file
name = sc-im

# The base directory where everything should be installed.
prefix  = /usr/local

EXDIR   = $(prefix)/bin
HELPDIR = $(prefix)/share/$(name)
LIBDIR  = $(prefix)/share/doc/$(name)

# This is where the man page goes.
MANDIR  = $(prefix)/share/man/man1

# History directory, relative to $HOME
HISTORY_DIR= .cache
HISTORY_FILE=sc-iminfo
# Configuration directory, relative to $HOME
CONFIG_DIR= .config/sc-im
CONFIG_FILE=scimrc

# Change these to your liking or use `make CC=gcc` etc
#CC   = cc
#YACC = bison -y
#SED  = sed

LDLIBS += -lm

CFLAGS += -Wall -g
CFLAGS += -DNCURSES
CFLAGS += -D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE
CFLAGS += -DSNAME=\"$(name)\"
CFLAGS += -DHELP_PATH=\"$(HELPDIR)\"
CFLAGS += -DLIBDIR=\"$(LIBDIR)\"

# Sets default pager, e.g. 'less' or 'more'
CFLAGS += -DDFLT_PAGER=\"less\"
# Sets default editor. Its use in case EDITOR env variable is not set
CFLAGS += -DDFLT_EDITOR=\"vim\"
# Comment out to disable color support
CFLAGS += -DUSECOLORS
# Command history file, relative to HISTORY_DIR directory. Comment out to disable commandline history
CFLAGS += -DHISTORY_FILE=\"$(HISTORY_FILE)\" -DHISTORY_DIR=\"$(HISTORY_DIR)\"
# Configuration file, relative to CONFIG_DIR directory
CFLAGS += -DCONFIG_FILE=\"$(CONFIG_FILE)\" -DCONFIG_DIR=\"$(CONFIG_DIR)\"
# Input mode history. Same as previous, but for insert mode commands
CFLAGS += -DINS_HISTORY_FILE=\"sc-iminfo\"
# Comment out to disable undo/redo support
CFLAGS += -DUNDO
# Maximum number of rows in spreadsheet. Up to 1048576
CFLAGS += -DMAXROWS=65536
# Used for date formatting with C-d shortcut using you local d_fmt
CFLAGS += -DUSELOCALE
# Comment out to enable mouse support on virtual terminal.
CFLAGS += -DMOUSE

# Clipboard support is OS dependent.
#
# Choose one of the following commands for copying to different clipboards:
# You can later change it at runtime.
#to copy to tmux clipboard:
#CFLAGS += -DDEFAULT_COPY_TO_CLIPBOARD_CMD=\""tmux load-buffer"\"
#to copy to X clipboard:
CFLAGS += -DDEFAULT_COPY_TO_CLIPBOARD_CMD=\""xclip -i -selection clipboard <"\"
#to copy to OSX clipboard:
#CFLAGS += -DDEFAULT_COPY_TO_CLIPBOARD_CMD=\""pbcopy <"\"
#
# Choose one of the proposed commands for pasting from different clipboards:
# You can later change it at runtime.
#CFLAGS += -DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=\""tmux show-buffer"\"
CFLAGS += -DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=\""xclip -o -selection clipboard"\"
#CFLAGS += -DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=\""pbpaste"\"

# Command to open file or link under cursor
CFLAGS += -DDEFAULT_OPEN_FILE_UNDER_CURSOR_CMD=\""scopen"\"

# Autobackup. If you unset this, no backup check nor autobackup feature will be available.
CFLAGS += -DAUTOBACKUP
# Have threads? Set these two, if you want the autobackup feature to work with threads.
CFLAGS += -DHAVE_PTHREAD

ifneq ($(shell uname -s),Darwin)
  LDLIBS += -pthread
endif

# Check for gnuplot existance
ifneq (, $(shell which gnuplot))
  CFLAGS += -DGNUPLOT
endif

# OpenBSD does not implement wordexp()
ifeq ($(shell uname -s),OpenBSD)
  CFLAGS += -DNO_WORDEXP
endif

# dynamic linking (not available in BSD)
ifneq ($(shell uname -s | grep -o BSD),BSD)
  LDLIBS += -ldl
endif

ifneq (, $(shell which pkg-config))
  # Any system with pkg-config

  # NOTE: ncursesw (required)
  ifeq ($(shell uname -s),Darwin)
    # macOS' ncurses is built with wide-char support
    LDFLAGS += -lncurses
  else ifneq ($(shell pkg-config --exists ncursesw || echo 'no'),no)
    CFLAGS += $(shell pkg-config --cflags ncursesw)
    LDLIBS += $(shell pkg-config --libs ncursesw)
  else ifneq ($(shell pkg-config --exists ncurses || echo 'no'),no)
    # hopefully this includes wide character support then
    CFLAGS += $(shell pkg-config --cflags ncurses)
    LDLIBS += $(shell pkg-config --libs ncurses)
  else
    LDLIBS += -lncursesw
  endif

  # NOTE: libxlsreader (libxls) is required for xls file reading support
  ifneq ($(shell pkg-config --exists libxls || echo 'no'),no)
    CFLAGS += -DXLS $(shell pkg-config --cflags libxls)
    LDLIBS += $(shell pkg-config --libs libxls)
  endif

  # NOTE: libxml and libzip are required for xlsx file import support
  ifneq ($(shell pkg-config --exists libzip libxml-2.0 || echo 'no'),no)
    CFLAGS += -DODS -DXLSX $(shell pkg-config --cflags libxml-2.0 libzip)
    LDLIBS += $(shell pkg-config --libs libxml-2.0 libzip)
  endif

  # NOTE: libxlsxwriter is required for xlsx file export support
  ifneq ($(shell pkg-config --exists xlsxwriter || echo 'no'),no)
    CFLAGS += -DXLSX_EXPORT $(shell pkg-config --cflags xlsxwriter)
    LDLIBS += $(shell pkg-config --libs xlsxwriter)
  endif

  # NOTE: lua support
  ifneq ($(shell pkg-config --exists lua || echo 'no'),no) # Check for user's default lua
    CFLAGS += -DXLUA $(shell pkg-config --cflags lua)
    ifneq ($(shell uname -s),Darwin)
      LDLIBS += $(shell pkg-config --libs lua) -Wl,--export-dynamic
    else
      LDLIBS += $(shell pkg-config --libs lua) -rdynamic
    endif
  else ifneq ($(shell pkg-config --exists luajit || echo 'no'),no) # If not found, check for luajit
    CFLAGS += -DXLUA $(shell pkg-config --cflags luajit)
    ifneq ($(shell uname -s),Darwin)
      LDLIBS += $(shell pkg-config --libs luajit) -Wl,--export-dynamic
    else
      LDLIBS += $(shell pkg-config --libs luajit) -rdynamic
    endif
  endif
else ifeq ($(shell uname -s),Darwin)
  # macOS without pkg-config

  # macOS' ncurses is built with wide-char support
  LDFLAGS += -lncurses
else ifeq ($(shell uname -s),NetBSD)
  # NetBSD without pkg-config

  CFLAGS  += -I/usr/pkg/include
  CFLAGS  += -I/usr/pkg/include/ncursesw

  LDFLAGS += -L/usr/pkg/lib
  LDFLAGS += -Wl,-R/usr/pkg/lib

  LDLIBS += -lncursesw
else
  LDFLAGS += -lncursesw
endif

OBJS = $(patsubst %.c, %.o, $(wildcard *.c) $(wildcard actions/*.c) $(wildcard formats/*.c) $(wildcard cmds/*.c) $(wildcard utils/*.c)) gram.o

.PHONY : all clean install docs man_install man_uninstall

all : $(name)

install :
	install -d $(DESTDIR)$(prefix)/bin
	install $(name) $(DESTDIR)$(prefix)/bin/$(name)
	install scopen $(DESTDIR)$(prefix)/bin/scopen
	install -d $(DESTDIR)$(HELPDIR)
	install -m 644 doc $(DESTDIR)$(HELPDIR)/$(name)_help
	install -m 644 $(wildcard plot_*) $(DESTDIR)$(HELPDIR)/
	install -d $(DESTDIR)$(MANDIR)/
	install -m 644 sc-im.1 $(DESTDIR)$(MANDIR)/$(name).1

uninstall :
	-rm $(DESTDIR)$(prefix)/bin/$(name)
	-rm $(DESTDIR)$(HELPDIR)/$(name)_help
	-rm $(DESTDIR)$(HELPDIR)/plot*
	-rm $(DESTDIR)$(MANDIR)/$(name).1

$(name) : $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(name)qref: sc.h
	$(CC) $(CFLAGS) $(LDFLAGS) -DQREF $(QREF_FMT) -DSCNAME=\"$(name)\" -o $(name)qref help.c $(LDLIBS)

$(OBJS) : y.tab.h experres.h statres.h

.INTERMEDIATE: y.tab.c
y.tab.c	: gram.y
	$(YACC) -d $<

gram.c : y.tab.c
	mv $< $@

y.tab.h : gram.y | gram.c

experres.h : gram.y eres.sed
	sed -f eres.sed < $< > $@

statres.h : gram.y sres.sed
	sed -f sres.sed < $< > $@

pvmtbl.o: sc.h pvmtbl.c
	$(CC) ${CFLAGS} -c -DPSC pvmtbl.c

docs:
	doxygen Doxyfile

man_install:
	@cp -r ../docs/man/man3/ /usr/local/share/man/
	mandb

# "sc-im" MUST match what is in Doxyfile `MAN_EXTENSION = .sc-im.3`
man_uninstall:
	@rm -rf /usr/local/share/man/man3/*sc-im.3
	@mandb

clean:
	rm -f $(OBJS)
	rm -f experres.h statres.h y.tab.h
	rm -f core gram.c y.output pxmalloc.c pvmtbl.c tags $(name)qref
	rm -f qhelp.c $(name)
	rm -rf ../docs/ 
