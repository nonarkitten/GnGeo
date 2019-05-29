APPNAME  := release/gngeo
SOURCES  := $(wildcard \
	src/*.c \
	src/ammx/*.s \
	src/asm/*.s \
	src/vasm/*.s \
	src/blitter/*.c \
	src/ym2610/*.c)

OBJECTS  := $(patsubst %.c,%.o, \
   $(patsubst %.s,%.o, \
   $(patsubst src/%,obj/%, \
   $(patsubst src/asm/%,obj/%, \
   $(patsubst src/vasm/%,obj/%, \
   $(patsubst src/blitter/%,obj/%, \
   $(patsubst src/ammx/%,obj/%, \
   $(patsubst src/ym2610/%,obj/%, \
   $(SOURCES)))))))))

DEFINES  := \
	-DWORDS_BIGENDIAN \
	-DHAVE_CONFIG_H -D__AMIGA__ \
	-DUSE_GENERATOR68K -DUSE_MAMEZ80 \
	-DDATA_DIRECTORY=\"${pkgdatadir}\"
   
INCLUDE  := \
	-IADE\:include.arti -IADE\:os-include -I.

LIBS      = -lm -lgenerator68k -lmamez80

LIBPATH  := -L./lib \
	-L"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/lib" \
	-L"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/lib/libb/libnix"

# -funit-at-a-time -frename-registers -fweb -fsingle-precision-constant
FLAGS    := -noixemul -msoft-float -w -O3  -m68020-60 -fshort-double -fshort-enums \
	-ffast-math -finline-functions -fomit-frame-pointer \
   
CC       := m68k-amigaos-gcc $(FLAGS) $(INCLUDE) $(DEFINES) -Wall
VASM     := vasm -Faout -quiet -x -m68020 -spaces -showopt
GAS      := as

all: premake $(OBJECTS)
	$(CC) -flto -s $(OBJECTS) -o $(APPNAME) $(LIBPATH) $(LIBS)
	shrinkler $(APPNAME) $(APPNAME)

premake:
	c:setclock load
	c:wait 1 sec

%.o: ../src/%.c
	$(CC) -c $< -o $@

%.o: ../src/blitter/%.c
	$(CC) -c $< -o $@

%.o: ../src/ym2610/%.c
	$(CC) -c $< -o $@

%.o: ../src/asm/%.s
	$(GAS) $< -o $@

%.o: ../src/vasm/%.s
	$(VASM) $< -o $@

#%.o: ../src/ammx/%.s
#	 $(AS) $< -o $@

clean:
	rm -rf obj/*
	rm -f $(APPNAME)

