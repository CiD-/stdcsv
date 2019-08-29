PREFIX		?= /usr/local
DESTDIR		?=

CC       	:= gcc
CFLAGS 	 	+= -Wall -Wextra
LDFLAGS  	+=
BUILD    	:= ./build
OBJ_DIR  	:= $(BUILD)/objects/
LIB_OBJ_DIR := $(OBJ_DIR)csv/
LIB_DIR		:= $(BUILD)/lib/
APP_DIR  	:= $(BUILD)/bin/
TARGET   	:= stdcsv
LIB_TARGET	:= libcsv.so
INCLUDE  	:= -Iinclude
LIB_SRC  	:= $(wildcard src/csv/*.c)
SRC      	:= $(wildcard src/*.c)

LIB_OBJECTS := $(LIB_SRC:%.c=$(OBJ_DIR)csv/%.o)
OBJECTS 	:= $(SRC:%.c=$(OBJ_DIR)%.o)

all: build lib stdcsv

$(LIB_OBJ_DIR)%.o: %.c
	@mkdir -p $(@D)
	$(CC) -fPIC $(CFLAGS) $(INCLUDE) -o $@ -c $<

$(LIB_DIR)$(LIB_TARGET): $(LIB_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -shared -o $(LIB_DIR)$(LIB_TARGET) $(LIB_OBJECTS)

$(OBJ_DIR)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -L$(LIB_DIR) -lcsv -o $(APP_DIR)$(TARGET) $(OBJECTS)

.PHONY: all build clean debug release install

build:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(APP_DIR)

lib:  $(LIB_DIR)$(LIB_TARGET)
stdcsv:  $(APP_DIR)$(TARGET)

debug: CFLAGS += -O0 -DDEBUG -g
debug: all

release: CFLAGS += -O3
release: all

install: install_lib install_bin

install_lib: $(LIB_DIR)$(LIB_TARGET)
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(LIB_DIR)$(LIB_TARGET) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 include/csv.h $(DESTDIR)$(PREFIX)/include/

install_bin: $(APP_DIR)$(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 644 $(APP_DIR)$(TARGET) $(DESTDIR)$(PREFIX)/bin/

clean:
	-@rm -rvf $(APP_DIR)*
	-@rm -rvf $(LIB_DIR)*
	-@rm -rvf $(OBJ_DIR)*
