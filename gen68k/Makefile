APPNAME  := ../libs/libgen68k.a

DEFINES  := \
	-DUSE_GENERATOR68K -DWORDS_BIGENDIAN \
	-DDATA_DIRECTORY=\"${pkgdatadir}\" \
	-DHAVE_CONFIG_H -D__AMIGA__ \

INCLUDE  := -I$(CURDIR)/../inc \
	-IADE\:include.arti -IADE\:os-include -I.

FLAGS    := -noixemul -msoft-float -w -Os  -m68020-60 -fshort-enums \
	-ffast-math -finline-functions -fomit-frame-pointer \

LIBS      = -lm

CC       := m68k-amigaos-gcc -pipe $(FLAGS) $(INCLUDE) $(DEFINES) $(LIBS) -Wall
AR		 := m68k-amigaos-ar q
RANLIB   := m68k-amigaos-ranlib
#SOURCES  := $(wildcard src/*.c src/*.s )
#OBJECTS  := $(patsubst %.c,%.o, $(patsubst %.s,%.o, $(SOURCES)))

.PHONY: all
all: def68k.exe gen68k.exe $(CURDIR)/$(APPNAME)

$(CURDIR)/$(APPNAME):
	$(eval SRCS := cpu68k.c diss68k.c reg68k.c tab68k.c $(wildcard cpu68k-*.c))
	$(eval OBJS := $(patsubst %.c,%.o, $(SRCS)))
	$(CC) -c $(SRCS)
	rm -f $(CURDIR)/$(APPNAME) 
	$(AR) $(CURDIR)/$(APPNAME) $(OBJS)
	$(RANLIB) $(CURDIR)/$(APPNAME)

def68k.exe: 
	$(CC) -o def68k.exe def68k.c tab68k.c
	def68k.exe

gen68k.exe:
	$(CC) -o gen68k.exe gen68k.c tab68k.c
	gen68k.exe
	
.PHONY: clean
clean:
	rm -f def68k-funcs.h def68k-iibs.h def68k-proto.h
	rm -f cpu68k-*.c
	rm -f *.o
	rm -f def68k.exe
	rm -f gen68k.exe
	rm -f $(CURDIR)/$(APPNAME)

.PHONY: rebuild
rebuild: clean all