# *************************************************************************
#     Makefile：軟件包根目錄下的總控Makefile文件。
#     版權 (C) 2020 gsm <406643764@qq.com>
#     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
# GNU通用公共許可證重新發布、修改本程序。
#     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
# 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
#     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
# <http://www.gnu.org/licenses/>。
# *************************************************************************

export MAKE := make
export prefix := /usr
subdirs := src man tools
doc := AUTHORS ChangeLog COPYING NEWS README THANKS TODO
package := gwm 

.PHONY : all install install-strip uninstall clean
all :
	@set -e ; \
	for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir all ; \
	done ;
	@echo "編譯成功！"
install :
	@set -e ; \
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir install ; \
	done ;
	install -d $(prefix)/share/doc/$(package) ;
	install -m 644 $(doc) $(prefix)/share/doc/$(package) ;
	@echo "安裝成功！" ;
install-strip :
	@set -e ; \
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir install-strip ; \
	done ;
	install -d $(prefix)/share/doc/$(package) ;
	install -m 644 $(doc) $(prefix)/share/doc/$(package) ;
	@echo "安裝成功！" ;
uninstall :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir uninstall ; \
	done ;
	rm -rf $(prefix)/share/doc/$(package) ;
clean :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir clean ; \
	done ;
	rm -rf *~
