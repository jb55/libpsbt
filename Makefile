NAME ?= libpsbt
SHLIB ?= $(NAME).so
BIN = psbt
STATICLIB ?= $(NAME).a
PREFIX ?= /usr/local

CFLAGS = -g -O2 -fpic -Wall -Werror -Wextra -std=c99 \
						-Wno-unused-function \
						-Wno-unused-parameter \
						-Wno-unused-variable \
						-Wno-cast-align \
						-Wno-padded

OBJS += psbt.o
OBJS += compactsize.o

SRCS=$(OBJS:.o=.c)

all: $(SHLIB) $(STATICLIB) $(BIN)

include $(OBJS:.o=.d)

%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@

$(BIN): $(OBJS) cli.c
	$(CC) $(CFLAGS) cli.c $(OBJS) -o $@

$(STATICLIB): $(OBJS)
	ar rcs $@ $(OBJS)

$(SHLIB): $(OBJS)
	$(CC) -shared -o $@ $(OBJS)

install: $(STATICLIB) $(SHLIB)
	install -d $(PREFIX)/lib $(PREFIX)/include
	install $(STATICLIB) $(SHLIB) $(PREFIX)/lib
	install psbt.h $(PREFIX)/include

test: test.c $(OBJS)
	$(CC) $(CFLAGS) test.c $(OBJS) -o $@

TAGS:
	etags *.c

clean:
	rm -f $(OBJS) $(SHLIB) *.d*

.PHONY: TAGS
