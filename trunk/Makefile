CC=gcc
CFLAGS=-Wall
LDFLAGS=-lm
DISTVERSION=1.0
DISTNAME=basicplay-$(DISTVERSION)-src

basicplay : basicplay.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) basicplay.c -o basicplay

clean : 
	rm -rf *~ *.o basicplay $(DISTNAME) $(DISTNAME).tar $(DISTNAME).tar.gz

dist : $(DISTNAME).tar.gz

$(DISTNAME).tar : basicplay
	mkdir -p $(DISTNAME)
	cp Makefile $(DISTNAME)
	cp basicplay.c $(DISTNAME)
	tar cvf $(DISTNAME).tar $(DISTNAME)
	rm -rf $(DISTNAME)

$(DISTNAME).tar.gz : $(DISTNAME).tar
	gzip -f $(DISTNAME).tar
