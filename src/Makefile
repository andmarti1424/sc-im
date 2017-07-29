# Specify the name of the resulting executable file
name = scim

# The base directory where everything should be installed.
prefix  = /usr/local

EXDIR   = $(prefix)/bin
HELPDIR = $(prefix)/share/$(name)
LIBDIR  = $(prefix)/share/doc/$(name)

# This is where the man page goes.
MANDIR  = $(prefix)/share/man/man1

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
# Command history file, relative to home directory. Comment out to disable commandline history
CFLAGS += -DHISTORY_FILE=\".$(name)info\"
# Input mode history. Same as previous, but for insert mode commands
CFLAGS += -DINS_HISTORY_FILE=\".$(name)info\"
# Comment out to disable undo/redo support
CFLAGS += -DUNDO
# Maximum number of rows in spreadsheet. Up to 1048576
CFLAGS += -DMAXROWS=65536
# Used for date formatting with C-d shortcut using you local d_fmt
CFLAGS += -DUSELOCALE

# Clipboard support is OS dependent.
#
# Choose one of the following commands for copying to different clipboards:
# You can later change it at runtime.
#to copy to tmux clipboard:
CFLAGS += -DDEFAULT_COPY_TO_CLIPBOARD_CMD=\""tmux load-buffer"\"
#to copy to X clipboard:
#CFLAGS += -DDEFAULT_COPY_TO_CLIPBOARD_CMD=\""xclip -i -selection clipboard <"\"
#to copy to OSX clipboard:
#CFLAGS += -DDEFAULT_COPY_TO_CLIPBOARD_CMD=\""pbcopy <"\"
#
# Choose one of the proposed commands for pasting from different clipboards:
# You can later change it at runtime.
CFLAGS += -DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=\""tmux show-buffer"\"
#CFLAGS += -DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=\""xclip -o -selection clipboard"\"
#CFLAGS += -DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=\""pbpaste"\"

# Uncomment for basic XLS import. Requires libxlsreader
#CFLAGS += -DXLS
#LDLIBS += -lxlsreader

# Autobackup. If you unset this, no backup check nor autobackup feature will be available.
CFLAGS += -DAUTOBACKUP
# Have threads? Set these two, if you want the autobackup feature to work with threads.
CFLAGS += -DHAVE_PTHREAD

ifneq ($(shell uname -s),Darwin)
  LDLIBS += -pthread
endif

# NOTE: libxlsxwriter is required for xlsx file export support
ifneq (,$(wildcard /usr/include/xlsxwriter.h))
  CFLAGS += -DXLSX_EXPORT
  LDLIBS += -lxlsxwriter
endif

# Check for gnuplot existance
ifneq (, $(shell which gnuplot))
  CFLAGS += -DGNUPLOT
endif

# dynamic linking (should not be used in FreeBSD
ifneq ($(shell uname -s),FreeBSD)
  LDLIBS += -ldl
endif

ifneq (, $(shell which pkg-config))
  # Any system with pkg-config

  # NOTE: ncursesw (required)
  ifneq ($(shell pkg-config --exists ncursesw || echo 'no'),no)
    CFLAGS += $(shell pkg-config --cflags ncursesw)
    LDLIBS += $(shell pkg-config --libs ncursesw)
  else ifneq ($(shell pkg-config --exists ncurses || echo 'no'),no)
    # hopefully this includes wide character support then
    CFLAGS += $(shell pkg-config --cflags ncurses)
    LDLIBS += $(shell pkg-config --libs ncurses)
  else
    LDLIBS += -lncursesw
  endif

  # NOTE: libxml and libzip are required for xlsx file import support
  ifneq ($(shell pkg-config --exists libzip libxml-2.0 || echo 'no'),no)
    CFLAGS += -DXLSX $(shell pkg-config --cflags libxml-2.0 libzip)
    LDLIBS += $(shell pkg-config --libs libxml-2.0 libzip)
  endif

  # NOTE: lua support
  ifneq ($(shell pkg-config --exists lua51 || echo 'no'),no)
    CFLAGS += -DXLUA $(shell pkg-config --cflags lua51)
    LDLIBS += $(shell pkg-config --libs lua51) -Wl,--export-dynamic
  else ifneq ($(shell pkg-config --exists lua-5.1 || echo 'no'),no) # FreeBSD
    CFLAGS += -DXLUA $(shell pkg-config --cflags lua-5.1)
    LDLIBS += $(shell pkg-config --libs lua-5.1) -Wl,--export-dynamic
  endif
else ifeq ($(shell uname -s),Darwin)
  # macOS without pkg-config

  # macOS' ncurses is built with wide-char support
  LDFLAGS += -lncurses
endif

OBJS = $(patsubst %.c, %.o, $(wildcard *.c) $(wildcard utils/*.c)) gram.o

.PHONY : all clean install

all : $(name)

install :
	install -d $(DESTDIR)$(prefix)/bin
	install $(name) $(DESTDIR)$(prefix)/bin/$(name)
	install -d $(DESTDIR)$(HELPDIR)
	install doc $(DESTDIR)$(HELPDIR)/$(name)_help
	install plot_* $(DESTDIR)$(HELPDIR)/
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

y.tab.h : gram.y gram.c
	test -f y.tab.c && mv y.tab.c gram.c

gram.c : gram.y
	$(YACC) -d $<

pvmtbl.o: sc.h pvmtbl.c
	$(CC) ${CFLAGS} -c -DPSC pvmtbl.c

experres.h : gram.y
	sed -f eres.sed < gram.y > experres.h

statres.h : gram.y
	sed -f sres.sed < gram.y > statres.h

clean:
	rm -f $(OBJS)
	rm -f *res.h $(YTAB).h
	rm -f core gram.c y.output pxmalloc.c pvmtbl.c tags $(name)qref
	rm -f qhelp.c $(name)
