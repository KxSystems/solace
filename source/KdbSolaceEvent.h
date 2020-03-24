#ifndef KDBSOLACEEVENT_H_
#define KDBSOLACEEVENT_H_

#include <string>
#include "k.h"

#pragma pack(push, id, 1)

typedef enum
{
    GUARANTEED_MSG_EVENT,
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

struct KdbSolaceEventGuarSubMsg
{
    solClient_opaqueMsg_pt  _msg;
    solClient_opaqueFlow_pt _opaqueFlow;
    int                     _destType;
    std::string             _destName;
    K                       _vals;
};

struct KdbSolaceEvent
{
    KDB_SOLACE_EVENT_TYPE           _type;
    union
    {
        KdbSolaceEventGuarSubMsg*       _subMsg;
        KdbSolaceEventFlowDetail*       _flow;
        KdbSolaceEventSessionDetail*    _session;
        solClient_opaqueMsg_pt          _directMsg;
    } _event; 
};

#pragma pack(pop, id)

#endif

