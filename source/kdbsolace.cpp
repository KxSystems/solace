
#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"q.lib")
#endif
#include "socketpair.c"
#include "solclient/solClient.h"
#include "solclient/solClientMsg.h"
#include "KdbSolaceEvent.h"
#include <string.h>
#include <string>
#include <map>
#include <queue>
#include "log/ThreadID.h"
#include <stdlib.h>
#include <fcntl.h>
#include "kdbsolace.h"

/**
 * UTILITY FUNCTIONS/MACROS
 **/
#define CHECK_SESSION_CREATED {if (session_p == NULL) return krr((S)"Solace function called without initialising solace");}
#define CHECK_PARAM_TYPE(x,y,z) {if (x->t != y) return krr((S)"Function " #z " called with incorrect param type for " #x "");}
#define CHECK_PARAM_STRING_TYPE(x,z) {if (x->t != -KS && x->t != KC) return krr((S)"Function " #z " called with incorrect param type for " #x "");}
#define CHECK_PARAM_DATA_TYPE(x,z) {if (x->t != KG && x->t != -KS && x->t != KC) return krr((S)"Function " #z " called with incorrect param type for " #x "");}
#define CHECK_PARAM_DICT_SYMS(x,z) {if (x->t != 99) return krr((S)"Function " #z " called with incorrect dict param type"); if ((kK(x)[0])->n>0 && (kK(x)[0])->t != 11 && (kK(x)[1])->t != 11) return krr((S)"Function " #z " expecting symbols in provided dict");}

int getDataSize(K d)
{
    return (d->t == -KS) ? strlen(d->s) : d->n;
}

void* getData(K d)
{
    return (d->t == -KS) ? d->s : (void*)d->G0;
}

char* createString(K in)
{
    char* newStr = NULL;
    int len = 1;
    if (in->t == KC)
    {
        len = (in->n) + 1;
        newStr = (char*)malloc(len);
        memcpy(newStr,in->G0,len-1);
    }
    else if (in->t == -KS)
    {
        len = strlen(in->s)+1;
        newStr = (char*)malloc(len);
        memcpy(newStr,in->s,len-1);
    }
    else
        newStr = (char*)malloc(len);
    newStr[len-1]='\0';
    return newStr;
}

const char** createProperties(K options)
{
    K propNames = (kK(options)[0]);
    K propValues = (kK(options)[1]);
    int propsSize = (propNames->n*2)+1; // option names and option values, with a null at end
    const char** props =  (const char**)malloc(sizeof(char*)*propsSize);
    int propIndex = 0;
    for (int row=0;row<propNames->n;++row)
    {
        props[propIndex++] = kS(propNames)[row];
        props[propIndex++] = kS(propValues)[row];
    }
    props[propIndex] = NULL;
    return props;
}

/* session */
static solClient_opaqueSession_pt session_p = NULL;
static solClient_opaqueContext_pt context = NULL;

#ifdef _WIN32
static SOCKET SPAIR[2];
#else
static int SPAIR[2];
#endif
static std::string KDB_SESSION_EVENT_CALLBACK_FUNC;
static std::string KDB_FLOW_EVENT_CALLBACK_FUNC;
static std::string KDB_DIRECT_MSG_CALLBACK_FUNC;
static std::string KDB_QUEUE_MSG_CALLBACK_FUNC;

static std::map<std::string, solClient_opaqueFlow_pt> QUEUE_SUB_INFO;

static std::queue<KdbSolaceEvent> BLOCKED_MSGS;
static std::string BLOCKED_PARTIAL_SEND_DATA;
static bool CURRENTLY_BLOCKED = false;

bool storePartialWrite(char* data, int dataSize, int numWritten)
{
    if (numWritten > 0)
    {
        BLOCKED_PARTIAL_SEND_DATA.assign(data+numWritten,dataSize-numWritten);
        return true;
    }
    return false;
}

static void socketWrittableCbFunc(solClient_opaqueContext_pt opaqueContext_p, solClient_fd_t fd, solClient_fdEvent_t events, void *user_p)
{
    if (!BLOCKED_PARTIAL_SEND_DATA.empty())
    {
        int numWritten = send(SPAIR[1], BLOCKED_PARTIAL_SEND_DATA.c_str(), BLOCKED_PARTIAL_SEND_DATA.size(), 0);
        if (numWritten != (int)BLOCKED_PARTIAL_SEND_DATA.size())
        {
            if (numWritten >= 0)
                BLOCKED_PARTIAL_SEND_DATA.assign(BLOCKED_PARTIAL_SEND_DATA.c_str()+numWritten,BLOCKED_PARTIAL_SEND_DATA.size()-numWritten);
            return;
        }
    }
    while (!BLOCKED_MSGS.empty())
    {
        KdbSolaceEvent msgAndSource = BLOCKED_MSGS.front();
        int numWritten = send(SPAIR[1], (char*)&msgAndSource, sizeof(msgAndSource), 0);
        if (numWritten != sizeof(msgAndSource))
        {   
            if (storePartialWrite((char*)&msgAndSource,sizeof(msgAndSource),numWritten))
                BLOCKED_MSGS.pop();
            return;
        }
        BLOCKED_MSGS.pop();
    }
    CURRENTLY_BLOCKED = false;
    solClient_context_unregisterForFdEvents(opaqueContext_p,fd,events);
}

