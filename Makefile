APPNAME  := release/gngeo
SOURCES  := $(wildcard src/*.c src/*.s )
OBJECTS  := $(patsubst %.c,%.o, $(patsubst %.s,%.o, $(SOURCES)))

DEFINES  := \
	-DWORDS_BIGENDIAN \
	-DHAVE_CONFIG_H -D__AMIGA__ \
	-DUSE_GENERATOR68K -DUSE_MAMEZ80 \
	-DDATA_DIRECTORY=\"${pkgdatadir}\"
   
INCLUDE  := \
	-IADE\:include.arti -IADE\:os-include -I.

LIBS      = -lm -lgen68k.a

LIBPATH  := -L./libs

# -funit-at-a-time -frename-registers -fweb -fsingle-precision-constant
FLAGS    := -noixemul -msoft-float -w -Os  -m68020-60 -fshort-double -fshort-enums \
	-ffast-math -finline-functions -fomit-frame-pointer \
   
CC       := m68k-amigaos-gcc $(FLAGS) $(INCLUDE) $(DEFINES) -Wall
VASM     := vasm -Faout -quiet -x -m68020 -spaces -showopt
GAS      := as

all: premake $(OBJECTS)
	$(CC) -flto -s $(OBJECTS) -o $(APPNAME) $(LIBPATH) $(LIBS)
	shrinkler $(APPNAME) $(APPNAME)

premake:
	$(MAKE) -C gen68k

%.o: ../src/%.c
	$(CC) -c $< -o $@

%.o: ../src/%.s
	$(GAS) $< -o $@

clean:
	$(MAKE) -C gen68k clean
	rm -rf obj/*
	rm -f $(APPNAME)

