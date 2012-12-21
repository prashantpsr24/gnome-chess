AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(pkgdatadir)

AM_CFLAGS = \
	$(GNOME_CHESS_CFLAGS)

LDADD = \
	$(top_builddir)/libgames-contacts/libgames-contacts.la \
	$(GNOME_CHESS_LIBS)

networking_tests = \
	test-fetch-contacts

noinst_PROGRAMS =


if ENABLE_NETWORKING
noinst_PROGRAMS += \
	$(networking_tests)

AM_CFLAGS += \
	$(GEE_CFLAGS) \
	$(TELEPATHY_CFLAGS)
LDADD += \
	$(GEE_LIBS) \
	$(TELEPATHY_LIBS)
endif #ENABLE_NETWORKING

test_fetch_contacts_SOURCES = test-fetch-contacts.c