void sendMsg(const KdbSolaceEvent* event)
{
    if (CURRENTLY_BLOCKED)
    {
        BLOCKED_MSGS.push(*event);
        return;
    }
    int numWritten = send(SPAIR[1], (char*)event, sizeof(KdbSolaceEvent), 0);
    if (numWritten != sizeof(KdbSolaceEvent))
    {
        if (!storePartialWrite((char*)event,sizeof(KdbSolaceEvent),numWritten))
            BLOCKED_MSGS.push(*event);
        CURRENTLY_BLOCKED = true;
        if (solClient_context_registerForFdEvents(context,SPAIR[1],SOLCLIENT_FD_EVENT_WRITE,socketWrittableCbFunc,NULL) != SOLCLIENT_OK)
            printf("[%ld] Solace problem create fd monitor\n", THREAD_ID);
    }
}

void setReplyTo(K replyType, K replydest, solClient_opaqueMsg_pt msg_p)
{
    if ((replydest != NULL) && (replydest->t == -KS) && (replyType->t == -KI) && (strlen(replydest->s) > 0))
    {
        solClient_destination_t replydestination = {(solClient_destinationType_t)replyType->i, replydest->s};
        solClient_msg_setReplyTo ( msg_p, &replydestination, sizeof ( replydestination ) );
    }
}

void kdbCallbackSessionEvent(const KdbSolaceEventSessionDetail* event)
{
    if (!KDB_SESSION_EVENT_CALLBACK_FUNC.empty())
    {
        K eventtype = ki(event->_eventType);
        K responsecode = ki(event->_responseCode);
        K eventinfo = kp((char*)event->_eventInfo.c_str()); 
        K result = k(0, (char*)KDB_SESSION_EVENT_CALLBACK_FUNC.c_str(), eventtype, responsecode, eventinfo, (K)0);
        if(-128 == result->t)
        {
            printf("[%ld] Solace not able to call kdb function %s with received data (eventtype:%d responsecode:%d eventinfo:%s)\n", 
                    THREAD_ID, 
                    KDB_SESSION_EVENT_CALLBACK_FUNC.c_str(),
                    event->_eventType,
                    event->_responseCode,
                    event->_eventInfo.c_str());
        }
    }
    delete (event);
}

void kdbCallbackFlowEvent(const KdbSolaceEventFlowDetail* event)
{
    if (!KDB_FLOW_EVENT_CALLBACK_FUNC.empty())
    {
        K eventtype = ki(event->_eventType);
        K responsecode = ki(event->_responseCode);
        K eventinfo = kp((char*)event->_eventInfo.c_str());
        K destType = ki(event->_destType);
        K destName = kp((char*)event->_destName.c_str());
        K result = k(0, (char*)KDB_FLOW_EVENT_CALLBACK_FUNC.c_str(), eventtype, responsecode, eventinfo, destType, destName, (K)0);
        if(-128 == result->t)
            printf("[%ld] Solace not able to call kdb function %s\n", THREAD_ID, KDB_FLOW_EVENT_CALLBACK_FUNC.c_str()); 
    }
    delete (event);    
}

void kdbCallbackDirectMsgEvent(solClient_opaqueMsg_pt msg)
{
    if (KDB_DIRECT_MSG_CALLBACK_FUNC.empty())
    {
        solClient_msg_free(&msg);
        return;
    }
    const char* destination = "";
    solClient_destination_t msgDest;
    if (solClient_msg_getDestination(msg,&msgDest,sizeof(msgDest)) == SOLCLIENT_OK)
        destination = msgDest.dest;
    solClient_destination_t replyDest;
    bool isRequest = (solClient_msg_getReplyTo(msg,&replyDest,sizeof(replyDest)) == SOLCLIENT_OK);
    void* dataPtr = NULL;
    solClient_uint32_t dataSize = 0;
    solClient_msg_getBinaryAttachmentPtr(msg, &dataPtr, &dataSize);
    K payload = ktn(KG, dataSize);
    memcpy(payload->G0, dataPtr, dataSize);

    solClient_bool_t redelivered = solClient_msg_isRedelivered(msg);
    solClient_bool_t discarded = solClient_msg_isDiscardIndication(msg);
    solClient_int64_t sendTime = 0;
    solClient_msg_getSenderTimestamp(msg,&sendTime);
    if (sendTime>0)
        sendTime=(sendTime*1000000l)-(946684800l*1000000000l);
                
    K keys = ktn(KS,4);
    kS(keys)[0]=ss((char*)"isRedeliv");
    kS(keys)[1]=ss((char*)"isDiscard");
    kS(keys)[2]=ss((char*)"isRequest");
    kS(keys)[3]=ss((char*)"sendTime");
    K vals = knk(4,kb(redelivered),kb(discarded),kb(isRequest),ktj(-KP,sendTime));
    K dict = xD(keys,vals);
    K replyK = k(0,(char*)KDB_DIRECT_MSG_CALLBACK_FUNC.c_str(),ks((char*)destination),payload,dict,(K)0);
    if ((isRequest && replyK!=NULL) && (replyK->t == KG || replyK->t == -KS || replyK->t == KC))
    {
        solClient_opaqueMsg_pt replyMsg = NULL;
        solClient_msg_alloc (&replyMsg);
        solClient_msg_setBinaryAttachment ( replyMsg, getData(replyK), getDataSize(replyK));
        solClient_session_sendReply(session_p,msg,replyMsg);
        solClient_msg_free(&replyMsg);
    }
    solClient_msg_free(&msg);
}

