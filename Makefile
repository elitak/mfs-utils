MYARCH := $(shell uname -m)
ARCH = $(MYARCH)
DEBUG=0
STATIC=0
USE_TRIDGE_MFS_SO=0

CFLAGS = -Wall
CCLDFLAGS =
PREFIX =

ifeq ($(ARCH),ppc)
# ARCH is ppc
PREFIX=powerpc-TiVo-linux
CFLAGS += -DTIVO -DTIVO_S1
else
# ARCH is mips or i386 (assuming kernel/glibc largefile support on i386)
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
ifeq ($(ARCH),mips)
# ARCH is mips
PREFIX=mips-TiVo-linux
CFLAGS += -mips2 -DTIVO -DTIVO_S2
#CCLDFLAGS += -static
else
# ARCH is i386
ARCH = i386
MYARCH = $(ARCH)
HOSTBINS = vplayer sd-h400_unlock vsplit
endif
endif


ifneq ($(ARCH),$(MYARCH))
CC = $(PREFIX)-gcc
AR = $(PREFIX)-ar
endif

ifeq ($(DEBUG),1)
CFLAGS += -O0 -ggdb
LIBS += -lefence -lpthread
else
CFLAGS += -O3
CCLDFLAGS += -Wl,--strip-all
endif

ifeq ($(STATIC),1)
CCLDFLAGS += -static
TRIDGE_MFS_LIB=$(OBJDIR)/libtridgemfs.a
else
ifeq ($(USE_TRIDGE_MFS_SO),1)
TRIDGE_MFS_LIB=$(BINDIR)/libtridgemfs.so.1.0
CCLDFLAGS += -Wl,-rpath,/usr/local/lib
else
TRIDGE_MFS_LIB=$(OBJDIR)/libtridgemfs.a
endif
endif

COMMON = mfs.c object.c util.c bitmap.c io.c partition.c \
	crc.c pri.c export.c schema.c query.c tzoffset.c tar.c \
	credits.c read_xml.c generate_xml.c generate_NowShowing.c attribute.c

BINS = \
 mfs_info mfs_ls mfs_streams mfs_dumpobj mfs_dumpschema mfs_tzoffset \
 mfs_import mfs_uberexport                              \
 mfs_export mfs_stream mfs_tarstream mfs_tmfstream      \
 tserver vserver NowShowing                             \
 vplay                                                  \
 mfs_dump mfs_poke                                      \
 mfs_bitmap mfs_purge mfs_getslice mfs_findzero

OBJDIR = obj.$(ARCH)
BINDIR = bin.$(ARCH)

.PHONY : all clean binaries mkdirs tags

all: proto.h mkdirs binaries

clean:
	rm -rf obj.* bin.* proto.h preload_schema.h *~

binaries: $(BINS:%=$(BINDIR)/%)  $(HOSTBINS:%=$(BINDIR)/%) 

mkdirs:
	mkdir -p $(OBJDIR) $(BINDIR)

tags:
	etags *.[ch]

mfs.h: proto.h

proto.h: $(COMMON)
	cat $(COMMON) | awk -f mkproto.awk > proto.h

.PRECIOUS : $(OBJDIR)/%.o


$(OBJDIR)/%.o : %.c mfs.h 
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/% : $(OBJDIR)/%.o $(TRIDGE_MFS_LIB)
	$(CC) $(CCLDFLAGS) -o $@ $^ $(LIBS)

$(BINDIR)/% : %.sh
	cp $^ $(subst .sh,,$@)

$(BINDIR)/vplayer : $(OBJDIR)/vplayer.o $(TRIDGE_MFS_LIB)
	$(CC) $(CCLDFLAGS) -o $@ $^ $(LIBS) -lncurses

$(BINDIR)/libtridgemfs.so.1.0: $(COMMON:%.c=$(OBJDIR)/%.o) $(SCHEMA:%.c=$(OBJDIR)/%.o)
	$(CC) -shared $(CCLDFLAGS) -Wl,-soname,libtridgemfs.so.1 -o $@ $^

$(OBJDIR)/libtridgemfs.a: $(COMMON:%.c=$(OBJDIR)/%.o) $(SCHEMA:%.c=$(OBJDIR)/%.o)
	$(AR) rc  $@ $^

schema.c: preload_schema.h

preload_schema.h: schema-5.4.txt
	perl make-preload-schema.pl <$< >$@
