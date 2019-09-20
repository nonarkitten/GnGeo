APPNAME	:= ../libs/liblz4w.a
SOURCES	:= $(wildcard *.c *.s)
OBJECTS	:= $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(SOURCES)))

DEFINES	:= \
	-DWORDS_BIGENDIAN -Dbool="int" \
	-DHAVE_CONFIG_H -D__AMIGA__ \
	-DUSE_GENERATOR68K -DUSE_MAMEZ80 \
	-DDATA_DIRECTORY=\"${pkgdatadir}\"
	
INCLUDE	:= -I$(CURDIR)/../inc\
	-IADE\:include.arti -IADE\:include/os-include -I.

LIBS      = -lm -lz-b

LIBPATH	:=

# -funit-at-a-time -frename-registers -fweb -fsingle-precision-constant
FLAGS    := \
	-noixemul \
	-msoft-float -w -O3 -m68040 -noixemul -fshort-enums -ffast-math \
	-finline-functions -fomit-frame-pointer -fexpensive-optimizations \
	
CC       := m68k-amigaos-gcc $(FLAGS) $(INCLUDE) $(DEFINES) -Wall
AS       := m68k-amigaos-as -m68040
AR       := m68k-amigaos-ar q
RANLIB   := m68k-amigaos-ranlib 

.PHONY: all
all: $(OBJECTS) $(CURDIR)/$(APPNAME)

$(CURDIR)/$(APPNAME):
	rm -f $(CURDIR)/$(APPNAME) 
	$(AR) $(CURDIR)/$(APPNAME) $(OBJECTS)
	$(RANLIB) $(CURDIR)/$(APPNAME)
#	$(CC) -c $(OBJECTS) $(APPNAME) $(LIBPATH) $(LIBS)

%.o: %.c
	$(CC) $(CCFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@

%.o: %.s
	$(AS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(CURDIR)/$(APPNAME) 

.PHONY: rebuild
rebuild: clean all

