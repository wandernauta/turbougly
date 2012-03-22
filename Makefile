# Makefile for building turbougly, the naive CSS minifer
# Check the README and turbougly.c files for details.

CFLAGS ?= -Weverything -Werror -pedantic -std=c99 -O4 -ftrapv
INSTALL_PATH ?= /usr/local
DIFF ?= colordiff

all: turbougly tags
turbougly: turbougly.o
turbougly.o: turbougly.c

clean:
	rm -rvf *.o turbougly tags

splint:
	splint turbougly.c

perf:
	valgrind --tool=callgrind ./turbougly tests/simple.css

mem:
	valgrind ./turbougly tests/simple.css

todo:
	@egrep -n --color=NEVER -o "(TODO|FIXME)(.*)" turbougly.c

tags: turbougly
	ctags turbougly.c

install:
	install ./turbougly $(INSTALL_PATH)/bin

uninstall:
	rm -i $(INSTALL_PATH)/bin/tblf

check: turbougly
	@for f in `ls tests/*.css`; do if (./turbougly $$f | $(DIFF) - $$f.min); then echo "[PASS] $$f"; else echo "[FAIL] $$f"; fi; done

.PHONY: README clean splint todo perf mem
