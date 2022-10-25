# *************************************************************************
#     Makefile：執行與源代碼相關的任務。
#     版權 (C) 2020-2022 gsm <406643764@qq.com>
#     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
# GNU通用公共許可證重新發布、修改本程序。
#     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
# 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
#     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
# <http://www.gnu.org/licenses/>。
# *************************************************************************

CC ?= gcc
CFLAGS ?= -std=c17 -Wall -pedantic-errors $(DEBUG) `pkg-config --cflags --libs x11 xft imlib2`
CTAGS ?= ctags
tag ?= tags
backup := $(wildcard *~)
srcs := $(wildcard *.c)
objs := $(srcs:.c=.o)
deps := $(srcs:.c=.d)
exec := gwm

.PHONY : all install install-strip uninstall clean
all : $(exec)
$(exec) : $(objs)
	@$(CTAGS) *.[ch] 2> /dev/null ; \
	$(CC) $(objs) -o $@ $(CFLAGS)
install :
	install -d $(prefix)/bin ;
	install -m 755 $(exec) $(prefix)/bin ;
install-strip :
	install -d $(prefix)/bin ;
	install -m 755 -s $(exec) $(prefix)/bin
uninstall :
	rm -f $(prefix)/bin/$(exec)
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