void kdbCallbackQueueMsgEvent(const KdbSolaceEventQueueMsg* msgEvent)
{
    if (KDB_QUEUE_MSG_CALLBACK_FUNC.empty())
        return;

    solClient_opaqueMsg_pt msg = msgEvent->_msg;
    solClient_opaqueFlow_pt flow = msgEvent->_flow;
    // check flow still going
    std::map<std::string, solClient_opaqueFlow_pt>::iterator itr = QUEUE_SUB_INFO.begin();
    while (itr!=QUEUE_SUB_INFO.end())
    {
        if (itr->second == flow)
            break;
        ++itr;
    }
    if (itr==QUEUE_SUB_INFO.end())
    {
        // not current subscribed
        solClient_msg_free(&msg);
        return;
    }

    solClient_destination_t flowDest = {SOLCLIENT_NULL_DESTINATION,NULL};
    const char* flowDestName = "";
    if (solClient_flow_getDestination(flow,&flowDest,sizeof(flowDest)) == SOLCLIENT_OK)
        flowDestName = flowDest.dest;

    if (itr->first.compare(flowDestName)!=0)
    {
        // not current subscribed
        solClient_msg_free(&msg);
        return;
    }

    // replyType
    solClient_destination_t msgDest = {SOLCLIENT_NULL_DESTINATION,NULL};
    solClient_destination_t replyto = {SOLCLIENT_NULL_DESTINATION,NULL};
    const char* msgDestName = "";
    if (solClient_msg_getDestination(msg,&msgDest,sizeof(msgDest)) == SOLCLIENT_OK)
        msgDestName = msgDest.dest;
    const char* replyToName = "";
    if (solClient_msg_getReplyTo(msg,&replyto,sizeof(msgDest)) == SOLCLIENT_OK)
        replyToName = replyto.dest;

    const char* correlationid = "";
    solClient_msg_getCorrelationId(msg,&correlationid);
    solClient_msgId_t msgId = 0;
    solClient_msg_getMsgId(msg,&msgId);
    // payload 
    void* dataPtr = NULL;
    solClient_uint32_t dataSize = 0;
    if (solClient_msg_getBinaryAttachmentPtr(msg, &dataPtr, &dataSize) != SOLCLIENT_OK)
    {
        printf("[%ld] Solace issue getting binary attachment from received message (id:%lld, type:%d, subscription:%s, msg destination:%s, msg destination type:%d)\n", THREAD_ID, msgId, flowDest.destType, flowDestName, msgDestName, msgDest.destType);
        solClient_msg_free(&msg);
        return;
    }
    K payload = ktn(KG, dataSize);
    memcpy(payload->G0, dataPtr, dataSize);
    K keys = ktn(KS,6);
    kS(keys)[0]=ss((char*)"destType");
    kS(keys)[1]=ss((char*)"destName");
    kS(keys)[2]=ss((char*)"replyType");
    kS(keys)[3]=ss((char*)"replyName");
    kS(keys)[4]=ss((char*)"correlationId");
    kS(keys)[5]=ss((char*)"msgId");
    K vals = knk(6,ki(msgDest.destType),kp((char*)msgDestName),ki(replyto.destType),kp((char*)replyToName),kp((char*)correlationid),kj(msgId));
    K dict = xD(keys,vals);
    
    K result = k(0, (char*)KDB_QUEUE_MSG_CALLBACK_FUNC.c_str(), ks((char*)flowDestName), payload, dict, (K)0);
    if(-128 == result->t)
        printf("[%ld] Solace calling KDB+ function %s returned with error. Using received data (destination:%s)\n", THREAD_ID, KDB_QUEUE_MSG_CALLBACK_FUNC.c_str(),flowDestName);

    solClient_msg_free(&msg);
}

K kdbCallback(I d)
{
    KdbSolaceEvent msgAndSource;
    int copied = -1;
    // drain the queue of pending solace events/msgs
    while( (copied = recv(d,(char*)&msgAndSource,sizeof(msgAndSource), 0)) != -1)
    {
        if (copied < (int)sizeof(msgAndSource))
        {
            printf("[%ld] Solace problem reading data from pipe, got %d bytes, expecting %ld\n", THREAD_ID, copied, sizeof(msgAndSource));
            return (K)0;
        }
        switch(msgAndSource._type)
        {
            case SESSION_EVENT:
            {
                kdbCallbackSessionEvent(msgAndSource._event._session);
                break;
            }
            case FLOW_EVENT:
            {
                kdbCallbackFlowEvent(msgAndSource._event._flow);
                break;
            }
            case DIRECT_MSG_EVENT:
            {
                kdbCallbackDirectMsgEvent(msgAndSource._event._directMsg);
                break;
            }
            case QUEUE_MSG_EVENT:
            {
                kdbCallbackQueueMsgEvent(&msgAndSource._event._queueMsg);
                break;
            }
        }
    }
    return (K)0;
}

/* The message receive callback function is mandatory for session creation. Gets called with msgs from direct subscriptions. */
solClient_rxMsgCallback_returnCode_t defaultSubCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{
    KdbSolaceEvent msgAndSource;
    msgAndSource._type = DIRECT_MSG_EVENT;
    msgAndSource._event._directMsg=msg_p;
    sendMsg(&msgAndSource);
    // take control of the msg_p memory
    return SOLCLIENT_CALLBACK_TAKE_MSG;
}

