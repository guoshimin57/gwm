# 執行當前目錄下與man1手冊頁相關的任務。

orig = $(wildcard ../../zh_HK/man1/*.1)
srcs = $(wildcard *.1)
objs = $(srcs:.1=.1.gz)
ln_objs = /usr/share/man/man1/$(objs)
install_path = $(prefix)/share/man/en_GB/man1

.PHONY : all install install-strip uninstall clean
all : $(objs)
$(objs) : $(srcs)
	gzip -kf $(srcs)
$(srcs) : $(orig)
	@echo "需要更新：$(realpath $(srcs))"
install :
	install -d  $(install_path) ;
	install -m 644 $(objs) $(install_path) ;
	$(RM) $(ln_objs) ;
	ln -s $(install_path)/$(objs) $(ln_objs)
install-strip:
uninstall :
	$(RM) $(install_path)/$(objs) $(ln_objs)
clean:
	$(RM) $(objs) *~
