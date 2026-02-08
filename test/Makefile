# 構建及運行測試程序。

CC ?= gcc
DEBUG ?= -ggdb3
libs = x11 xft imlib2 fontconfig
CFLAGS ?= -std=c17 -Wall -Wextra -pedantic-errors $(DEBUG) \
		 `pkg-config --cflags --libs $(libs)`
LDFLAGS ?= `pkg-config --libs $(libs)` -lm
CTAGS ?= ctags
backup = $(wildcard *~)
src_dir = ../src
test_dir = .
src_srcs = $(filter-out $(src_dir)/main.c, $(wildcard $(src_dir)/*.c))
test_srcs = $(wildcard $(test_dir)/*.c)
src_objs = $(src_srcs:.c=.o)
test_objs = $(test_srcs:.c=.o)
src_deps = $(src_srcs:.c=.d)
test_deps = $(test_srcs:.c=.d)
deps = $(src_deps) $(test_deps)
exes = $(test_srcs:.c=)

.PHONY : all test install install-strip uninstall clean

all : $(exes)
	@$(CTAGS) $(src_dir)/*.[ch] $(test_dir)/*.[ch] 2> /dev/null || true

$(exes) : % : %.o $(src_objs)
	$(CC) $< $(filter-out $(src_dir)/$(@:t%=%).o, $(src_objs)) -o $@ $(LDFLAGS)
	@echo "$(CC) : $@ ... [完成]"

clean :
	rm -f $(exes) $(test_objs) $(test_deps) $(backup)

test : $(exes)
	@for exe in $(exes); \
	do \
		echo -n "正在測試$$exe ... "; \
		if $$exe ; \
		then \
			echo "[通過]"; \
		else \
			echo "[失敗]"; exit 1; \
		fi; \
	done ; \
	echo "全部測試通過"

%.d : %.c
	$(CC) -MM $(CFLAGS) $< | sed 's|\($*\)\.o[ :]*|\1.o $@ :|g' > $@

ifneq ($(MAKECMDGOALS), clean)
sinclude $(deps)
endif
