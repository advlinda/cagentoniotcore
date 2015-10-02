AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON
lib_LTLIBRARIES = HandlerSample.la
HandlerSample_la_SOURCES = handler_api.c ../../Platform/Linux/platform.c ../../Platform/Linux/common.c
HandlerSample_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