solClient_rxMsgCallback_returnCode_t guaranteedSubCallback ( solClient_opaqueFlow_pt opaqueFlow_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{
    KdbSolaceEvent msgAndSource;
    msgAndSource._type = QUEUE_MSG_EVENT;
    msgAndSource._event._queueMsg._msg=msg_p;
    msgAndSource._event._queueMsg._flow=opaqueFlow_p;
    sendMsg(&msgAndSource);
    // take control of the msg_p memory
    return SOLCLIENT_CALLBACK_TAKE_MSG;
}

void flowEventCallback ( solClient_opaqueFlow_pt opaqueFlow_p, solClient_flow_eventCallbackInfo_pt eventInfo_p, void *user_p )
{
    int destinationType = -1;
    const char* destinationName = "";
    solClient_destination_t destination;
    if ( solClient_flow_getDestination ( opaqueFlow_p, &destination, sizeof ( destination ) ) == SOLCLIENT_OK)
    {
        destinationType = destination.destType;
        destinationName = destination.dest; 
    }

    if (!KDB_FLOW_EVENT_CALLBACK_FUNC.empty())
    {
        // send details to kdb+
        KdbSolaceEvent msgAndSource;
        msgAndSource._type = FLOW_EVENT;
        msgAndSource._event._flow = new KdbSolaceEventFlowDetail(); 
        msgAndSource._event._flow->_eventType = eventInfo_p->flowEvent;
        msgAndSource._event._flow->_responseCode = eventInfo_p->responseCode;
        msgAndSource._event._flow->_eventInfo = eventInfo_p->info_p;
        msgAndSource._event._flow->_destType = destinationType;
        msgAndSource._event._flow->_destName = destinationName; 
        sendMsg(&msgAndSource);
    }

    switch ( eventInfo_p->flowEvent ) {
        case SOLCLIENT_FLOW_EVENT_UP_NOTICE:
        case SOLCLIENT_FLOW_EVENT_SESSION_DOWN:
        case SOLCLIENT_FLOW_EVENT_ACTIVE:
        case SOLCLIENT_FLOW_EVENT_INACTIVE:
        {
            /* Non error events are logged at the INFO level. */
            printf ( "[%ld] Solace flowEventCallback() called - %s (destination type: %d name: %s)\n",
                     THREAD_ID,solClient_flow_eventToString ( eventInfo_p->flowEvent ), destinationType, destinationName);
            break;
        }
        case SOLCLIENT_FLOW_EVENT_DOWN_ERROR:
        case SOLCLIENT_FLOW_EVENT_BIND_FAILED_ERROR:
        case SOLCLIENT_FLOW_EVENT_REJECTED_MSG_ERROR:
        {
            /* Extra error information is available on error events */
            solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo (  );
            /* Error events are output to STDOUT. */
            printf ( "[%ld] Solace flowEventCallback() called - %s; subCode %s, responseCode %d, reason %s (destination type: %d name: %s)\n",
                     THREAD_ID,solClient_flow_eventToString ( eventInfo_p->flowEvent ),
                     solClient_subCodeToString ( errorInfo_p->subCode ), errorInfo_p->responseCode, errorInfo_p->errorStr,
                     destinationType, destinationName );
            break;
        }
        default:
        {
            /* Unrecognized or deprecated events are output to STDOUT. */
            printf ( "[%ld] Solace flowEventCallback() called - %s.  Unrecognized or deprecated event (destination type: %d name: %s)\n",
                     THREAD_ID,solClient_flow_eventToString ( eventInfo_p->flowEvent ), destinationType, destinationName );
            break;
        }
    }
}

/*****************************************************************************
 * eventCallback
 *
 * The event callback function is mandatory for session creation.
 *****************************************************************************/
void eventCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_session_eventCallbackInfo_pt eventInfo_p, void *user_p )
{
    if (!KDB_SESSION_EVENT_CALLBACK_FUNC.empty())
    {
        // send details to kdb+
        KdbSolaceEvent msgAndSource;
        msgAndSource._type = SESSION_EVENT;
        msgAndSource._event._session = new KdbSolaceEventSessionDetail;
        msgAndSource._event._session->_eventType = eventInfo_p->sessionEvent;
        msgAndSource._event._session->_responseCode = eventInfo_p->responseCode;
        msgAndSource._event._session->_eventInfo = eventInfo_p->info_p;
        sendMsg(&msgAndSource);
    }
    switch ( eventInfo_p->sessionEvent ) 
    {
        case SOLCLIENT_SESSION_EVENT_ACKNOWLEDGEMENT:
        {
            break;
        }
        case SOLCLIENT_SESSION_EVENT_UP_NOTICE:
        case SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_OK:
        case SOLCLIENT_SESSION_EVENT_CAN_SEND:
        case SOLCLIENT_SESSION_EVENT_RECONNECTING_NOTICE:
        case SOLCLIENT_SESSION_EVENT_RECONNECTED_NOTICE:
        case SOLCLIENT_SESSION_EVENT_PROVISION_OK:
        case SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_OK:
        case SOLCLIENT_SESSION_EVENT_MODIFYPROP_OK:
        case SOLCLIENT_SESSION_EVENT_ASSURED_DELIVERY_DOWN:
        {
            /* Non-error events */
            printf("[%ld] Solace session event %d: %s\n",THREAD_ID,eventInfo_p->sessionEvent, solClient_session_eventToString(eventInfo_p->sessionEvent));
            break;
        }
        case SOLCLIENT_SESSION_EVENT_DOWN_ERROR:
        case SOLCLIENT_SESSION_EVENT_CONNECT_FAILED_ERROR:
        case SOLCLIENT_SESSION_EVENT_REJECTED_MSG_ERROR:
        case SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_ERROR:
        case SOLCLIENT_SESSION_EVENT_RX_MSG_TOO_BIG_ERROR:
        case SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_ERROR:
        case SOLCLIENT_SESSION_EVENT_PROVISION_ERROR:
        {
            /* Extra error information is available on error events */
            solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo (  );
            printf("[%ld] Solace session error event %d: %s, subcode %s, responsecode %d reason %s \n",THREAD_ID,eventInfo_p->sessionEvent,
                                                                            solClient_session_eventToString(eventInfo_p->sessionEvent), 
                                                                            solClient_subCodeToString(errorInfo_p->subCode),errorInfo_p->responseCode,errorInfo_p->errorStr);
            break;
        }
        default:
        {
            /* Unrecognized or deprecated events */
            printf("[%ld] Solace unrecognized/deprecated session event %d: %s\n",THREAD_ID,eventInfo_p->sessionEvent, solClient_session_eventToString(eventInfo_p->sessionEvent));
            break;
        }
    } 
}

K init_solace(K options)
{
    CHECK_PARAM_DICT_SYMS(options,"init_solace");
    int ret = dumb_socketpair(SPAIR,1);
    if (ret != 0)
        return krr((S)"Solace init couldn't create pipe");
#ifdef WIN32
    u_long iMode = 1;
    if (ioctlsocket(SPAIR[0], FIONBIO, &iMode) != NO_ERROR)
        return krr((S)"Solace init couldn't set socket to non-blocking");
    if (ioctlsocket(SPAIR[1], FIONBIO, &iMode) != NO_ERROR)
        return krr((S)"Solace init couldn't set socket to non-blocking");
#else
    if (fcntl(SPAIR[0], F_SETFL, O_NONBLOCK) == -1)
        return krr((S)"Solace init couldn't set socket to non-blocking");
    if (fcntl(SPAIR[1], F_SETFL, O_NONBLOCK) == -1)
        return krr((S)"Solace init couldn't set socket to non-blocking");
#endif
    sd1(SPAIR[0],kdbCallback);

    solClient_opaqueContext_pt context_p;
    solClient_context_createFuncInfo_t contextFuncInfo = SOLCLIENT_CONTEXT_CREATEFUNC_INITIALIZER;
    solClient_session_createFuncInfo_t sessionFuncInfo = SOLCLIENT_SESSION_CREATEFUNC_INITIALIZER;
 
    /* solClient needs to be initialized before any other API calls. */
    solClient_returnCode_t retCode = solClient_initialize ( SOLCLIENT_LOG_DEFAULT_FILTER, NULL );
    if (retCode != SOLCLIENT_OK)
    {
        printf("[%ld] Solace initialize error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        return ki(retCode);
    }

    /*
     * Create a Context, and specify that the Context thread be created
     * automatically instead of having the application create its own
     * Context thread.
     */
    retCode = solClient_context_create ( SOLCLIENT_CONTEXT_PROPS_DEFAULT_WITH_CREATE_THREAD,
                               &context_p, &contextFuncInfo, sizeof ( contextFuncInfo ) );
    if (retCode != SOLCLIENT_OK)
    {
        printf("[%ld] Solace context create error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        return ki(retCode);
    }

    context = context_p;

    /*
     * Message receive callback function and the Session event function
     * are both mandatory. In this sample, default functions are used.
     */
    sessionFuncInfo.rxMsgInfo.callback_p = defaultSubCallback;
    sessionFuncInfo.rxMsgInfo.user_p = NULL;
    sessionFuncInfo.eventInfo.callback_p = eventCallback;
    sessionFuncInfo.eventInfo.user_p = NULL;

    const char** sessionProps = createProperties(options);
    retCode = solClient_session_create ( ( char ** ) sessionProps,
                               context_p,
                               &session_p, &sessionFuncInfo, sizeof ( sessionFuncInfo ) );
    free (sessionProps);
    if (retCode != SOLCLIENT_OK)
    {
        printf("[%ld] Solace session create error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        solClient_cleanup();
        return ki(retCode);
    }

    if (solClient_session_connect ( session_p ) != SOLCLIENT_OK)
    {
        printf("[%ld] Solace session connect error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        solClient_session_destroy (&session_p);
        solClient_cleanup();
        return ki(retCode);
    }
    return ki(SOLCLIENT_OK); 
}

K destroy_solace(K a)
{
    solClient_returnCode_t retCode = SOLCLIENT_OK;
    if (session_p != NULL)
    {
        retCode = solClient_session_destroy (&session_p);
        if (retCode != SOLCLIENT_OK)
        {
            // can have a return code of SOLCLIENT_FAIL, with subcode of SOLCLIENT_SUBCODE_TIMEOUT
            printf("[%ld] Solace session destroy error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
            return ki(retCode);
        }
    }
    retCode = solClient_cleanup ();
    if (retCode != SOLCLIENT_OK)
    {
        // can have a return code of SOLCLIENT_FAIL, with various subcodes
        printf("[%ld] Solace destroy error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        return ki(retCode);
    }
    session_p = NULL;
    return ki(SOLCLIENT_OK); 
}

K version_solace(K unused)
{
    solClient_version_info_pt version = NULL;
    solClient_version_get (&version);
    K keys = knk(3,ks((char*)"solVersion"),ks((char*)"solDate"),ks((char*)"solVariant"));
    K vals = knk(3,ks((char*)version->version_p),ks((char*)version->dateTime_p),ks((char*)version->variant_p));
    K dict = xD(keys,vals);
    return dict;
}

K setsessioncallback_solace(K callbackFunction)
{
    CHECK_PARAM_TYPE(callbackFunction,-KS,"setsessioncallback_solace");
    KDB_SESSION_EVENT_CALLBACK_FUNC.assign(callbackFunction->s);
    return ki(SOLCLIENT_OK);
}

K setflowcallback_solace(K callbackFunction)
{
    CHECK_PARAM_TYPE(callbackFunction,-KS,"setflowcallback_solace");
    KDB_FLOW_EVENT_CALLBACK_FUNC.assign(callbackFunction->s);
    return ki(SOLCLIENT_OK);
}

K iscapable_solace(K capabilityName)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_STRING_TYPE(capabilityName,"iscapable_solace");
    char* capStr = createString(capabilityName);
    bool isCap = solClient_session_isCapable(session_p,capStr);
    free(capStr);
    return kb(isCap);
}

K getcapability_solace(K capabilityName)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_STRING_TYPE(capabilityName,"getcapability_solace");
    solClient_field_t field;

    char* capStr = createString(capabilityName);
    solClient_session_getCapability (session_p, capStr, &field, sizeof(field));
    free(capStr);

    switch (field.type)
    {
    case SOLCLIENT_BOOL:
        return kb(field.value.boolean);
    case SOLCLIENT_UINT8:
        return ki(field.value.uint8);
    case SOLCLIENT_INT8:
        return kh(field.value.int8);
    case SOLCLIENT_UINT16:
        return ki(field.value.uint16);
    case SOLCLIENT_INT16:
        return ki(field.value.int16);
    case SOLCLIENT_UINT32:
        return kj(field.value.uint32);
    case SOLCLIENT_INT32:
        return ki(field.value.int32);
    case SOLCLIENT_UINT64:
        return kj(field.value.uint64);
    case SOLCLIENT_INT64:
        return kj(field.value.int64);
    case SOLCLIENT_WCHAR:
        return kc(field.value.wchar);
    case SOLCLIENT_STRING:
        return ks((char*)field.value.string);
    case SOLCLIENT_FLOAT:
        return kf(field.value.float32);
    case SOLCLIENT_DOUBLE:
        return kf(field.value.float64);
    case SOLCLIENT_BYTEARRAY:
    case SOLCLIENT_MAP:
    case SOLCLIENT_STREAM:
    case SOLCLIENT_NULL:
    case SOLCLIENT_DESTINATION:
    case SOLCLIENT_SMF:
    case SOLCLIENT_UNKNOWN:
    {
        return krr((S)"Solace getcapability - returned type not yet supported"); 
    }
    }
    return krr((S)"Solace getcapability - returned type not yet supported");
}

K createendpoint_solace(K options, K provFlags)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(provFlags,-KI,"createendpoint_solace");
    CHECK_PARAM_DICT_SYMS(options,"createendpoint_solace");
    if (!solClient_session_isCapable(session_p,SOLCLIENT_SESSION_CAPABILITY_ENDPOINT_MANAGEMENT))
        return krr((S)"Solace createendpoint - server does not support endpoint management");

    const char** provProps = createProperties(options);
    solClient_returnCode_t retCode = solClient_session_endpointProvision((char**)provProps,
                                            session_p,
                                            provFlags->i, 
                                            NULL, // correlation tag
                                            NULL, // network name (depreciated)
                                            0);
    free(provProps);
    if (retCode != SOLCLIENT_OK)
    {
        solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo();
        printf("[%ld] Solace createendpoint - solClient_session_endpointProvision() failed subCode (%d:'%s')\n",THREAD_ID,errorInfo_p->subCode,solClient_subCodeToString(errorInfo_p->subCode)); 
        return ki(errorInfo_p->subCode);
    }                  
    return ki(SOLCLIENT_OK);
}

K destroyendpoint_solace(K options, K provFlags)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(provFlags,-KI,"destroyendpoint_solace");
    CHECK_PARAM_DICT_SYMS(options,"destroyendpoint_solace");
    if (!solClient_session_isCapable(session_p,SOLCLIENT_SESSION_CAPABILITY_ENDPOINT_MANAGEMENT))
        return krr((S)"Solace destroyendpoint - server does not support endpoint management");
    
    const char** provProps = createProperties(options);
    solClient_returnCode_t retCode = solClient_session_endpointDeprovision((char**)provProps,
                                                session_p,
                                                provFlags->i,
                                                NULL);
    free(provProps);
    if (retCode != SOLCLIENT_OK) 
    {
        solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo();
        printf("[%ld] Solace destroyendpoint - solClient_session_endpointDeprovision() failed subCode (%d:'%s')\n",THREAD_ID,errorInfo_p->subCode,solClient_subCodeToString(errorInfo_p->subCode));
        return ki(errorInfo_p->subCode);
    }
    return ki(SOLCLIENT_OK);
}

K endpointtopicsubscribe_solace(K options, K provFlags, K topic)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(provFlags,-KI,"endpointtopicsubscribe_solace");
    CHECK_PARAM_DICT_SYMS(options,"endpointtopicsubscribe_solace");
    CHECK_PARAM_STRING_TYPE(topic,"endpointtopicsubscribe_solace");
    if (!solClient_session_isCapable(session_p,SOLCLIENT_SESSION_CAPABILITY_ENDPOINT_MANAGEMENT))
        return krr((S)"Solace endpointtopicsubscribe - server does not support endpoint management");

    char* topicStr = createString(topic);
    const char** provProps = createProperties(options);
    solClient_returnCode_t retCode = solClient_session_endpointTopicSubscribe((char**)provProps,
                                            session_p,
                                            provFlags->i, 
                                            topicStr,
                                            NULL); // correlation tag
    free(topicStr);
    free(provProps);
    if (retCode != SOLCLIENT_OK)
    {
        solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo();
        printf("[%ld] Solace createendpoint - solClient_session_endpointTopicSubscribe() failed subCode (%d:'%s')\n",THREAD_ID,errorInfo_p->subCode,solClient_subCodeToString(errorInfo_p->subCode)); 
        return ki(errorInfo_p->subCode);
    }                  
    return ki(SOLCLIENT_OK);
}

K endpointtopicunsubscribe_solace(K options, K provFlags, K topic)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(provFlags,-KI,"endpointtopicunsubscribe_solace");
    CHECK_PARAM_DICT_SYMS(options,"endpointtopicunsubscribe_solace");
    CHECK_PARAM_STRING_TYPE(topic,"endpointtopicunsubscribe_solace");
    if (!solClient_session_isCapable(session_p,SOLCLIENT_SESSION_CAPABILITY_ENDPOINT_MANAGEMENT))
        return krr((S)"Solace endpointtopicunsubscribe - server does not support endpoint management");

    char* topicStr = createString(topic);
    const char** provProps = createProperties(options);
    solClient_returnCode_t retCode = solClient_session_endpointTopicUnsubscribe((char**)provProps,
                                            session_p,
                                            provFlags->i, 
                                            topicStr,
                                            NULL); // correlation tag
    free(topicStr);
    free(provProps);
    if (retCode != SOLCLIENT_OK)
    {
        solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo();
        printf("[%ld] Solace createendpoint - solClient_session_endpointTopicUnsubscribe() failed subCode (%d:'%s')\n",THREAD_ID,errorInfo_p->subCode,solClient_subCodeToString(errorInfo_p->subCode)); 
        return ki(errorInfo_p->subCode);
    }                  
    return ki(SOLCLIENT_OK);
}

