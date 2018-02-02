IDIR = include
ODIR = obj
BDIR = bin
SDIR = src

_DEPS = alarm.h application.h datalink.h shared.h interface.h util_interface.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ_LIB = alarm.o application.o datalink.o shared.o util_interface.o
OBJ_LIB = $(patsubst %,$(ODIR)/%,$(_OBJ_LIB))

_PROGS = tests tests-rcvr tests-xmit interface
PROGS = $(patsubst %,$(BDIR)/%,$(_PROGS))

CC = gcc
CFLAGS = -Wall -I$(IDIR)

all: $(PROGS)

$(BDIR)/%: $(ODIR)/%.o $(OBJ_LIB)
	$(CC) -o $@ $^ $(CFLAGS)
	
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

src: TP1-src.c datalink.c datalink.h alarm.c alarm.h shared.c shared.h
	$(CC) -o ../src TP1-src.c datalink.c alarm.c shared.c $(CFLAGS)

dest: TP1-dest.c datalink.c datalink.h alarm.c alarm.h shared.c shared.h
	$(CC) -o bin/dest TP1-dest.c datalink.c alarm.c shared.c $(CFLAGS)

clean:
	rm -f ../src ../dest
	
.PRECIOUS: obj/%.o

