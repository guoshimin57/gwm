#*************************************************************************
#     Makefile：執行當前目錄下與zh_CN/LC_MESSAGES相關的本地化任務。
#     版權 (C) 2020-2025 gsm <406643764@qq.com>
#     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
# GNU通用公共許可證重新發布、修改本程序。
#     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
# 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
#     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
# <http://www.gnu.org/licenses/>。
#*************************************************************************

srcs = $(wildcard ../../../src/*.[ch])
po = $(wildcard *.po)
pot = $(po:.po=.pot)
mo = $(po:.po=.mo)
install_path= $(prefix)/share/locale/zh_CN/LC_MESSAGES

.PHONY : all install install-strip uninstall clean
all : $(mo)
$(mo) : $(po)
	msgfmt -o $@ $<
$(po) : $(srcs)
	@echo "可能需要更新：$(realpath gwm.po)"
install :
	install -d $(install_path) ;
	install -m 644 $(mo) $(install_path) ;
install-strip:
uninstall :
	$(RM) $(mo) $(install_path)
clean:
	$(RM) $(mo) $(pot) *~
