CC=gcc
CFLAGS=-O3
DEBUGFLAGS=-Wall -g
LDFLAGS=-lm
DISTVERSION=1.0
DISTNAME=basicplay-$(DISTVERSION)-src
PREFIX=/usr/share
DIRS=$(PREFIX)/basicplay $(PREFIX)/man/man1 $(PREFIX)/bin

basicplay : basicplay.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) basicplay.c -o basicplay

debug : basicplay.c Makefile
	$(CC) $(DEBUGFLAGS) $(LDFLAGS) basicplay.c -o basicplay 

clean : 
	rm -rf *~ *.o basicplay $(DISTNAME) $(DISTNAME).tar $(DISTNAME).tar.gz

dist : $(DISTNAME).tar.gz

$(DISTNAME).tar : basicplay
	mkdir -p $(DISTNAME)
	cp Makefile $(DISTNAME)
	cp basicplay.c $(DISTNAME)
	cp basicplay.1 $(DISTNAME)
	cp AUTHORS $(DISTNAME)
	cp INSTALL $(DISTNAME)
	cp COPYING $(DISTNAME)
	tar cvf $(DISTNAME).tar $(DISTNAME)
	rm -rf $(DISTNAME)

$(DISTNAME).tar.gz : $(DISTNAME).tar
	gzip -f $(DISTNAME).tar

basicplay.1.gz : basicplay.1
	gzip -c basicplay.1 > basicplay.1.gz

man : basicplay.1 basicplay.1.gz
	groff -man -T ascii basicplay.1 | less

$(PREFIX)/basicplay :
	mkdir -p $@

$(PREFIX)/man/man1 :
	mkdir -p $@

$(PREFIX)/bin :
	mkdir -p $@

install : basicplay basicplay.1.gz $(DIRS)
	cp basicplay $(PREFIX)/basicplay/
	cp basicplay.1.gz $(PREFIX)/basicplay/
	ln -s $(PREFIX)/basicplay/basicplay.1.gz $(PREFIX)/man/man1/
	ln -s $(PREFIX)/basicplay/basicplay $(PREFIX)/bin/

uninstall :
	rm -rf $(PREFIX)/bin/basicplay
	rm -rf $(PREFIX)/man/man1/basicplay.1.gz
	rm -rf $(PREFIX)/basicplay/
