AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log
lib_LTLIBRARIES = ProcessMonitorHandler.la
ProcessMonitorHandler_la_SOURCES = Parser.c ProcessMonitorHandler.c ../../Platform/Linux/platform.c
ProcessMonitorHandler_la_LDFLAGS = $(XML_LIBS) -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module
