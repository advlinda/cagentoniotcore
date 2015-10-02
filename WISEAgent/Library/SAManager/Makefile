include ../../common_config.mk
include ../../common_version.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := 
# for building library
lib_NAME := libSAManager

CJSON_INC_DIR :=$(LIB_DIR)/cJSON
LOG_INC_DIR := $(LIB_DIR)/Log


program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/platform.o $(PLATFORM_LINUX_DIR)/common.o $(PLATFORM_LINUX_DIR)/NamedPipeClient.o

program_SUBDIRS :=
CFLAGS += -Wall -fPIC
CPPFLAGS += -D_LINUX
LDFLAGS += 
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(INCLUDE_DIR) $(CJSON_INC_DIR) $(LOG_INC_DIR)
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(LOG_INC_DIR) $(CJSON_INC_DIR)
program_LIBRARIES := cJSON Log pthread dl
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

