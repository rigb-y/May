CC = gcc
CF = -g -std=c23 \
	  -Iinclude \
	  -Isrc \
	  -Isrc/alloc \
	  -Isrc/yield \
	  -Isrc/block \
	  -Isrc/freelist \
	  -Isrc/heapman

OBJDIR = obj
TARGETDIR = lib
TARGET = libmay.a

PREFIX ?= /usr/local
LIBDIR := $(PREFIX)/lib
INCLUDEDIR := $(PREFIX)/include

INCLUDE := include/may.h

SRC = src/alloc/mgrant.c \
	  src/yield/mrel.c \
	  src/block/block.c \
	  src/freelist/free_list.c \
	  src/heapman/heapman.c

OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

.PHONY: clean

ALL: $(TARGETDIR)/$(TARGET)

$(TARGETDIR)/$(TARGET): $(OBJ)
	@mkdir -p $(TARGETDIR)
	ar rcs $(TARGETDIR)/$(TARGET) $(OBJ) 

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CF) -c $< -o $@

install: $(TARGETDIR)/$(TARGET)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -m 644 $(TARGETDIR)/$(TARGET) $(DESTDIR)$(LIBDIR)/
	install -m 644 $(INCLUDE) $(DESTDIR)$(INCLUDEDIR)/

clean:
	rm -rf $(OBJDIR) $(TARGETDIR)
