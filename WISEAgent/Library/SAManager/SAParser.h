#ifndef _SA_PARSER_H_
#define _SA_PARSER_H_

#include "susiaccess_def.h"

#define PJSON   void*

#ifdef __cplusplus
extern "C" {
#endif

char *SAParser_Print(PJSON item);

char *SAParser_PrintUnformatted(PJSON item);

void SAParser_Free(PJSON ptr);

PJSON SAParser_AgentBaseInfoToJSON(susiaccess_agent_profile_body_t const * pProfile, int status);

PJSON SAParser_CreateOSInfo(susiaccess_agent_profile_body_t const * pProfile);

PJSON SAParser_CreateAgentPacketToJSON(susiaccess_packet_body_t const * pPacket);

int SAParser_ParseRecvMessage(void* data, int datalen, susiaccess_packet_body_t * pkt);

#ifdef __cplusplus
}
#endif

#endif