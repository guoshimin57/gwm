# *************************************************************************
#     Makefile：構建及運行測試程序。
#     版權 (C) 2025 gsm <406643764@qq.com>
#     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
# GNU通用公共許可證重新發布、修改本程序。
#     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
# 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
#     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
# <http://www.gnu.org/licenses/>。
# *************************************************************************

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
	@$(CTAGS) $(src_dir)/*.[ch] $(test_dir)/*.[ch] 2> /dev/null

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
