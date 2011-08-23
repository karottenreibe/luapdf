# Include makefile config
include config.mk

# Token lib generation
TLIST = common/tokenize.list
THEAD = common/tokenize.h
TSRC  = common/tokenize.c

SRCS  = $(filter-out $(TSRC),$(wildcard *.c) $(wildcard common/*.c) $(wildcard clib/*.c) $(wildcard widgets/*.c)) $(TSRC)
HEADS = $(wildcard *.h) $(wildcard common/*.h) $(wildcard widgets/*.h) $(wildcard clib/*.h) $(THEAD) globalconf.h
OBJS  = $(foreach obj,$(SRCS:.c=.o),$(obj))

all: options newline luapdf luapdf.1

options:
	@echo luapdf build options:
	@echo "CC           = $(CC)"
	@echo "LUA_PKG_NAME = $(LUA_PKG_NAME)"
	@echo "CFLAGS       = $(CFLAGS)"
	@echo "CPPFLAGS     = $(CPPFLAGS)"
	@echo "LDFLAGS      = $(LDFLAGS)"
	@echo "INSTALLDIR   = $(INSTALLDIR)"
	@echo "MANPREFIX    = $(MANPREFIX)"
	@echo "DOCDIR       = $(DOCDIR)"
	@echo
	@echo build targets:
	@echo "SRCS  = $(SRCS)"
	@echo "HEADS = $(HEADS)"
	@echo "OBJS  = $(OBJS)"

$(THEAD) $(TSRC): $(TLIST)
	./build-utils/gentokens.lua $(TLIST) $@

globalconf.h: globalconf.h.in
	sed 's#LUAPDF_INSTALL_PATH .*#LUAPDF_INSTALL_PATH "$(PREFIX)/share/luapdf"#' globalconf.h.in > globalconf.h

$(OBJS): $(HEADS) config.mk

.c.o:
	@echo $(CC) -c $< -o $@
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

luapdf: $(OBJS)
	@echo $(CC) -o $@ $(OBJS)
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)

luapdf.1: luapdf
	help2man -N -o $@ ./$<

apidoc: luadoc/luapdf.lua
	mkdir -p apidocs
	luadoc --nofiles -d apidocs luadoc/* lib/*

doc: globalconf.h $(THEAD) $(TSRC)
	doxygen -s luapdf.doxygen

clean:
	rm -rf apidocs doc luapdf $(OBJS) $(TSRC) $(THEAD) globalconf.h luapdf.1

install:
	install -d $(INSTALLDIR)/share/luapdf/
	install -d $(DOCDIR)
	install -m644 README.md AUTHORS COPYING* $(DOCDIR)
	cp -r lib $(INSTALLDIR)/share/luapdf/
	chmod 755 $(INSTALLDIR)/share/luapdf/lib/
	chmod 755 $(INSTALLDIR)/share/luapdf/lib/lousy/
	chmod 755 $(INSTALLDIR)/share/luapdf/lib/lousy/widget/
	chmod 644 $(INSTALLDIR)/share/luapdf/lib/*.lua
	chmod 644 $(INSTALLDIR)/share/luapdf/lib/lousy/*.lua
	chmod 644 $(INSTALLDIR)/share/luapdf/lib/lousy/widget/*.lua
	install -d $(INSTALLDIR)/bin
	install luapdf $(INSTALLDIR)/bin/luapdf
	install -d $(DESTDIR)/etc/xdg/luapdf/
	install config/*.lua $(DESTDIR)/etc/xdg/luapdf/
	chmod 644 $(DESTDIR)/etc/xdg/luapdf/*.lua
	install -d $(DESTDIR)/usr/share/pixmaps
	install extras/luapdf.png $(DESTDIR)/usr/share/pixmaps/
	install -d $(DESTDIR)/usr/share/applications
	install extras/luapdf.desktop $(DESTDIR)/usr/share/applications/
	install -d $(MANPREFIX)/man1/
	install -m644 luapdf.1 $(MANPREFIX)/man1/

uninstall:
	rm -rf $(INSTALLDIR)/bin/luapdf $(INSTALLDIR)/share/luapdf $(MANPREFIX)/man1/luapdf.1
	rm -rf /usr/share/applications/luapdf.desktop /usr/share/pixmaps/luapdf.png

newline: options;@echo
.PHONY: all clean options install newline apidoc doc
