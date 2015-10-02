#ifndef _CAGENT_PROFILE_H_
#define _CAGENT_PROFILE_H_
#include "susiaccess_def.h"

#define DEF_CONFIG_FILE_NAME	"agent_config.xml"

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SACONFIG_API
#define SACONFIG_API WINAPI
#endif
#else
#define SACONFIG_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

int SACONFIG_API profile_load(char const * configFile, susiaccess_agent_profile_body_t * profile);
int SACONFIG_API profile_save(char const * configFile, susiaccess_agent_profile_body_t const * const profile);
int SACONFIG_API profile_create(char const * configFile, susiaccess_agent_profile_body_t * profile);
int SACONFIG_API profile_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen);
int SACONFIG_API profile_set(char const * const configFile, char const * const itemName, char const * const itemValue);

#ifdef __cplusplus
}
#endif

#endif