noinst_LTLIBRARIES = 

if ENABLE_NETWORKING
noinst_LTLIBRARIES += libgames-contacts.la
endif

libgames_contacts_la_SOURCES =
	
libgames_contacts_la_CPPFLAGS = \
	-DPKGDATADIR="\"$(pkgdatadir)\""			\
	-DPREFIX="\"$(prefix)\"" \
	-DDATADIR="\"$(datadir)\""				\
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\""				\
	-DLOCALE_DIRECTORY="\"$(datadir)/locale\""				\
	-DGETTEXT_PACKAGE="\"gnome-chess\""				\
	$(AM_CPPFLAGS)

libgames_contacts_la_CFLAGS = \
	$(GNOME_CHESS_CFLAGS) \
	$(AM_CFLAGS) \
	$(FOLKS_CFLAGS) \
	$(GEE_CFLAGS) \
	$(XML_CFLAGS)

libgames_contacts_la_LIBADD = \
	$(GNOME_CHESS_LIBS) \
	$(FOLKS_LIBS) \
	$(GEE_LIBS) \
	$(XML_LIBS)

EXTRA_DIST = \
	GamesContacts-1.0.vapi

CLEANFILES =

DISTCLEANFILES =

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

-include $(top_srcdir)/git.mk
