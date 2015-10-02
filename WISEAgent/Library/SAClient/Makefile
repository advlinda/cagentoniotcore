include ../../common_config.mk
include ../../common_version.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := 
# for building library
lib_NAME := libSAClient

CJSON_INC_DIR :=$(LIB_DIR)/cJSON
LOG_INC_DIR := $(LIB_DIR)/Log

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/platform.o $(PLATFORM_LINUX_DIR)/common.o $(PLATFORM_LINUX_DIR)/network.o
program_SUBDIRS :=
CFLAGS += -Wall -fPIC
CPPFLAGS += -D_LINUX
LDFLAGS += 
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(INCLUDE_DIR) $(CJSON_INC_DIR) $(LOG_INC_DIR) $(LIB_DIR)/DES $(LIB_DIR)/Base64
program_LIBRARY_DIRS := $(CJSON_INC_DIR) $(LOG_INC_DIR) $(LIB_DIR)/Base64 $(LIB_DIR)/DES 
program_LIBRARIES := Base64 DES Log cJSON mosquitto pthread dl
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

