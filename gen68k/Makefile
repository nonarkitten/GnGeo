APPNAME  := ../libs/libgen68k.a

DEFINES  := \
	-DUSE_GENERATOR68K -DWORDS_BIGENDIAN \
	-DDATA_DIRECTORY=\"${pkgdatadir}\"
	-DHAVE_CONFIG_H -D__AMIGA__ \

INCLUDE  := \
	-IADE\:include.arti -IADE\:os-include -I.

FLAGS    := -noixemul -msoft-float -w -Os  -m68020-60 -fshort-double -fshort-enums \
	-ffast-math -finline-functions -fomit-frame-pointer \

LIBS      = -lm

CC       := m68k-amigaos-gcc $(FLAGS) $(INCLUDE) $(DEFINES) $(LIBS) -Wall

all: makedefs makegen makelib

makedefs: def68k.exe
	$(CC) -o def68k.exe def68k.c tab68k.c
	def68k.exe

makegen: gen68k.exe
	$(CC) -o gen68k.exe gen68k.c tab68k.c
	gen68k.exe

makelib:
	$(CC) -c cpu68k.c cpu68k-*.c diss68k.c reg68k.c tab68k.c -o $(APPNAME)
	
clean:
	rm -f *.o
	rm -f $(APPNAME)