static solClient_opaqueMsg_pt createDirectMsg(K topic,K data)
{
    solClient_destination_t destination = {SOLCLIENT_TOPIC_DESTINATION, topic->s};
    solClient_opaqueMsg_pt msg_p = NULL;
    solClient_msg_alloc ( &msg_p );
    solClient_msg_setDeliveryMode ( msg_p, SOLCLIENT_DELIVERY_MODE_DIRECT );
    solClient_msg_setDestination ( msg_p, &destination, sizeof ( destination ) );
    solClient_msg_setElidingEligible( msg_p, 1); // set to true
    solClient_msg_setDMQEligible( msg_p, 1); // set to true
    solClient_msg_setBinaryAttachment ( msg_p, getData(data), getDataSize(data));
    return msg_p;
}

K senddirect_solace(K topic, K data)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"senddirect_solace");
    CHECK_PARAM_DATA_TYPE(data,"senddirect_solace");
    solClient_opaqueMsg_pt msg_p = createDirectMsg(topic,data);
    solClient_returnCode_t retCode = solClient_session_sendMsg ( session_p, msg_p ); 
    solClient_msg_free ( &msg_p );
    return ki(retCode);
}

 K senddirectrequest_solace(K topic, K data, K timeout, K replyType, K replydest)
 {
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"senddirectrequest_solace");
    CHECK_PARAM_DATA_TYPE(data,"senddirectrequest_solace");
    CHECK_PARAM_TYPE(timeout,-KI,"senddirectrequest_solace");
    if (timeout->i <= 0)
        krr((char*)"senddirectrequest_solace must be provided with timeout greater than zero");
    solClient_opaqueMsg_pt msg_p = createDirectMsg(topic,data);
    setReplyTo(replyType, replydest, msg_p);
    solClient_opaqueMsg_pt replyMsg = NULL;
    solClient_returnCode_t retCode = solClient_session_sendRequest ( session_p, msg_p, &replyMsg, timeout->i); 
    solClient_msg_free ( &msg_p );
    if (retCode == SOLCLIENT_OK)
    { 
        void* dataPtr = NULL;
        solClient_uint32_t dataSize = 0;
        solClient_msg_getBinaryAttachmentPtr(replyMsg, &dataPtr, &dataSize);
        K payload = ktn(KG, dataSize);
        memcpy(payload->G0, dataPtr, dataSize);
        solClient_msg_free(&replyMsg);
        return payload;
    }
    return ki(retCode);
 }

