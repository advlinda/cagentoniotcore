include ../../common_config.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := 
# for building library
lib_NAME := libmsggen
SOVERSION := 1

CJSON_INC_DIR :=$(LIB_DIR)/cJSON

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/platform.o
program_SUBDIRS :=
CFLAGS += -Wall -fPIC
CPPFLAGS += -D_LINUX
LDFLAGS += 
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(INCLUDE_DIR) $(CJSON_INC_DIR)
program_LIBRARY_DIRS := $(CJSON_INC_DIR)
program_LIBRARIES := cJSON
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk
