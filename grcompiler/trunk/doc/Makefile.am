pkgdocdir = $(datadir)/doc/@PACKAGE@

dist_pkgdoc_DATA = CompilerDebugFiles.pdf CppDoc.pdf GDL.pdf GraphiteOverview.pdf

EXTRA_DIST = grcompiler.1.xml gdlpp.1.in CompilerDebugFiles.rtf GDL.rtf GraphiteOverview.rtf

# EXTRA_DIST = CompilerDebugFiles.pdf CppDoc.pdf GDL.pdf GraphiteOverview.pdf

man_MANS = grcompiler.1 gdlpp.1
MOSTLYCLEANFILES = $(man_MANS)

grcompiler.1: $(srcdir)/grcompiler.1.xml
	$(DOCBOOK2MAN) $(srcdir)/grcompiler.1.xml

gdlpp.1: $(srcdir)/gdlpp.1.in
	cp $(srcdir)/gdlpp.1.in gdlpp.1
