APPNAME  := ../libs/libgen68k.a

DEFINES  := \
	-DUSE_GENERATOR68K -DWORDS_BIGENDIAN \
	-DDATA_DIRECTORY=\"${pkgdatadir}\" \
	-DHAVE_CONFIG_H -D__AMIGA__ \

INCLUDE  := -I$(CURDIR)/../inc \
	-IADE\:include.arti -IADE\:os-include -I.

FLAGS    := -noixemul -msoft-float -w -Os  -m68020-60 -fshort-double -fshort-enums \
	-ffast-math -finline-functions -fomit-frame-pointer \

LIBS      = -lm

CC       := m68k-amigaos-gcc $(FLAGS) $(INCLUDE) $(DEFINES) $(LIBS) -Wall
#SOURCES  := $(wildcard src/*.c src/*.s )
#OBJECTS  := $(patsubst %.c,%.o, $(patsubst %.s,%.o, $(SOURCES)))

.PHONY: all
all: def68k.exe gen68k.exe $(CURDIR)/$(APPNAME)

$(CURDIR)/$(APPNAME):
	$(eval SRCS := cpu68k.c diss68k.c reg68k.c tab68k.c $(wildcard cpu68k-*.c))
	$(eval OBJS := $(patsubst %.c,%.o, $(SRCS)))
	$(CC) -c $(SRCS)
	rm -f $(CURDIR)/$(APPNAME) 
	ar q $(CURDIR)/$(APPNAME) $(OBJS)
	ranlib $(CURDIR)/$(APPNAME)

def68k.exe: def68k.c tab68k.c def68k.def
	$(CC) -o def68k.exe def68k.c tab68k.c
	def68k.exe

gen68k.exe: gen68k.c tab68k.c def68k-funcs.h def68k-iibs.h def68k-proto.h 
	$(CC) -o gen68k.exe gen68k.c tab68k.c
	gen68k.exe
	
.PHONY: clean
clean:
	rm -f *.o
	rm -f $(CURDIR)/$(APPNAME)

.PHONY: rebuild
rebuild: clean all