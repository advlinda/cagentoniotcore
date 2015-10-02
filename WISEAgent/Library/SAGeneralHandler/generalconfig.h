#ifndef _CAGENT_AMT_CONFIG_H_
#define _CAGENT_AMT_CONFIG_H_
#include "susiaccess_def.h"

#define DEF_CONFIG_FILE_NAME	"agent_config.xml"

typedef struct {
	/*Intel AMT*/
	char autoReportEn[DEF_ENABLE_LENGTH];
	int reportDataLength;
	char* reportData;
}susiaccess_general_conf_body_t;

#ifdef __cplusplus
extern "C" {
#endif

int general_load(char const * configFile, susiaccess_general_conf_body_t * conf);
int general_save(char const * configFile, susiaccess_general_conf_body_t const * const conf);
int general_create(char const * configFile, susiaccess_general_conf_body_t * conf);
int general_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen);
int general_set(char const * const configFile, char const * const itemName, char const * const itemValue);

#ifdef __cplusplus
}
#endif

#endif