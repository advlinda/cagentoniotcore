include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := 
# for building library
lib_NAME := libsusihelper
SOVERSION := 1

XML2_INC_DIR := $(ROOTFS)/usr/include/libxml2

program_EXT_OBJS := 
program_SUBDIRS :=
CFLAGS += -Wall -fPIC
CPPFLAGS += -D_LINUX
LDFLAGS += 
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(SUSI_DIR) $(XML2_INC_DIR)
program_LIBRARY_DIRS := 
program_LIBRARIES := $(program_3rdPARTY_LIBRARIES)
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

