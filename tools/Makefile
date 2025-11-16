# 執行與輔助工具相關的任務。

CC ?= gcc
CFLAGS ?= -std=c17 -Wall -W -pedantic-errors $(DEBUG) `pkg-config --cflags --libs x11 xft fontconfig`
CTAGS ?= ctags
backup := $(wildcard *~)
srcs := $(wildcard *.c)
objs := $(srcs:.c=.o)
deps := $(srcs:.c=.d)
exec := checkfont 

.PHONY : all install install-strip uninstall clean
all : $(exec)
$(exec) : $(objs)
	@$(CTAGS) *.[ch] 2> /dev/null ; \
	$(CC) $(objs) -o $@ $(CFLAGS)
install :
	install -D -m 644 gwm.desktop $(prefix)/share/xsessions/gwm.desktop
	install -D -m 755 startgwm $(prefix)/bin/startgwm
	install -D -m 755 checkfont $(prefix)/bin/checkfont
install-strip :
	install -d $(prefix)/bin ;
	install -m 755 -s $(exec) $(prefix)/bin
uninstall :
	rm -f $(prefix)/bin/$(exec) $(prefix)/share/xsessions/gwm.desktop $(prefix)/bin/startgwm
clean :
	rm -f $(exec) $(objs) $(deps) $(backup)
%.d : %.c
	$(CC) -M $(CFLAGS) $< | sed 's/\($*\)\.o[ :]*/\1.o $@ :/g' > $@

ifneq ($(MAKECMDGOALS), install)
ifneq ($(MAKECMDGOALS), install-strip)
ifneq ($(MAKECMDGOALS), uninstall)
ifneq ($(MAKECMDGOALS), clean)
sinclude $(deps)
endif
endif
endif
endif
