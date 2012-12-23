AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(pkgdatadir)

AM_CFLAGS = \
	$(GNOME_CHESS_CFLAGS)

LDADD = \
	$(top_builddir)/libgames-contacts/libgames-contacts.la \
	$(GNOME_CHESS_LIBS)

test_demo_gdbus_server_CFLAGS = \
	$(GNOME_CHESS_CFLAGS)

test_demo_gdbus_server_LDADD = \
	$(GNOME_CHESS_LIBS)

test_demo_gdbus_client_CFLAGS = \
	$(GNOME_CHESS_CFLAGS)

test_demo_gdbus_client_LDADD = \
	$(GNOME_CHESS_LIBS)

test_demo_gdbus_server_VALAFLAGS = \
	--pkg gio-2.0

test_demo_gdbus_client_VALAFLAGS = \
	--pkg gio-2.0

networking_tests = \
	test-fetch-contacts \
	offerer \
	accepter

vala_tests = \
	test-demo-gdbus-client \
	test-demo-gdbus-server

noinst_PROGRAMS = \
	$(vala_tests)

if ENABLE_NETWORKING
noinst_PROGRAMS += \
	$(networking_tests)

AM_CFLAGS += \
	$(GEE_CFLAGS) \
	$(TELEPATHY_CFLAGS) \
	$(GIO_CFLAGS)
LDADD += \
	$(GEE_LIBS) \
	$(TELEPATHY_LIBS) \
	$(GIO_LIBS)
endif #ENABLE_NETWORKING

test_fetch_contacts_SOURCES = test-fetch-contacts.c

test_demo_gdbus_server_SOURCES = test-demo-gdbus-server.vala
test_demo_gdbus_client_SOURCES = test-demo-gdbus-client.vala

offerer_SOURCES = offerer.c constants.h
accepter_SOURCES = accepter.c constants.h

