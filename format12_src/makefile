# Makefile using make86 (on ELKS) to build format12
# Based on /usr/src/makefile

INCLUDES=-I/usr/include -I/usr/include/c86
C86LIB=/usr/lib
#BIN=/usr/bin/

DEFINES=
#TIME=time

##### Standardized section of Makefile #####

CPP=$(BIN)cpp
CC=$(BIN)c86
AS=$(BIN)as
LD=$(BIN)ld

CPPFLAGS=-0 $(INCLUDES) $(DEFINES)
CFLAGS=-g -O -bas86 -separate=yes -warn=4 -lang=c99 \
    -align=yes -separate=yes -stackopt=minimum -peep=all -stackcheck=no
ASFLAGS=-0 -j
LDFLAGS=-0 -i -L$(C86LIB)
LDLIBS=-lc86

# Automated rules for C86 toolchain
.c.o:
	$(TIME) $(CPP) $(CPPFLAGS) $*.c -o $*.i
	$(TIME) $(CC) $(CFLAGS) $*.i $*.as
	$(TIME) $(AS) $(ASFLAGS) $*.as -o $*.o

.s.o:
	$(TIME) $(AS) $(ASFLAGS) $*.s -o $*.o

##### End of standardized section #####

PROGS=format12

all: $(PROGS)

format12: format12.o int1b5d.o
	$(TIME) $(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.i *.o *.as $(PROGS)