K callbacktopic_solace(K func)
{
    CHECK_PARAM_STRING_TYPE(func,"callbacktopic_solace");
    char* cbStr = createString(func);
    KDB_DIRECT_MSG_CALLBACK_FUNC.assign(cbStr);
    free(cbStr);
    return ki(SOLCLIENT_OK);
}

K subscribetopic_solace(K topic, K isBlocking)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"subscribetopic_solace");
    CHECK_PARAM_TYPE(isBlocking,-KB,"subscribetopic_solace");
    solClient_returnCode_t ret = solClient_session_topicSubscribeExt ( session_p,
                                          ((isBlocking->g==1)?SOLCLIENT_SUBSCRIBE_FLAGS_WAITFORCONFIRM:SOLCLIENT_SUBSCRIBE_FLAGS_REQUEST_CONFIRM),
                                          topic->s );
    return ki(ret);
}

K unsubscribetopic_solace(K topic)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"subscribetopic_solace");
    solClient_returnCode_t ret = solClient_session_topicUnsubscribeExt (session_p,
                                        SOLCLIENT_SUBSCRIBE_FLAGS_REQUEST_CONFIRM,
                                        topic->s );
    return ki(ret);
}

static solClient_opaqueMsg_pt createPersistentMsg(K destType, K destName,K data)
{
    solClient_destination_t destination = {(solClient_destinationType_t)destType->i, destName->s};
    solClient_opaqueMsg_pt msg_p = NULL;
    solClient_msg_alloc ( &msg_p );
    solClient_msg_setDeliveryMode ( msg_p, SOLCLIENT_DELIVERY_MODE_PERSISTENT );
    solClient_msg_setDestination ( msg_p, &destination, sizeof ( destination ) );
    solClient_msg_setElidingEligible( msg_p, 1); // set to true
    solClient_msg_setDMQEligible( msg_p, 1); // set to true
    solClient_msg_setBinaryAttachment ( msg_p, getData(data), getDataSize(data));
    return msg_p;
}

K sendpersistent_solace(K destType, K dest, K data, K correlationId)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(destType,-KI,"sendpersistent_solace");
    CHECK_PARAM_TYPE(dest,-KS,"sendpersistent_solace");
    CHECK_PARAM_DATA_TYPE(data,"sendpersistent_solace");

    solClient_opaqueMsg_pt msg_p = createPersistentMsg(destType,dest,data);
    if (correlationId != NULL)
    {
        char* correlationIdStr = createString(correlationId);
        solClient_msg_setCorrelationId (msg_p, correlationIdStr);
        free(correlationIdStr);
    }
    solClient_returnCode_t retCode = solClient_session_sendMsg ( session_p, msg_p ); 
    solClient_msg_free ( &msg_p );

    if (retCode != SOLCLIENT_OK)
    {
        printf("[%ld] Solace sendpersistent error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        return ki(retCode);
    }
    return ki(SOLCLIENT_OK);
}

