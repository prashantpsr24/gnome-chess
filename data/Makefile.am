SUBDIRS = icons pieces textures

desktopdir = $(datadir)/applications
desktop_in_files = gnome-chess.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

uidir = $(datadir)/gnome-chess
ui_DATA = \
	new-game-launcher.ui \
	gnome-chess-game-window.ui \
	gnome-chess-network-window.ui \
	preferences.ui \
	promotion-type-selector.ui

gsettings_SCHEMAS = org.gnome.gnome-chess.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

aidir = $(datadir)/gnome-chess/
ai_DATA = engines.conf

servicefiledir = $(datadir)/dbus-1/services
servicefile_in_files = \
	org.freedesktop.Telepathy.Client.Gnome.Chess.service.in
servicefile_DATA = $(servicefile_in_files:.service.in=.service)

org.freedesktop.Telepathy.Client.Gnome.Chess.service: org.freedesktop.Telepathy.Client.Gnome.Chess.service.in
	$(AM_V_GEN)sed -e "s|[@]libexecdir[@]|$(libexecdir)|" $< > $@

clientfiledir = $(datarootdir)/telepathy/clients
clientfile_DATA = \
	Gnome.Chess.client


man_MANS = gnome-chess.6

EXTRA_DIST = engines.conf \
	     org.gnome.gnome-chess.gschema.xml.in \
       $(clientfile_DATA) \
       $(servicefile_in_files) \
	     $(man_MANS) \
	     $(ui_DATA)

CLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)
DISTCLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS) $(servicefile_DATA)

-include $(top_srcdir)/git.mk
