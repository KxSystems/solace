#ifndef KDBSOLACEEVENT_H_
#define KDBSOLACEEVENT_H_

#include <string>
#include "k.h"

#pragma pack(push, id, 1)

typedef enum
{
    QUEUE_MSG_EVENT,
    DIRECT_MSG_EVENT,
    SESSION_EVENT,
    FLOW_EVENT
} KDB_SOLACE_EVENT_TYPE;

struct KdbSolaceEventSessionDetail
{
    int         _eventType; 
    int         _responseCode;
    std::string _eventInfo;
};

struct KdbSolaceEventFlowDetail
{
    int         _eventType;
    int         _responseCode;
    std::string _eventInfo;
    int         _destType;
    std::string _destName;
};

struct KdbSolaceEventQueueMsg
{
    solClient_opaqueMsg_pt  _msg;
    solClient_opaqueFlow_pt _flow;
};

typedef union
{
    KdbSolaceEventQueueMsg          _queueMsg;
    KdbSolaceEventFlowDetail*       _flow;
    KdbSolaceEventSessionDetail*    _session;
    solClient_opaqueMsg_pt          _directMsg;
} KDB_SOLACE_EVENT; 

struct KdbSolaceEvent
{
    KDB_SOLACE_EVENT_TYPE   _type;
    KDB_SOLACE_EVENT        _event; 
};

#pragma pack(pop, id)

#endif

