POFILES = $(PAVUK_PO)
MOFILES = $(PAVUK_MO)
POXFILES = $(PAVUK_POX)

# Unicode message catalogs
UPOFILES = $(PAVUK_UPO)
UMOFILES = $(PAVUK_UMO)

MSGFMT = @MSGFMT@

EXTRA_DIST = $(POFILES)
DISTCLEANFILES = $(POXFILES) $(UPOFILES) $(UMOFILES) $(MOFILES) $(PACKAGE).pot
CLEANFILES = $(POXFILES) $(UPOFILES) $(UMOFILES) $(MOFILES) $(PACKAGE).pot

SUFFIXES = .po .mo .pox .upo .umo

.po.mo:
	$(MSGFMT) @MSGFMTSTATS@ -o $@ $<

.upo.umo:
	$(MSGFMT) @MSGFMTSTATS@ -o $@ $<

%.pox: %.po $(PACKAGE).pot
	msgmerge $< $(PACKAGE).pot >$@

.po.upo:
	codep=`awk -F= '/Content-Type:/{split($$2,r,"\\\");print r[1]}' $<`; \
	$(ICONV) -f "$$codep" -t UTF-8 $< | sed "s/charset=$$codep/charset=UTF-8/" > $@

all: @PAVUK_UTFMODE@

mo: $(MOFILES)

pox: $(POXFILES)

umo: $(UMOFILES)

upo: $(UPOFILES)

pot: $(PACKAGE).pot

$(PACKAGE).pot:
	cd ../src && make pot

install-data-am:
	for lang in $(PAVUK_LANG); do \
	  install_dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkinstalldirs) $$install_dir; \
	  $(INSTALL_DATA) $$lang.@PAVUK_UTFMODE@ $$install_dir/$(PACKAGE).mo; \
	done
