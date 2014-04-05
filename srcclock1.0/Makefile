CC = g++

OBJS = main.o csrc.o crw.o clog.o
SOURCE = source/
VERSION = 1.0
VPATH = ./$(SOURCE)
DEBUG = -g
LIBS = -lpulse-simple -lpulse
CFLAGS = -Wall -c $(DEBUG) $(LIBS) -std=c++11
LFLAGS = -Wall $(DEBUG) $(LIBS)

SRCclock: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o srcclock

main.o: main.cpp csrc.h crw.h clog.h
	$(CC) $(CFLAGS) $<

csrc.o: csrc.cpp csrc.h crw.h clog.h
	$(CC) $(CFLAGS) $<

crw.o: crw.cpp crw.h
	$(CC) $(CFLAGS) $<

clog.o: clog.cpp clog.h
	$(CC) $(CFLAGS) $<



clean:
	rm *.o $(VPATH)*~

tar:
	mkdir srcclock$(VERSION)
	cp -r $(SOURCE) Makefile LICENSE.txt CHANGELOG \
		README.txt .Doxyfile TODO srcclock.7 srcclock$(VERSION)
	tar -cvzf srcclock$(VERSION).tar.gz srcclock$(VERSION)/
	rm -rf srcclock$(VERSION)/

install: srcclock
	cp srcclock /usr/bin
	install -D srcclock.7 /usr/local/share/man/man7/

uninstall:
	rm /usr/bin/srcclock
	rm /usr/local/share/man/man7/srcclock.7

doc:
	mkdir doc
	doxygen .Doxyfile

