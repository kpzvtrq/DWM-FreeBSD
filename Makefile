.OBJDIR: ./

CC = cc

VERSION = 6.8

X11INC = /usr/local/include
FREETYPEINC = /usr/local/include/freetype2
X11LIB = /usr/local/lib

INCS = -I$(X11INC) -I$(FREETYPEINC) -Isrc

CPPFLAGS = \
	-D_DEFAULT_SOURCE \
	-D_BSD_SOURCE \
	-D_XOPEN_SOURCE=700L \
	-DVERSION=\"$(VERSION)\" \
	-DXINERAMA

CFLAGS = \
	-std=c99 \
	-pedantic \
	-Wall \
	-Wextra \
	-Wno-deprecated-declarations \
	-Os \
	$(CPPFLAGS) \
	$(INCS)

LIBS = \
	-L$(X11LIB) \
	-lX11 \
	-lXinerama \
	-lfontconfig \
	-lXft


TARGET = bwm

OBJDIR = obj

OBJS = \
	$(OBJDIR)/brw.o \
	$(OBJDIR)/bwm.o \
	$(OBJDIR)/util.o


all: $(TARGET)


$(OBJDIR):
	mkdir -p $(OBJDIR)


$(OBJDIR)/brw.o: $(OBJDIR) src/brw.c src/config.h
	$(CC) $(CFLAGS) -c src/brw.c -o $@

$(OBJDIR)/bwm.o: $(OBJDIR) src/bwm.c src/config.h
	$(CC) $(CFLAGS) -c src/bwm.c -o $@

$(OBJDIR)/util.o: $(OBJDIR) src/util.c src/config.h
	$(CC) $(CFLAGS) -c src/util.c -o $@


$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $@


clean:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)


install: $(TARGET)
	mkdir -p /usr/local/bin
	cp $(TARGET) /usr/local/bin


uninstall:
	rm -f /usr/local/bin/$(TARGET)


.PHONY: all clean install uninstall
