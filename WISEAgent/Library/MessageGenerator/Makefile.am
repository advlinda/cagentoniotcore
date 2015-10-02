AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Library/cJSON
lib_LIBRARIES = libmsggen.a
libmsggen_a_SOURCES = MsgGenerater.c DeviceMessageGenerate.c GPSMessageGenerate.c IoTMessageGenerate.c ../../Platform/Linux/platform.c
AM_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION)
