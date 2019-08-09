# Project: generator68k
# Compiler: m68k-Amiga-OS3
# Compiler Type: MingW 3
# Makefile created by wxDev-C++ 6.10.2 on 04/02/16 13:07

OUTPUT := libgenerator.a
SOURCES := diss68k.c reg68k.c tab68k.c $(wildcard cpu68k*.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

CC := m68k-amigaos-gcc
INCS := -I"C:\amiga-gcc\m68k-amigaos\sys-include"
DEFINES := 
CCFLAGS := $(INCS) $(DEFINES) -Os -m68020-60 -s -noixemul

all: $(OBJECTS)
	$(CC) $(CCFLAGS) -c $< -o $(OUTPUT)
		
%.o: %.c
	$(CC) $(CCFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f cpu68k-?.c
	rm -f uber*
	rm -f *.o
	
	