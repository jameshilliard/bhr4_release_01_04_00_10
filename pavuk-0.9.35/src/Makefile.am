AM_CPPFLAGS = -DGETTEXT_DEFAULT_CATALOG_DIR="\"$(localedir)\"" \
              -DDEFAULTRC="\"${sysconfdir}/pavukrc\""

EXTRA_DIST = pavuk_logo.xpm \
	icons/append.xpm \
	icons/apply.xpm \
	icons/break.xpm \
	icons/break_small.xpm \
	icons/browse.xpm \
	icons/cancel.xpm \
	icons/clear.xpm \
	icons/close_small.xpm \
	icons/common.xpm \
	icons/configure.xpm \
	icons/continue.xpm \
	icons/continue_small.xpm \
	icons/delete.xpm \
	icons/exit.xpm \
	icons/gobg.xpm \
	icons/limit.xpm \
	icons/limits.xpm \
	icons/load.xpm \
	icons/maximize_small.xpm \
	icons/minimize_small.xpm \
	icons/modify.xpm \
	icons/ok.xpm \
	icons/restart.xpm \
	icons/restart_small.xpm \
	icons/save.xpm \
	icons/schedule.xpm \
	icons/stop.xpm \
	icons/stop_small.xpm \
	xpm/audio.xpm \
	xpm/binary.xpm \
	xpm/broken.xpm \
	xpm/cantaccess.xpm \
	xpm/compressed.xpm \
	xpm/ftpdir.xpm \
	xpm/gopherdir.xpm \
	xpm/html.xpm \
	xpm/image.xpm \
	xpm/incomplete.xpm \
	xpm/local.xpm \
	xpm/notprocessed.xpm \
	xpm/redirected.xpm \
	xpm/rejected.xpm \
	xpm/text.xpm \
	xpm/video.xpm \
	pavukrc.rc \
	pavukrc.h \
	pavuk.ico

DISTCLEANFILES = ../po/pavuk.pot
CLEANFILES = pavukrc.ro

bin_PROGRAMS = pavuk

pavuk_SOURCES = \
	absio.c \
	absio.h \
	abstract.c \
	abstract.h \
	ainterface.c \
	ainterface.h \
	authinfo.c \
	authinfo.h \
	base64.c \
	base64.h \
	bufio.c \
	bufio.h \
	charset.h \
	cleanup.c \
	cmdparse.c \
	condition.c \
	condition.h \
	config.c \
	config.h \
	cookie.c \
	cookie.h \
	css.c \
	css.h \
	debugl.h \
	debugl.c \
	decode.c \
	decode.h \
	digest_auth.c \
	digest_auth.h \
	dinfo.h \
	dinfo.c \
	dlhash.c \
	dlhash.h \
	dlhash_tools.c \
	dlhash_tools.h \
	dllist.c \
	dllist.h \
	dns.c \
	dns.h \
	doc.c \
	doc.h \
	errcode.c \
	errcode.h \
	file.c \
	file.h \
	fnmatch.c \
	fnmatch.h \
	form.c \
	form.h \
	form_encode.c \
	ftp.c \
	ftp.h \
	gaccel.c \
	gaccel.h \
	gauthinfo.c \
	gauthinfo.h \
	gcinfo.c \
	gcinfo.h \
	gkeys.c \
	gkeys.h \
	gopher.c \
	gopher.h \
	gprop.c \
	gprop.h \
	gtkmulticol.c \
	gtkmulticol.h \
	gui.h \
	gui_about.c \
	gui_api.c \
	gui_api.h \
	gui_addurl.c \
	gui_common.c \
	gui_jscons.c \
	gui_limits.c \
	gui_main.c \
	gui_scenario.c \
	gui_sched.c \
	gui_tools.c \
	gui_tree.c \
	html.c \
	html.h \
	htmlparser.c \
	htmlparser.h \
	htmltags.c \
	http.c \
	http.h \
	http_proxy.c \
	iecache.c \
	iecache.h \
	icons.c \
	icons.h \
	jsbind.c \
	jsbind.h \
	jstrans.c \
	jstrans.h \
	lang.h \
	lfname.c \
	lfname.h \
	log.c \
	log.h \
	md5c.c \
	md5c.h \
	md4c.c \
	md4c.h \
	mime.c \
	mime.h \
	mimetype.c \
	mimetype.h \
	mode.c \
	mode.h \
	mopt.c \
	mopt.h \
	mozcache.c \
	mozcache.h \
	mt.h \
	mt.c \
	myssl_nss.c \
	myssl_nss.h \
	myssl_openssl.c \
	myssl_openssl.h \
	myssl.h \
	net.c \
	net.h \
	nls.c \
	nls.h \
	nscache.c \
	nscache.h \
	ntlm_auth.c \
	ntlm_auth.h \
	options.h \
	pavuk.c \
	re.c \
	re.h \
	recurse.c \
	recurse.h \
	remind.c \
	remind.h \
	robots.c \
	robots.h \
	schedule.c \
	schedule.h \
	stats.c \
	stats.h \
	tag_pattern.c \
	tag_pattern.h \
	times.c \
	times.h \
	tools.c \
	tools.h \
	tr.h \
	tr.c \
	uconfig.h \
	uconfig.c \
	uexit.c \
	uexit.h \
	update_links.c \
	update_links.h \
	url.c \
	url.h


LDADD = @x_libs@

INCLUDES = $(x_cflags)

../po/pavuk.pot: $(pavuk_SOURCES)
	xgettext --default-domain=pavuk --keyword=gettext_nop -n -E $(pavuk_SOURCES) -o $@

pot: ../po/pavuk.pot

reindent:
	indent -npcs -nprs -npsl -nsaf -nsai -nsaw -nut -bli0 -nlp *.c *.h

@DEPS@

pavukrc.ro: pavukrc.rc pavukrc.h pavuk.ico

SUFFIXES = .rc .ro

.rc.ro:
	@WINDRES@ $< $@
