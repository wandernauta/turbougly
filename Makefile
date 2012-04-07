# Makefile for building turbougly, the naive CSS minifer
# Check the README and turbougly.c files for details.

CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=c99 -O4
INSTALL_PATH ?= /usr/local

all: turbougly tags
turbougly: turbougly.o
turbougly.o: turbougly.c lib/colnames.c
lib/colnames.c: lib/colnames.yml lib/colnames.py
	cd lib && python colnames.py | indent -br -nut -brf > colnames.c

clean:
	rm -rvf *.o turbougly tags lib/colnames.c

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
	strip $(INSTALL_PATH)/bin/turbougly

uninstall:
	rm -i $(INSTALL_PATH)/bin/tblf

check: turbougly
	@for f in `ls tests/*.css`; do printf "$$f: "; if (./turbougly $$f | diff - $$f.min); then echo "[PASS]"; else echo "[FAIL]"; fi; done

.PHONY: README clean splint todo perf mem
