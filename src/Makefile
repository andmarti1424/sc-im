name = scim
NAME = SC-IM

prefix  = /usr/local
EXDIR   = $(prefix)/bin
HELPDIR = $(prefix)/share/$(name)
LIBDIR  = $(prefix)/share/doc/$(name)
MANDIR  = $(prefix)/man/man1

# Change these to your liking or use `make CC=gcc` etc
#CC   = cc
#YACC = bison -y
#SED  = sed

LDLIBS += -lm $(shell pkg-config --libs ncursesw)

CFLAGS += -Wall -g
CFLAGS += $(shell pkg-config --cflags ncursesw)
CFLAGS += -DSNAME=\"$(NAME)\"
CFLAGS += -D_GNU_SOURCE
CFLAGS += -DHELP_PATH=\"$(HELPDIR)\"
CFLAGS += -DLIBDIR=\"$(LIBDIR)\"
CFLAGS += -DUSECOLORS
CFLAGS += -DDFLT_PAGER=\"less\"
CFLAGS += -DHISTORY_FILE=\".$(name)info\"
CFLAGS += -DUNDO
# Up to 1048576
CFLAGS += -DMAXROWS=65536
# Used for date formatting with C-d shortcut
CFLAGS += -DUSELOCALE

# Uncomment for basic XLS import
#CFLAGS += -DXML
#LDLIBS += -lxlsreader

# Uncomment for basic XSL support
#CFLAGS += -DXLSX -I/usr/include/libxml2
#LDLIBS += -lzip -lxml2

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
	$(CC) $(CFLAGS) $(LDFLAGS) -DQREF $(QREF_FMT) -DSCNAME=\"$(NAME)\" -o $(name)qref help.c $(LDLIBS)

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
	rm -f debug core gram.c y.output pxmalloc.c pvmtbl.c tags $(name)qref
	rm -f qhelp.c $(name)
