# 執行當前目錄下與en_GB/LC_MESSAGES相關的本地化任務。

srcs = $(wildcard ../../../src/*.[ch])
po = $(wildcard *.po)
pot = $(po:.po=.pot)
mo = $(po:.po=.mo)
ln_mo = /usr/share/locale/en_US/LC_MESSAGES/$(mo)
install_path = $(prefix)/share/locale/en_GB/LC_MESSAGES

.PHONY : all install install-strip uninstall clean
all : $(mo)
$(mo) : $(po)
	msgfmt -o $@ $<
$(po) : $(srcs)
	@echo "可能需要更新：$(realpath gwm.po)"
install :
	install -d $(install_path) ;
	install -m 644 $(mo) $(install_path) ;
	$(RM) $(ln_mo) ;
	ln -s $(install_path)/$(mo) $(ln_mo)
install-strip:
uninstall :
	$(RM) $(install_path)/$(mo) $(ln_mo)
clean:
	$(RM) $(mo) $(pot) *~
