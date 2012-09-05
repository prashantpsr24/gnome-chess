noinst_LTLIBRARIES = 
BUILT_SOURCES =

if ENABLE_NETWORKING
BUILT_SOURCES += \
	games-enum-types.c \
	games-enum-types.h

noinst_LTLIBRARIES += libgames-contacts.la
endif

networking_headers = \
	games-individual-manager.h \
	games-individual-store.h \
	games-individual-store-manager.h \
	games-individual-groups.h \
	games-ui-utils.h \
	games-contact.h \
	games-individual-view.h \
	games-cell-renderer-text.h \
	games-cell-renderer-expander.h \
	games-live-search.h \
	games-channel-request-utils.h


networking_sources = \
	$(networking_headers) \
	games-individual-manager.c \
	games-individual-store.c \
	games-individual-store-manager.c \
	games-individual-groups.c \
	games-ui-utils.c \
	games-contact.c \
	games-individual-view.c \
	games-cell-renderer-text.c \
	games-cell-renderer-expander.c \
	games-live-search.c \
	games-channel-request-utils.c


libgames_contacts_la_SOURCES = \
	$(networking_sources)

nodist_libgames_contacts_la_SOURCES = \
	$(BUILT_SOURCES)

libgames_contacts_la_CPPFLAGS = \
	-DPKGDATADIR="\"$(pkgdatadir)\""			\
	-DPREFIX="\"$(prefix)\"" \
	-DDATADIR="\"$(datadir)\""				\
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\""				\
	-DLOCALE_DIRECTORY="\"$(datadir)/locale\""				\
	-DGETTEXT_PACKAGE="\"gnome-chess\""				\
	-DG_LOG_DOMAIN="\"Games\""
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

if ENABLE_NETWORKING
dtddir = $(pkgdatadir)
dtd_DATA = \
	games-individual-groups.dtd

EXTRA_DIST += \
	$(dtd_DATA)
endif #ENABLE_NETWORKING


CLEANFILES =
if ENABLE_NETWORKING
CLEANFILES += \
	$(BUILT_SOURCES) \
	stamp-games-enum-types.h
endif #ENABLE_NETWORKING

DISTCLEANFILES =

# Generation of Enum types
games-enum-types.h: stamp-games-enum-types.h
	$(AM_V_GEN)true
stamp-games-enum-types.h: $(networking_headers) Makefile
	$(AM_V_GEN)(cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#ifndef __GAMES_ENUM_TYPES_H__\n" \
			--fhead "#define __GAMES_ENUM_TYPES_H__ 1\n\n" \
			--fhead "#include <glib-object.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __GAMES_ENUM_TYPES_H__ */\n" \
			--fprod "#include \"@filename@\"\n" \
			--eprod "#define GAMES_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(networking_headers) ) > xgen-gth \
	&& (cmp -s xgen-gth games-enum-type.h || cp xgen-gth games-enum-types.h) \
	&& rm -f xgen-gth \
	&& echo timestamp > $(@F)

games-enum-types.c: $(networking_headers) Makefile
	$(AM_V_GEN)(cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#include <config.h>\n" \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#include \"games-enum-types.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(networking_headers) ) > xgen-gtc \
	&& cp xgen-gtc $(@F) \
	&& rm -f xgen-gtc


@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

-include $(top_srcdir)/git.mk

