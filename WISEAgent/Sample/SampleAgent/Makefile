include ../../common_config.mk
include ../../common_version.mk

############################################################################
# Beginming of Developer Modification 
############################################################################
# for building program
application_NAME := sampleagent
# for building library
lib_NAME := 
SOVERSION := 

program_EXT_OBJS := $(PLATFORM_LINUX_DIR)/platform.o $(PLATFORM_LINUX_DIR)/common.o
program_SUBDIRS :=
CFLAGS += -Wall
CPPFLAGS += -D_LINUX
LDFLAGS += -Wl,-rpath,./
program_INCLUDE_DIRS := $(PLATFORM_LINUX_DIR) $(application_LIBSDIR)  $(INCLUDE_DIR)
program_LIBRARY_DIRS := $(PROJECT_ROOT_DIR) $(LIB_DIR)/Log $(LIB_DIR)/SAClient $(LIB_DIR)/SAConfig
program_LIBRARIES :=  SAClient SAConfig $(program_3rdPARTY_LIBRARIES) Log pthread dl rt
############################################################################
# End of Developer Modification 
############################################################################

include ../../common_rule.mk

