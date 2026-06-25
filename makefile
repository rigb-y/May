CC = gcc
CF = -g -std=c23 \
	  -Iinclude \
	  -Isrc \
	  -Isrc/alloc \
	  -Isrc/yield \
	  -Isrc/block \
	  -Isrc/freelist

OBJDIR = obj
TARGETDIR = bin
TARGET = may

SRC = src/main.c \
	  src/alloc/mgrant.c \
	  src/yield/mrel.c \
	  src/block/block.c \
	  src/freelist/free_list.c

OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

.PHONY: clean

ALL: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(TARGETDIR)
	$(CC) $(OBJ) -o $(TARGETDIR)/$(TARGET) 

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CF) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGETDIR)
