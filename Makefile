# Makefile for building turbougly, the naive CSS minifer
# Check the README and turbougly.c files for details.

CFLAGS ?= -g -Weverything -Werror -pedantic -std=c99
INSTALL_PATH ?= /usr/local

all: turbougly tags
turbougly: turbougly.o
turbougly.o: turbougly.c

clean:
	rm -rvf *.o turbougly tags

tags: turbougly
	ctags turbougly.c

install:
	install ./turbougly $(INSTALL_PATH)/bin

uninstall:
	rm -i $(INSTALL_PATH)/bin/tblf

check: turbougly
	@find tests/ -type f -name \*.css -print -exec ./turbougly {} \;

.PHONY: README clean
