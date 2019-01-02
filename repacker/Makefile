APPNAME	:= gngeo_repack
SOURCES	:= $(wildcard *.c *.s)
OBJECTS	:= $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(SOURCES)))

DEFINES	:= \
	-DWORDS_BIGENDIAN -Dbool="int" \
	-DHAVE_CONFIG_H -D__AMIGA__ \
	-DUSE_GENERATOR68K -DUSE_MAMEZ80 \
	-DDATA_DIRECTORY=\"${pkgdatadir}\"
	
INCLUDE	:= \
	-IADE\:include.arti -IADE\:include/os-include -I.

LIBS      = -lm -lz-b

LIBPATH	:= \
	-L"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/lib" \
	-L"C:/Development/AmiDevCpp/usr/local/amiga/m68k-amigaos/lib/libb/libnix"

# -funit-at-a-time -frename-registers -fweb -fsingle-precision-constant
FLAGS    := \
	-noixemul \
	-msoft-float -w -O3 -m68040 -noixemul -fshort-double -fshort-enums -ffast-math \
	-finline-functions -fomit-frame-pointer -fexpensive-optimizations \
	
CC       := m68k-amigaos-gcc $(FLAGS) $(INCLUDE) $(DEFINES) -Wall
AS       := as -m68040

all: $(OBJECTS)
	$(CC) -s $(OBJECTS) -o $(APPNAME) $(LIBPATH) $(LIBS)
#	shrinkler $(APPNAME) $(APPNAME)
	copy $(APPNAME) ../release/

%.o: ../%.c
	$(CC) $(CCFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

%.o: ../%.s
	$(AS) $< -o $@

clean:
	delete #?
	delete $(APPNAME)
	delete /release/$(APPNAME)

