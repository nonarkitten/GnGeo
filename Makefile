APPNAME  := release/gngeo
SOURCES  := $(wildcard src/*.c src/*.s )
OBJECTS  := $(patsubst src/%.c,obj/%.o, $(patsubst src/%.s,obj/%.o, $(SOURCES)))

DEFINES  := \
	-DWORDS_BIGENDIAN \
	-DHAVE_CONFIG_H -D__AMIGA__ \
	-DUSE_GENERATOR68K -DUSE_MAMEZ80 \
	-DDATA_DIRECTORY=\"${pkgdatadir}\"
   
INCLUDE  := -I./inc\
	-IADE\:include.arti -IADE\:os-include -I.

LIBS      = -lm -lgen68k -llz4w

LIBPATH  := -L./libs

# -funit-at-a-time -frename-registers -fweb -fsingle-precision-constant
FLAGS    := -noixemul -msoft-float -w -Os  -m68020-60 -fshort-double -fshort-enums \
	-ffast-math -finline-functions -fomit-frame-pointer \
   
CC       := m68k-amigaos-gcc $(FLAGS) $(INCLUDE) $(DEFINES) -Wall
VASM     := vasm -Faout -quiet -x -m68020 -spaces -showopt
GAS      := as

.PHONY: all
all: premake $(OBJECTS)
	$(CC) -flto -s $(OBJECTS) -o $(APPNAME) $(LIBPATH) $(LIBS)
	shrinkler $(APPNAME) $(APPNAME)

.PHONY: premake
premake:
	$(MAKE) -C lz4w
	$(MAKE) -C gen68k

%.o: ../src/%.c
	$(CC) -c $< -o $@

%.o: ../src/%.s
	$(GAS) $< -o $@

.PHONY: clean
clean:
	$(MAKE) -C lz4w clean
	$(MAKE) -C gen68k clean
	rm -rf obj/*
	rm -f $(APPNAME)

.PHONY: rebuild
rebuild: clean all
