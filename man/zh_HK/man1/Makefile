# *************************************************************************
#     Makefile：執行當前目錄下與man1手冊頁相關的任務。
#     版權 (C) 2020 gsm <406643764@qq.com>
#     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
# GNU通用公共許可證重新發布、修改本程序。
#     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
# 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
#     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
# <http://www.gnu.org/licenses/>。
# *************************************************************************

srcs := $(wildcard *.1)
objs := $(srcs:.1=.1.gz)

.PHONY : all install install-strip uninstall clean
all : $(objs)
$(objs) : $(srcs)
	gzip -kf $(srcs)
install :
	install -d  $(prefix)/share/man/zh_HK/man1 ;
	install -m 644 $(objs) $(prefix)/share/man/zh_HK/man1 ;
install-strip:
uninstall :
	cd $(prefix)/share/man/zh_HK/man1 ;
	rm -f $(objs) ;
clean:
	rm -f $(objs) *~
