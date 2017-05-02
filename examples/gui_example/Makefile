# Specify the name of the resulting executable file
name = scim

# The base directory where everything should be installed.
prefix  = /usr/local

EXDIR   = $(prefix)/bin
HELPDIR = $(prefix)/share/$(name)
LIBDIR  = $(prefix)/share/doc/$(name)

# This is where the man page goes.
MANDIR  = $(prefix)/man/man1

# Change these to your liking or use `make CC=gcc` etc
#CC   = cc
#YACC = bison -y
#SED  = sed

#ifeq ($(shell uname -s),Darwin)
#  NCURSES_CFLAGS ?=
#  NCURSES_LIBS   ?= -lncursesw
#else ifeq ($(shell pkg-config --exists ncursesw || echo 'no'),no)
#  NCURSES_CFLAGS ?=
#  NCURSES_LIBS   ?= -lncursesw
#else
#  NCURSES_CFLAGS ?= $(shell pkg-config --cflags ncursesw)
#  NCURSES_LIBS   ?= $(shell pkg-config --libs ncursesw)
#endif

#LDLIBS += -lm $(NCURSES_LIBS)
LDLIBS += -lm $(shell pkg-config --libs gtk+-3.0)

CFLAGS += -Wall -g
CFLAGS += $(NCURSES_CFLAGS)
#CFLAGS += -DNCURSES
CFLAGS += -D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE
CFLAGS += -DSNAME=\"$(name)\"
CFLAGS += -DHELP_PATH=\"$(HELPDIR)\"
CFLAGS += -DLIBDIR=\"$(LIBDIR)\"
CFLAGS += $(shell pkg-config --cflags gtk+-3.0)

# Sets default pager, e.g. 'less' or 'more'
CFLAGS += -DDFLT_PAGER=\"less\"
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

# Uncomment for basic XLS import. Requires libxlsreader
#CFLAGS += -DXLS
#LDLIBS += -lxlsreader

# NOTE: libxml and libzip are required for xlsx file import support
ifneq ($(shell pkg-config --exists libzip libxml-2.0 || echo 'no'),no)
CFLAGS += -DXLSX $(shell pkg-config --cflags libxml-2.0 libzip)
LDLIBS += $(shell pkg-config --libs libxml-2.0 libzip)
endif

# NOTE: libxlsxwriter is required for xlsx file export support
ifneq (,$(wildcard /usr/include/xlsxwriter.h))
CFLAGS += -DXLSX_EXPORT
LDLIBS += -lxlsxwriter
endif

# NOTE: lua support
ifneq ($(shell pkg-config --exists lua51 || echo 'no'),no)
CFLAGS += -DXLUA $(shell pkg-config --cflags lua51)
LDLIBS += $(shell pkg-config --libs lua51) -Wl,--export-dynamic
endif
# dynamic linking
LDLIBS += -ldl


OBJS = $(patsubst %.c, %.o, $(wildcard *.c) $(wildcard utils/*.c)) gram.o

.PHONY : all clean install

all : $(name)

install :
	install -d $(DESTDIR)$(prefix)/bin
	install $(name) $(DESTDIR)$(prefix)/bin/$(name)
	install -d $(DESTDIR)$(HELPDIR)
	install doc $(DESTDIR)$(HELPDIR)/$(name)_help
	install -d $(DESTDIR)$(prefix)/share/man/man1
	install -m 644 sc-im.1 $(DESTDIR)$(prefix)/share/man/man1/$(name).1

uninstall :
	-rm $(DESTDIR)$(prefix)/bin/$(name)
	-rm $(DESTDIR)$(HELPDIR)/$(name)_help
	-rm $(DESTDIR)$(prefix)/share/man/man1/$(name).1

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
