AUTOMAKE_OPTIONS = foreign no-dependencies

bin_PROGRAMS = tdsproxy

tdsproxy_SOURCES = main.c

tdsproxy_LDADD = \
	$(top_builddir)/libevent-1.4.11-stable/libevent.la \
	$(top_builddir)/libevent-1.4.11-stable/libevent_core.la
INCLUDES = -I$(top_srcdir)/libevent-1.4.11-stable
