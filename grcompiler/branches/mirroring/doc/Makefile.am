pkgdocdir = $(datadir)/doc/@PACKAGE@

dist_pkgdoc_DATA = CompilerDebugFiles.pdf CppDoc.pdf GDL.pdf GraphiteOverview.pdf

EXTRA_DIST = grcompiler.1.xml

# EXTRA_DIST = CompilerDebugFiles.pdf CppDoc.pdf GDL.pdf GraphiteOverview.pdf

man_MANS = grcompiler.1
DISTCLEANFILES = $(man_MANS)

grcompiler.1: $(srcdir)/grcompiler.1.xml
	docbook2x-man $(srcdir)/grcompiler.1.xml