K sendpersistentrequest_solace(K destType, K dest, K data, K timeout, K replyType, K replydest)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(destType,-KI,"sendpersistentrequest_solace");
    CHECK_PARAM_TYPE(dest,-KS,"sendpersistentrequest_solace");
    CHECK_PARAM_DATA_TYPE(data,"sendpersistentrequest_solace");
    CHECK_PARAM_TYPE(timeout,-KI,"sendpersistentrequest_solace");
    if (timeout->i <= 0)
        krr((char*)"sendpersistentrequest_solace must be provided with timeout greater than zero");
    solClient_opaqueMsg_pt msg_p = createPersistentMsg(destType,dest,data);
    setReplyTo(replyType, replydest, msg_p);
    solClient_opaqueMsg_pt replyMsg = NULL;
    solClient_returnCode_t retCode = solClient_session_sendRequest ( session_p, msg_p, &replyMsg, timeout->i); 
    solClient_msg_free ( &msg_p );
    if (retCode == SOLCLIENT_OK)
    { 
        void* dataPtr = NULL;
        solClient_uint32_t dataSize = 0;
        solClient_msg_getBinaryAttachmentPtr(replyMsg, &dataPtr, &dataSize);
        K payload = ktn(KG, dataSize);
        memcpy(payload->G0, dataPtr, dataSize);
        solClient_msg_free(&replyMsg);
        return payload;
    }
    return ki(retCode);
}

K callbackqueue_solace(K func)
{
    CHECK_PARAM_STRING_TYPE(func,"callbackqueue_solace");
    char* cbStr = createString(func);
    KDB_QUEUE_MSG_CALLBACK_FUNC.assign(cbStr);
    free(cbStr);
    return ki(SOLCLIENT_OK);
}

K bindqueue_solace(K bindProps)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_DICT_SYMS(bindProps,"bindqueue_solace");

    // create callbacks for flow
    solClient_flow_createFuncInfo_t flowFuncInfo = SOLCLIENT_SESSION_CREATEFUNC_INITIALIZER;
    flowFuncInfo.rxMsgInfo.callback_p = guaranteedSubCallback;
    flowFuncInfo.rxMsgInfo.user_p = NULL;
    flowFuncInfo.eventInfo.callback_p = flowEventCallback;
    flowFuncInfo.eventInfo.user_p = NULL;

    const char** flowProps = createProperties(bindProps);
    solClient_opaqueFlow_pt flow_p; 
    solClient_returnCode_t rc = solClient_session_createFlow((char**)flowProps,session_p,&flow_p,&flowFuncInfo,sizeof(flowFuncInfo));    
    free(flowProps);
    if (rc != SOLCLIENT_OK)
    {
        printf("[%ld] Solace subscribepersistent createFlow error %d: %s\n",THREAD_ID,rc,solClient_returnCodeToString(rc));
        return ki(rc); 
    }

    solClient_destination_t queueDest;
    if ( ( rc = solClient_flow_getDestination ( flow_p, &queueDest, sizeof ( queueDest ) ) ) != SOLCLIENT_OK )
    {
        printf("[%ld] Solace bindqueue_solace getdestination error %d: %s\n",THREAD_ID,rc,solClient_returnCodeToString(rc));
        return ks((char*)"");
    }

    QUEUE_SUB_INFO.insert(std::pair<std::string,solClient_opaqueFlow_pt>(queueDest.dest,flow_p));
    return ki(SOLCLIENT_OK);
}

K sendack_solace(K endpointname, K msgid)
{
    CHECK_PARAM_TYPE(msgid,-KJ,"sendack_solace");
    CHECK_PARAM_STRING_TYPE(endpointname,"sendack_solace");
    char* endpointStr = createString(endpointname);
    std::map<std::string, solClient_opaqueFlow_pt>::iterator flowIt = QUEUE_SUB_INFO.find(endpointStr);
    free(endpointStr);
    if (flowIt == QUEUE_SUB_INFO.end())
        return ki(SOLCLIENT_FAIL);
    solClient_opaqueFlow_pt solFlow = (solClient_opaqueFlow_pt)flowIt->second;
    solClient_msgId_t solMsgId = msgid->j;
    solClient_returnCode_t retCode = solClient_flow_sendAck (solFlow, solMsgId); 
    return ki(retCode);
}

K unbindqueue_solace(K endpointname)
{
    CHECK_PARAM_TYPE(endpointname,-KS,"unsubscribepersistent_solace");
    
    const char* subname =  endpointname->s;
    std::map<std::string,solClient_opaqueFlow_pt>::iterator it;
    it = QUEUE_SUB_INFO.find(subname);
    if (it == QUEUE_SUB_INFO.end())
    {
        printf("[%ld] Solace unsubscribe for subscription %s that doesnt exist\n", THREAD_ID, subname);
        return krr((S)"Solace unsubscribe for subscription that doesnt exist");
    }
    solClient_opaqueFlow_pt flow_pt = it->second;
    solClient_returnCode_t retCode = solClient_flow_destroy(&flow_pt);
    if (retCode != SOLCLIENT_OK)
        printf("[%ld] Solace unsubscribe failure for %s\n", THREAD_ID, subname);
    else
        printf("[%ld] Solace subscription removed for %s\n", THREAD_ID, subname);

    QUEUE_SUB_INFO.erase(it);

    return ki(retCode);
}
