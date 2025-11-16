# 執行當前目錄下與zh_CN/LC_MESSAGES相關的本地化任務。

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
