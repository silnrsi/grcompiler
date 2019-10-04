pkgdocdir = $(datadir)/doc/@PACKAGE@

dist_pkgdoc_DATA = CompilerDebugFiles.pdf GDL.pdf GraphiteOverview.pdf README.gdlpp

EXTRA_DIST = grcompiler.1.xml gdlpp.1.in CompilerDebugFiles.rtf GDL.rtf GraphiteOverview.rtf 

man_MANS = grcompiler.1 gdlpp.1
MOSTLYCLEANFILES = $(man_MANS)

grcompiler.1: $(srcdir)/grcompiler.1.xml
	$(DOCBOOK2MAN) $(srcdir)/grcompiler.1.xml

gdlpp.1: $(srcdir)/gdlpp.1.in
	cp $(srcdir)/gdlpp.1.in gdlpp.1
