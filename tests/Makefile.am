AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(pkgdatadir)

AM_CFLAGS = \
	$(GNOME_CHESS_CFLAGS)

LDADD = \
	$(GNOME_CHESS_LIBS)

telepathy_networking_tests = \
	test-fetch-contacts \
	offerer \
	accepter \
	test-tube-app

plain_tests = \
	test-demo-gdbus-client \
	test-demo-gdbus-server

noinst_PROGRAMS = \
	$(plain_tests)

if ENABLE_NETWORKING
noinst_PROGRAMS += \
	$(telepathy_networking_tests)

AM_CFLAGS += \
	$(GEE_CFLAGS) \
	$(TELEPATHY_CFLAGS) \
	$(GIO_CFLAGS)
LDADD += \
	$(top_builddir)/libgames-contacts/libgames-contacts.la \
	$(GEE_LIBS) \
	$(TELEPATHY_LIBS) \
	$(GIO_LIBS)

VALAFLAGS = \
	--pkg gio-2.0 \
	--pkg telepathy-glib
endif #ENABLE_NETWORKING


test_fetch_contacts_SOURCES = test-fetch-contacts.c

test_demo_gdbus_server_SOURCES = test-demo-gdbus-server.vala
test_demo_gdbus_client_SOURCES = test-demo-gdbus-client.vala

offerer_SOURCES = offerer.c constants.h
accepter_SOURCES = accepter.c constants.h

test_tube_app_SOURCES = test-tube-app.vala
