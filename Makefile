#
# Makefile for rogue
# @(#)Makefile	4.13 (Berkeley) 1/23/82
#
HDRS=	rogue.h extern.h
DOBJS=	vers.o extern.o armor.o chase.o command.o daemon.o daemons.o \
	fight.o init.o io.o list.o main.o misc.o monsters.o move.o \
	new_level.o options.o pack.o passages.o potions.o rings.o rip.o \
	rooms.o save.o scrolls.o state.o sticks.o things.o weapons.o wizard.o xcrypt.o
OBJS=	$(DOBJS) mach_dep.o
POBJS=	vers.po extern.po armor.po chase.po command.po daemon.po daemons.po \
	fight.po init.o io.o list.o main.o misc.o monsters.o move.o \
	new_level.po options.o pack.o passages.o potions.o rings.o rip.o \
	rooms.po save.o scrolls.o state.o sticks.o things.o weapons.o wizard.o xcrypt.o \
	mach_dep.po
CFILES=	vers.c extern.c armor.c chase.c command.c daemon.c daemons.c \
	fight.c init.c io.c list.c main.c misc.c monsters.c move.c \
	new_level.c options.c pack.c passages.c potions.c rings.c rip.c \
	rooms.c save.c scrolls.c state.o sticks.c things.c weapons.c wizard.c xcrypt.o \
	mach_dep.c

#CFLAGS=		-O
#CFLAGS= -g
CFLAGS= -O3 -pedantic -DWIZARD -DDUMP
PROFLAGS=	-p -O
#LDFLAGS=-i	# For PDP-11's
LDFLAGS=	# For VAXen

SCOREFILE=	/usr/games/lib/rogue_roll
SF=		-DSCOREFILE='"$(SCOREFILE)"'
NAMELIST=	/vmunix
NL=		-DNAMELIST='"$(NAMELIST)"'
#MACHDEP=	-DMAXLOAD=40 -DLOADAV -DCHECKTIME=4
MACHDEP= -DMAXLOAD=40 -DLOADAV -DCHECKTIME=4
BINARY=		distr.out

VGRIND=/usr/ucb/vgrind
CRLIB=	-lcurses
CRYPTLIB=
MISC=	xstr.c Makefile prob.c findpw.c
LD=	ld

.SUFFIXES: .po

.c.po:
	@echo $(CC) -c $(PROFLAGS) $*.c
	@cc -E $(PROFLAGS) $*.c | xstr -c -
	@cc -c $(PROFLAGS) x.c
	@mv x.o $*.po

.c.o:
#	@echo $(CC) -c $(CFLAGS) $*.c
	$(CC) -c $(CFLAGS) $*.c
#	@cc -E $(CFLAGS) $*.c | xstr -c -
#	@cc -c $(CFLAGS) x.c
#	@mv x.o $*.o

rogue: a.out
	cp a.out rogue
	strip rogue

a.out: xstr $(HDRS) $(OBJS)
	@rm -f x.c
	$(CC) $(LDFLAGS) $(OBJS) $(CRLIB) $(CRYPTLIB)
	size a.out

vers.o:
	$(CC) -c $(CFLAGS) vers.c

mach_dep.o:
	$(CC) -c $(CFLAGS) $(SF) $(NL) $(MACHDEP) mach_dep.c

xs.o: strings
	xstr
	$(CC) -c $(CFLAGS) xs.c

xstr: xstr.c
	$(CC) -s -O -o xstr xstr.c

install: rogue
	cp rogue         $(DESTDIR)/usr/games/rogue
	chown games      $(DESTDIR)/usr/games/rogue
	chgrp bin        $(DESTDIR)/usr/games/rogue
	chmod 4755       $(DESTDIR)/usr/games/rogue
	cat /dev/null >> $(DESTDIR)/usr/games/lib/rogue_roll
	chown games      $(DESTDIR)/usr/games/lib/rogue_roll
	chgrp bin        $(DESTDIR)/usr/games/lib/rogue_roll
	chmod 644        $(DESTDIR)/usr/games/lib/rogue_roll

distr: rmmach distmod.o mach_dep.o
	@rm -f x.c
	$(CC) -s $(LDFLAGS) -o =bin/$(BINARY) distmod.o mach_dep.o $(CRLIB) $(CRYPTLIB)
	size =bin/$(BINARY)

rmmach:
	rm -f mach_dep.o

distmod.o: $(DOBJS) xs.o
	@rm -f x.c
	-$(LD) -r -x -o distmod.o xs.o $(DOBJS)

findpw: findpw.c xcrypt.o
	$(CC) -s -o findpw findpw.c xcrypt.o $(CRYPTLIB)

prob: prob.o extern.o xs.o
	$(CC) -O -o prob prob.o extern.o xs.o

prob.o: prob.c rogue.h
	$(CC) -O -c prob.c

p.out:	$(HDRS) $(POBJS) xs.po
	@rm -f x.c
	$(CC) $(PROFLAGS) $(LDFLAGS) -o p.out xs.po $(POBJS) $(CRLIB) $(CRYPTLIB)
	size p.out

vers.po:
	@echo $(CC) -c $(PROFLAGS) vers.c
	@ln vers.c x.c
	@cc -c $(PROFLAGS) $(SF) x.c
	@mv x.o vers.po
	@rm -f x.c

mach_dep.po:
	$(CC) -c $(PROFLAGS) $(SF) $(NL) $(MACHDEP) mach_dep.c

xs.po: strings
	xstr
	@echo $(CC) -c $(PROFLAGS) xs.c
	@mv xs.c x.c
	@$(CC) -c $(PROFLAGS) x.c
	@mv x.o xs.po

tags: $(HDRS) $(CFILES)
	ctags -u $?
	ed - tags < :ctfix
	sort tags -o tags

lint:
	/bin/csh -c "lint -hxbc $(MACHDEP) $(SF) $(NL) $(CFILES) -lcurses > linterrs"

clean:
	rm -f $(POBJS) $(OBJS) core a.out p.out rogue strings make.out rogue.tar vgrind.* x.c x.o xs.c xs.o linterrs findpw distmod.o xs.po xstr

count:
	wc -l $(HDRS) $(CFILES)

realcount:
	cc -E $(CFILES) | ssp - | wc -l

update:
	ar uv .SAVE $(CFILES) $(HDRS) Makefile

xtar: $(HDRS) $(CFILES) $(MISC)
	rm -f rogue.tar
	tar cf rogue.tar $? :ctfix
	touch xtar

vgrind:
	@csh $(VGRIND) -t -h "Rogue Version 3.7" $(HDRS) *.c > vgrind.out
	@ctags -v $(HDRS) *.c > index
	@csh $(VGRIND) -t -x index > vgrind.out.tbl

cfiles: $(CFILES)
