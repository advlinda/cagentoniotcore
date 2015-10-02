AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log
lib_LTLIBRARIES = libSAManager.la
libSAManager_la_SOURCES = dynamicloader.c SAManager.c  SAParser.c topic.c Keepalive.c ../../Platform/Linux/platform.c ../../Platform/Linux/common.c ../../Platform/Linux/NamedPipeClient.c
libSAManager_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -L$(top_srcdir)/Library/Log -lcJSON -lLog -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION)
# LDADD = $(top_srcdir)/Library/cJSON/libcJSON.a $(top_srcdir)/Library/Log/libLog.a $(INTLLIBS)
