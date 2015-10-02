#ifndef _CORE_PARSER_H_
#define _CORE_PARSER_H_

#include "SAClient.h"

#define PJSON   void*

#ifdef __cplusplus
extern "C" {
#endif

char *SAClientParser_Print(PJSON item);

char *SAClientParser_PrintUnformatted(PJSON item);

void SAClientParser_Free(PJSON ptr);

PJSON SAClientParser_AgentBaseInfoToJSON(susiaccess_agent_profile_body_t const * pProfile, int status);

PJSON SAClientParser_CreateOSInfo(susiaccess_agent_profile_body_t const * pProfile);

PJSON SAClientParser_CreateAgentPacketToJSON(susiaccess_packet_body_t const * pPacket);

int SAClientParser_ParseRecvMessage(void* data, int datalen, susiaccess_packet_body_t * pkt);

#ifdef __cplusplus
}
#endif

#endif