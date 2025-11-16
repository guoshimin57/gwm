# 調用當前子目錄下的Makefile文件。

subdirs := LC_MESSAGES

.PHONY : all install install-strip uninstall clean
all :
	@set -e ; \
	for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir all ; \
	done ;
install :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir install ; \
	done ;
install-strip :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir install-strip ; \
	done ;
uninstall :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir uninstall ; \
	done ;
clean :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir clean ; \
	done ; \
	rm -f *~ ;
