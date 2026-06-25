CC = gcc
CF = -g -std=c23 \
	  -Isrc \

OBJDIR = obj
TARGETDIR = bin
TARGET = may

SRC = src/main.c \

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
