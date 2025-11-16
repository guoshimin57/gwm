# 軟件包根目錄下的總控Makefile文件。

export MAKE = make
export prefix = /usr
subdirs = src man po tools test
doc = AUTHORS ChangeLog COPYING NEWS README THANKS TODO
package = gwm 
backup = $(wildcard *~)

.PHONY : all install install-strip uninstall clean test

all :
	@set -e ;
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir all ; \
	done

install :
	@set -e ;
	@for dir in $(subdirs) ; \
  	do \
		$(MAKE) -C $$dir install ; \
	done ;
	install -d $(prefix)/share/doc/$(package) ;
	install -m 644 $(doc) $(prefix)/share/doc/$(package)

install-strip :
	@set -e ;
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir install-strip ; \
	done ;
	install -d $(prefix)/share/doc/$(package) ;
	install -m 644 $(doc) $(prefix)/share/doc/$(package)

uninstall :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir uninstall ; \
	done ;
	rm -rf $(prefix)/share/doc/$(package)

clean :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir clean ; \
	done ;
	rm -rf $(backup)

test :
	@for dir in $(subdirs) ; \
	do \
		$(MAKE) -C $$dir test ; \
	done
