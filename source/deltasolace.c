#include "deltasolace.h"
#include "solclient/solClient.h"
#include "solclient/solClientMsg.h"
#include "KdbSolaceEvent.h"
#include <string.h>
#include <string>
#include <map>
#include "log/ThreadID.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>

/**
 * UTILITY FUNCTIONS/MACROS
 **/
#define CHECK_SESSION_CREATED {if (session_p == NULL) return krr((S)"Solace function called without initialising solace");}
#define CHECK_PARAM_TYPE(x,y,z) {if (x->t != y) return krr((S)"Function " #z " called with incorrect param type for " #x "");}
#define CHECK_PARAM_STRING_TYPE(x,z) {if (x->t != -KS && x->t != KC) return krr((S)"Function " #z " called with incorrect param type for " #x "");}
#define CHECK_PARAM_DATA_TYPE(x,z) {if (x->t != KG && x->t != -KS && x->t != KC) return krr((S)"Function " #z " called with incorrect param type for " #x "");}
#define CHECK_PARAM_DICT_SYMS(x,z) {if (x->t != 99) return krr((S)"Function " #z " called with incorrect dict param type"); if ((kK(x)[0])->n>0 && (kK(x)[0])->t != 11 && (kK(x)[1])->t != 11) return krr((S)"Function " #z " expecting symbols in provided dict");}

solClient_int64_t getMillisSinceEpoch()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ( (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000 );
}

int getDataSize(K d)
{
    return (d->t == -KS) ? strlen(d->s) : d->n;
}

void* getData(K d)
{
    return (d->t == -KS) ? d->s : (void*)d->G0;
}

int getStringSize(K str)
{
    if (str->t == KC)
        return (str->n) + 1;
    else if (str->t == -KS)
        return strlen(str->s)+1;
    return 1;
}

void setString(char* str,K in,int len)
{
    if (in->t == KC)
        memcpy(str,in->G0,len-1);
    else if (in->t == -KS)
        memcpy(str,in->s,len-1);
    str[len-1]= '\0';
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

static int CALLBACK_PIPE[2];
static std::string KDB_SESSION_EVENT_CALLBACK_FUNC;
static std::string KDB_FLOW_EVENT_CALLBACK_FUNC;

enum KdbSolaceEndpointType
{
    KDB_SOLACE_ENDPOINT_TYPE_TOPIC      = 0,
    KDB_SOLACE_ENDPOINT_TYPE_QUEUE      = 1,
    KDB_SOLACE_ENDPOINT_TYPE_TMP_TOPIC  = 2,
    KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE  = 3
};

struct FlowSubscriptionInfo
{
    solClient_opaqueFlow_pt flow_pt;
    std::string             flowKdbCbFunc;
};
static std::map<std::string, FlowSubscriptionInfo> FLOW_INFO;

struct TimerPayload 
{
    std::string                     dest;
    solClient_context_timerId_t     destTimer;
};

typedef std::map<std::string, K> batchInfoMap;
static batchInfoMap BATCH_PER_DESTINATION;

static void timerCbFunc(solClient_opaqueContext_pt opaqueContext_p, void* user_p)
{
    TimerPayload* payload = (static_cast<TimerPayload*>(user_p));
    // lookup batch info and retrieve batch for this destination
    batchInfoMap::iterator find = BATCH_PER_DESTINATION.find(payload->dest);
    if ((find != BATCH_PER_DESTINATION.end()) && (NULL != find->second))
    {
        // attempt to send the batch over the pipe
        KdbSolaceEvent msgAndSource;
        msgAndSource._type = MSG_EVENT;
        msgAndSource._event._subMsg = new KdbSolaceEventSubMsg;
        msgAndSource._event._subMsg->_destName.assign(payload->dest);
        msgAndSource._event._subMsg->_vals = (find->second);
        int numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
        if (numWritten != sizeof(msgAndSource))
        {
            msgAndSource._event._subMsg->_vals = NULL;
            delete (msgAndSource._event._subMsg);
            // allow timer to repeat, try again later
            return;
        }
        // reset batch
        find->second = NULL;
    }
    solClient_returnCode_t rc;
    if ((rc = solClient_context_stopTimer(context, &payload->destTimer)) != SOLCLIENT_OK)
        printf("[%ld] Solace problem couldn't stop batch timer for destination %s.\n", THREAD_ID, payload->dest.c_str());
    delete (payload);
}

int setBatchData(K* batch, int subsType, const char* topicName, int replyType, const char* replyName, const char* correlationId, solClient_opaqueFlow_pt flowPtr, solClient_msgId_t msgId, void* data, solClient_uint32_t dataSize)
{
    // if not created yet, create the table
    if (NULL == (*batch))
    {
       (*batch) = knk(0);
       jk(&(*batch), ktn(KI, 0)); //[0]
       jk(&(*batch), knk(0));     //[1]
       jk(&(*batch), ktn(KI, 0)); //[2]
       jk(&(*batch), knk(0));     //[3]
       jk(&(*batch), knk(0));     //[4]
       jk(&(*batch), ktn(KJ, 0)); //[5]
       jk(&(*batch), ktn(KJ, 0)); //[6]
       jk(&(*batch), knk(0));     //[7]
    }

   ja(&(((K*)(*batch)->G0)[0]), &subsType);
   jk(&(((K*)(*batch)->G0)[1]), kp((char*)topicName));
   ja(&(((K*)(*batch)->G0)[2]), &replyType);
   jk(&(((K*)(*batch)->G0)[3]), kp((char*)replyName));
   jk(&(((K*)(*batch)->G0)[4]), kp((char*)correlationId));
   ja(&(((K*)(*batch)->G0)[5]), &flowPtr);
   ja(&(((K*)(*batch)->G0)[6]), &msgId);
   jk(&(((K*)(*batch)->G0)[7]), kpn((char*)data,dataSize));

   return (((K*)(*batch)->G0)[0])->n;
}

void setReplyTo(K replyType, K replydest, solClient_opaqueMsg_pt msg_p)
{
    if ((replydest != NULL) && (replydest->t == -KS) && (replyType->t == -KI) && (strlen(replydest->s) > 0))
    {
        solClient_destination_t replydestination;
        if ((replyType->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (replyType->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
            replydestination.destType = SOLCLIENT_QUEUE_DESTINATION;
        else
            replydestination.destType = SOLCLIENT_TOPIC_DESTINATION;
        replydestination.dest = replydest->s;
        solClient_msg_setReplyTo ( msg_p, &replydestination, sizeof ( replydestination ) );
    }
}

K kdbCallback(I d);

K kdbCallback(I d)
{
    KdbSolaceEvent msgAndSource;
    ssize_t copied = -1;
    // drain the queue of pending solace events/msgs
    while( (copied = read(d,&msgAndSource,sizeof(msgAndSource))) != -1)
    {
        if (copied < (ssize_t)sizeof(msgAndSource))
        {
            printf("[%ld] Solace problem reading data from pipe, got %ld bytes, expecting %ld\n", THREAD_ID, copied, sizeof(msgAndSource));
            return (K)0;
        }
        if (msgAndSource._type == SESSION_EVENT)
        {
            if (!KDB_SESSION_EVENT_CALLBACK_FUNC.empty())
            {
                const char* kdbFunc = KDB_SESSION_EVENT_CALLBACK_FUNC.c_str();
                K eventtype = ki(msgAndSource._event._session->_eventType);
                K responsecode = ki(msgAndSource._event._session->_responseCode);
                K eventinfo = kp((char*)msgAndSource._event._session->_eventInfo.c_str()); 
                K result = k(0, (char*)kdbFunc, eventtype, responsecode, eventinfo, (K)0);
                if(-128 == result->t)
                {
                    printf("[%ld] Solace not able to call kdb function %s with received data (eventtype:%d responsecode:%d eventinfo:%s)\n", 
                                    THREAD_ID, 
                                    kdbFunc,
                                    msgAndSource._event._session->_eventType,
                                    msgAndSource._event._session->_responseCode,
                                    msgAndSource._event._session->_eventInfo.c_str());
                }
            }
            delete (msgAndSource._event._session);
            return (K)0;
        }
        else if (msgAndSource._type == FLOW_EVENT)
        {
            if (!KDB_FLOW_EVENT_CALLBACK_FUNC.empty())
            {
                const char* kdbFunc = KDB_FLOW_EVENT_CALLBACK_FUNC.c_str();
                K eventtype = ki(msgAndSource._event._flow->_eventType);
                K responsecode = ki(msgAndSource._event._flow->_responseCode);
                K eventinfo = kp((char*)msgAndSource._event._flow->_eventInfo.c_str());
                K destType = ki(msgAndSource._event._flow->_destType);
                K destName = kp((char*)msgAndSource._event._flow->_destName.c_str());
                K result = k(0, (char*)kdbFunc, eventtype, responsecode, eventinfo, destType, destName, (K)0);
                if(-128 == result->t)
                {
                    printf("[%ld] Solace not able to call kdb function %s\n", THREAD_ID, kdbFunc); 
                }
            }
            delete (msgAndSource._event._flow);
            return (K)0;
        }
        else if (msgAndSource._type != MSG_EVENT)
            return (K)0;
        
        std::map<std::string,FlowSubscriptionInfo>::const_iterator it = FLOW_INFO.find(msgAndSource._event._subMsg->_destName);
        if (it == FLOW_INFO.end())
        {
            printf ( "[%ld] Solace received callback message on flow with no callback function subscription:%s)\n", THREAD_ID, msgAndSource._event._subMsg->_destName.c_str());
            // clean up the sub item
            delete (msgAndSource._event._subMsg);
            return (K)0;
        }
        
        K rows = ktn(KS,0);
        js(&rows,ss((char*)"subsType"));
        js(&rows,ss((char*)"topicName"));
        js(&rows,ss((char*)"replyType"));
        js(&rows,ss((char*)"replyName"));
        js(&rows,ss((char*)"correlationId"));
        js(&rows,ss((char*)"flowPtr"));
        js(&rows,ss((char*)"msgId"));
        js(&rows,ss((char*)"stringData"));

        K dict = xD(rows, msgAndSource._event._subMsg->_vals);
        K result = k(0, (char*)it->second.flowKdbCbFunc.c_str(), dict, (K)0);
        if(-128 == result->t)
            printf("[%ld] Solace not able to call kdb function %s with received data (destination:%s)\n", THREAD_ID, it->second.flowKdbCbFunc.c_str(), msgAndSource._event._subMsg->_destName.c_str());
        
        // clean up the sub item
        delete (msgAndSource._event._subMsg);
    }
    return (K)0;
}

K sendack_solace(K flow, K msgid)
{
    CHECK_PARAM_TYPE(flow,-KJ,"sendack_solace");
    CHECK_PARAM_TYPE(msgid,-KJ,"sendack_solace");
    solClient_opaqueFlow_pt solFlow = (solClient_opaqueFlow_pt)flow->j;
    solClient_msgId_t solMsgId = msgid->j;
    solClient_returnCode_t retCode = solClient_flow_sendAck (solFlow, solMsgId); 
    return ki(retCode);
}

/* The message receive callback function is mandatory for session creation. */
solClient_rxMsgCallback_returnCode_t defaultReceiveCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{
    return SOLCLIENT_CALLBACK_OK;
}

solClient_rxMsgCallback_returnCode_t flowMsgCallbackFunc ( solClient_opaqueFlow_pt opaqueFlow_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{
    //solClient_msg_dump(msg_p,NULL,0);
    solClient_destination_t destination;
    if (solClient_flow_getDestination(opaqueFlow_p,&destination,sizeof(destination)) != SOLCLIENT_OK)
    {
        printf("[%ld] Solace cant get destination of flow\n",THREAD_ID);
        return SOLCLIENT_CALLBACK_OK;
    }

    KdbSolaceEvent msgAndSource;
    msgAndSource._type = MSG_EVENT;
    msgAndSource._event._subMsg = new KdbSolaceEventSubMsg;
    msgAndSource._event._subMsg->_destName.assign(destination.dest);
    std::string tmpDest;
    tmpDest.assign(destination.dest);
    // replyType
    solClient_destination_t msgDest;
    solClient_destination_t replyto;
    msgDest.destType = SOLCLIENT_NULL_DESTINATION;
    msgDest.dest = NULL;
    replyto.destType = SOLCLIENT_NULL_DESTINATION;
    replyto.dest = NULL;
    const char* msgDestName = "";
    if (solClient_msg_getDestination(msg_p,&msgDest,sizeof(msgDest)) == SOLCLIENT_OK)
        msgDestName = msgDest.dest;
    const char* replyToName = "";
    if (solClient_msg_getReplyTo(msg_p,&replyto,sizeof(msgDest)) == SOLCLIENT_OK)
        replyToName = replyto.dest;

    // correlationid
    const char* correlationid = "";
    solClient_msg_getCorrelationId(msg_p,&correlationid);
    // msgId
    solClient_msgId_t msgId = 0;
    solClient_msg_getMsgId(msg_p,&msgId);
    // payload 
    void* dataPtr = NULL;
    solClient_uint32_t dataSize = 0;
    if (solClient_msg_getBinaryAttachmentPtr(msg_p, &dataPtr, &dataSize) != SOLCLIENT_OK)
    {
        printf("[%ld] Solace issue getting binary attachment from received message (id:%lld, type:%d, subscription:%s, msg destination:%s, msg destination type:%d)\n", THREAD_ID, msgId, destination.destType, tmpDest.c_str(), msgDestName, msgDest.destType);
        return SOLCLIENT_CALLBACK_OK;
    }
    // lookup batch info and check if its batch is NULL 
    batchInfoMap::iterator it = BATCH_PER_DESTINATION.find(tmpDest);
    if (it == BATCH_PER_DESTINATION.end())
    {
        // first update on this destination - insert entry to map
        K tmp = NULL;
        BATCH_PER_DESTINATION.insert(std::make_pair(tmpDest, tmp));
        it = BATCH_PER_DESTINATION.find(tmpDest);
    }

    if (NULL == it->second)
    {
        // not baching - yet
        K vals = knk(0);
        jk(&(vals), ktn(KI, 0));
        ja(&(((K*)vals->G0)[0]), &destination.destType);
        jk(&(vals), knk(0));
        jk(&(((K*)vals->G0)[1]), kp((char*)tmpDest.c_str()));
        jk(&(vals), ktn(KI,0));
        ja(&(((K*)vals->G0)[2]), &replyto.destType);
        jk(&(vals), knk(0));
        jk(&(((K*)vals->G0)[3]), kp((char*)replyToName));
        jk(&(vals), knk(0));
        jk(&(((K*)vals->G0)[4]), kp((char*)correlationid));
        jk(&(vals), ktn(KJ,0));
        ja(&(((K*)vals->G0)[5]), &opaqueFlow_p);
        jk(&(vals), ktn(KJ,0));
        ja(&(((K*)vals->G0)[6]), &msgId);
        jk(&(vals), knk(0));
        jk(&(((K*)vals->G0)[7]), kpn((char*)dataPtr,dataSize));

        msgAndSource._event._subMsg->_vals = vals;
        int numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
        if (numWritten != sizeof(msgAndSource))
        {
            // start batching now
            setBatchData(&(it->second), destination.destType, tmpDest.c_str(), replyto.destType, replyToName, correlationid, opaqueFlow_p, msgId, dataPtr, dataSize);
            delete (msgAndSource._event._subMsg);
            // start timer with topic and timer payload
            TimerPayload* timerPayload = new TimerPayload;
            timerPayload->dest.assign(tmpDest);
            solClient_returnCode_t rc;
            if ((rc = solClient_context_startTimer(context, SOLCLIENT_CONTEXT_TIMER_REPEAT, 100, timerCbFunc, (void*)timerPayload, &timerPayload->destTimer)) != SOLCLIENT_OK)
                printf("[%ld] Solace problem starting batch timer for destination %s.\n", THREAD_ID, tmpDest.c_str());
        }
        return SOLCLIENT_CALLBACK_OK;
    }
    else
    {
        // batching
        // add current msg data and get batch size
        int currentBatchSize = setBatchData(&(it->second), destination.destType, tmpDest.c_str(), replyto.destType, replyToName, correlationid, opaqueFlow_p, msgId, dataPtr, dataSize);
        // if batch over batch size attempt to send.
        if (currentBatchSize >= 1000)
        {
            msgAndSource._event._subMsg->_vals = (it->second);
            ssize_t numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
            if (numWritten != sizeof(msgAndSource))
            {
                // pipe still full
                msgAndSource._event._subMsg->_vals = NULL;
                delete (msgAndSource._event._subMsg);
                // leave batch publish timer running
                return SOLCLIENT_CALLBACK_OK;
            }
            // batch is sent over pipe - reset to NULL - Timer will cancel itself
            it->second = NULL;
            return SOLCLIENT_CALLBACK_OK;
        }
        delete (msgAndSource._event._subMsg);
        return SOLCLIENT_CALLBACK_OK;
    }
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
        ssize_t numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
        if (numWritten != sizeof(msgAndSource))
            printf("[%ld] Solace flow problem writting to pipe\n", THREAD_ID);
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
        ssize_t numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
        if (numWritten != sizeof(msgAndSource))
            printf("[%ld] Solace session problem writting to pipe\n", THREAD_ID);
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

    int ret = pipe(CALLBACK_PIPE);
    if (ret != 0)
        return krr((S)"Solace init couldn't create pipe");
    fcntl(CALLBACK_PIPE[0], F_SETFL, O_NONBLOCK);
    fcntl(CALLBACK_PIPE[1], F_SETFL, O_NONBLOCK);
    sd1(CALLBACK_PIPE[0],kdbCallback);

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
    sessionFuncInfo.rxMsgInfo.callback_p = defaultReceiveCallback;
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
    char capStr[getStringSize(capabilityName)];
    setString(capStr,capabilityName,sizeof(capStr));
    return kb(solClient_session_isCapable(session_p,capStr));
}

K getcapability_solace(K capabilityName)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_STRING_TYPE(capabilityName,"getcapability_solace");
    solClient_field_t field;

    char capStr[getStringSize(capabilityName)];
    setString(capStr,capabilityName,sizeof(capStr));

    solClient_returnCode_t returnCode = solClient_session_getCapability (session_p, capStr, &field, sizeof(field));

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

    char topicStr[getStringSize(topic)];
    setString(topicStr,topic,sizeof(topicStr));
    const char** provProps = createProperties(options);
    solClient_returnCode_t retCode = solClient_session_endpointTopicSubscribe((char**)provProps,
                                            session_p,
                                            provFlags->i, 
                                            topicStr,
                                            NULL); // correlation tag
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

    char topicStr[getStringSize(topic)];
    setString(topicStr,topic,sizeof(topicStr));
    const char** provProps = createProperties(options);
    solClient_returnCode_t retCode = solClient_session_endpointTopicUnsubscribe((char**)provProps,
                                            session_p,
                                            provFlags->i, 
                                            topicStr,
                                            NULL); // correlation tag
    free(provProps);
    if (retCode != SOLCLIENT_OK)
    {
        solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo();
        printf("[%ld] Solace createendpoint - solClient_session_endpointTopicUnsubscribe() failed subCode (%d:'%s')\n",THREAD_ID,errorInfo_p->subCode,solClient_subCodeToString(errorInfo_p->subCode)); 
        return ki(errorInfo_p->subCode);
    }                  
    return ki(SOLCLIENT_OK);
}

K senddirect_solace(K topic, K data)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"send_solace");
    CHECK_PARAM_DATA_TYPE(data,"send_solace");
    
    /* Set the destination. */
    solClient_destination_t destination;
    destination.destType = SOLCLIENT_TOPIC_DESTINATION;
    destination.dest = topic->s;

    solClient_opaqueMsg_pt msg_p = NULL;
    solClient_msg_alloc ( &msg_p );
    solClient_msg_setDeliveryMode ( msg_p, SOLCLIENT_DELIVERY_MODE_DIRECT );
    solClient_msg_setDestination ( msg_p, &destination, sizeof ( destination ) );
    solClient_msg_setElidingEligible( msg_p, 1); // set to true
    solClient_msg_setDMQEligible( msg_p, 1); // set to true
    solClient_msg_setSenderTimestamp (msg_p, getMillisSinceEpoch());
    solClient_msg_setBinaryAttachment ( msg_p, getData(data), getDataSize(data));
    solClient_returnCode_t retCode = solClient_session_sendMsg ( session_p, msg_p ); 
    solClient_msg_free ( &msg_p );
    if (retCode != SOLCLIENT_OK)
    {
        printf("[%ld] Solace send error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        return ki(retCode);
    }
    return ki(SOLCLIENT_OK);
}

K sendpersistent_solace(K type, K dest, K replyType, K replydest, K data, K correlationId)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(type,-KI,"sendpersistent_solace");
    CHECK_PARAM_TYPE(dest,-KS,"sendpersistent_solace");
    CHECK_PARAM_DATA_TYPE(data,"sendpersistent_solace");

    /* Set the destination. */
    solClient_destination_t destination;
    if ((type->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (type->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
        destination.destType = SOLCLIENT_QUEUE_DESTINATION;
    else
        destination.destType = SOLCLIENT_TOPIC_DESTINATION;
    destination.dest = dest->s;

    solClient_opaqueMsg_pt msg_p = NULL;
    solClient_msg_alloc ( &msg_p );

    solClient_msg_setDeliveryMode ( msg_p, SOLCLIENT_DELIVERY_MODE_PERSISTENT );
    solClient_msg_setExpiration (msg_p, 0);
    solClient_msg_setSenderTimestamp (msg_p, getMillisSinceEpoch());
    if (correlationId != NULL)
    {
        char correlationIdStr[getStringSize(correlationId)];
        setString(correlationIdStr,correlationId,sizeof(correlationIdStr));
        solClient_msg_setCorrelationId (msg_p, correlationIdStr);
    }
    solClient_msg_setDestination ( msg_p, &destination, sizeof ( destination ) ); 
    setReplyTo(replyType, replydest, msg_p);
    solClient_msg_setBinaryAttachment ( msg_p, getData(data), getDataSize(data));
    solClient_returnCode_t retCode = SOLCLIENT_OK;
    retCode = solClient_session_sendMsg ( session_p, msg_p ); 
    solClient_msg_free ( &msg_p );

    if (retCode != SOLCLIENT_OK)
    {
        printf("[%ld] Solace sendpersistent error %d: %s\n",THREAD_ID,retCode,solClient_returnCodeToString(retCode));
        return ki(retCode);
    }
    return ki(SOLCLIENT_OK);
}

K subscribetmp_solace(K type,  K callbackFunction)
{ 
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(type,-KI,"subscribetmp_solace");
    CHECK_PARAM_TYPE(callbackFunction,-KS,"subscribetmp_solace");

    solClient_flow_createFuncInfo_t flowFuncInfo = SOLCLIENT_SESSION_CREATEFUNC_INITIALIZER;
    flowFuncInfo.rxMsgInfo.callback_p = flowMsgCallbackFunc;
    flowFuncInfo.rxMsgInfo.user_p = NULL;
    flowFuncInfo.eventInfo.callback_p = flowEventCallback;
    flowFuncInfo.eventInfo.user_p = NULL;

    solClient_opaqueFlow_pt flow_p = NULL;
    solClient_returnCode_t rc = SOLCLIENT_OK;
    const char* flowProps[20];
    int propIndex = 0;
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_BLOCKING;
    flowProps[propIndex++] = SOLCLIENT_PROP_ENABLE_VAL;

    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_ENTITY_ID;
    if ((type->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (type->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
        flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_ENTITY_QUEUE;
    else
        flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_ENTITY_TE;
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_ACKMODE;
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_ACKMODE_CLIENT;
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_ENTITY_DURABLE;
    flowProps[propIndex++] = SOLCLIENT_PROP_DISABLE_VAL;
    flowProps[propIndex] = NULL;

    rc = solClient_session_createFlow((char**)flowProps,session_p,&flow_p,&flowFuncInfo,sizeof(flowFuncInfo));
    if (rc != SOLCLIENT_OK)
    {
        printf("[%ld] Solace subscribetmp_solace createFlow error %d: %s\n",THREAD_ID,rc,solClient_returnCodeToString(rc));
        return ks((char*)"");
    }

    solClient_destination_t replyToAddr;
    if ( ( rc = solClient_flow_getDestination ( flow_p, &replyToAddr, sizeof ( replyToAddr ) ) ) != SOLCLIENT_OK )
    {
        printf("[%ld] Solace subscribetmp_solace getdestination error %d: %s\n",THREAD_ID,rc,solClient_returnCodeToString(rc));
        return ks((char*)"");
    }

    // remember callback function to use for this subscription
    FlowSubscriptionInfo subInfo;
    subInfo.flow_pt = flow_p;
    subInfo.flowKdbCbFunc = callbackFunction->s;
    FLOW_INFO.insert(std::pair<std::string,FlowSubscriptionInfo>(replyToAddr.dest,subInfo));

    return ks((char*)replyToAddr.dest);
}

K subscribepersistent_solace(K type, K endpointname, K topicname, K callbackFunction)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(endpointname,-KS,"subscribepersistent_solace");
    CHECK_PARAM_TYPE(callbackFunction,-KS,"subscribepersistent_solace");
    CHECK_PARAM_TYPE(topicname,-KS,"subscribepersistent_solace");
    CHECK_PARAM_TYPE(type,-KI,"subscribepersistent_solace");

    if ((type->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (type->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
    {
        if (FLOW_INFO.find(endpointname->s) != FLOW_INFO.end())
        {
            printf("[%ld] Solace subscription not being created for %s, as existing subscription to queue exists\n",THREAD_ID,endpointname->s);
            return ki(SOLCLIENT_OK);
        }
    }
    else
    {
        if (FLOW_INFO.find(topicname->s) != FLOW_INFO.end())
        {
            printf("[%ld] Solace subscription not being created for %s, as existing subscription to topic exists\n",THREAD_ID,topicname->s);
            return ki(SOLCLIENT_OK);
        }
    }

    solClient_returnCode_t rc = SOLCLIENT_OK;
    const char* flowProps[20];
    int         propIndex = 0;
    solClient_flow_createFuncInfo_t flowFuncInfo = SOLCLIENT_SESSION_CREATEFUNC_INITIALIZER;
    // create callbacks for flow
    flowFuncInfo.rxMsgInfo.callback_p = flowMsgCallbackFunc;
    flowFuncInfo.rxMsgInfo.user_p = NULL;
    flowFuncInfo.eventInfo.callback_p = flowEventCallback;
    flowFuncInfo.eventInfo.user_p = NULL;
    // create flow properties
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_BLOCKING; // block in createFlow call
    flowProps[propIndex++] = SOLCLIENT_PROP_ENABLE_VAL;
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_ENTITY_ID; // the type of object to which this flow is bound
    if ((type->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (type->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
    {
        flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_ENTITY_QUEUE; // for queue subscriptions
    }
    else
    {
        flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_ENTITY_TE; // for topic subscriptions
        flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_TOPIC;
        flowProps[propIndex++] = topicname->s;
    }
    
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_ACKMODE;
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_ACKMODE_CLIENT;
    flowProps[propIndex++] = SOLCLIENT_FLOW_PROP_BIND_NAME;
    flowProps[propIndex++] = endpointname->s;
    // TODO durable types (?)
    flowProps[propIndex] = NULL;

    solClient_opaqueFlow_pt flow_p; 
    rc = solClient_session_createFlow((char**)flowProps,session_p,&flow_p,&flowFuncInfo,sizeof(flowFuncInfo));    
    if (rc != SOLCLIENT_OK)
    {
        printf("[%ld] Solace subscribepersistent createFlow error %d: %s\n",THREAD_ID,rc,solClient_returnCodeToString(rc));
        return ki(rc); 
    }

    // remember callback function to use for this subscription
    FlowSubscriptionInfo subInfo;
    subInfo.flow_pt = flow_p;
    subInfo.flowKdbCbFunc = callbackFunction->s;
    if ((type->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (type->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
        FLOW_INFO.insert(std::pair<std::string,FlowSubscriptionInfo>(endpointname->s,subInfo));
    else
        FLOW_INFO.insert(std::pair<std::string,FlowSubscriptionInfo>(topicname->s,subInfo));

    return ki(SOLCLIENT_OK);
}

K unsubscribepersistent_solace(K type, K endpointname, K topicname)
{
    CHECK_PARAM_TYPE(endpointname,-KS,"unsubscribepersistent_solace");
    CHECK_PARAM_TYPE(topicname,-KS,"unsubscribepersistent_solace");
    CHECK_PARAM_TYPE(type,-KI,"unsubscribepersistent_solace");
    
    const char* subname = NULL;
    if ((type->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (type->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
        subname = endpointname->s;
    else
        subname = topicname->s;

    std::map<std::string,FlowSubscriptionInfo>::iterator it;
    it = FLOW_INFO.find(subname);
    if (it == FLOW_INFO.end())
    {
        printf("[%ld] Solace unsubscribe for subscription %s that doesnt exist\n", THREAD_ID, subname);
        return krr((S)"Solace unsubscribe for subscription that doesnt exist");
    }

    solClient_opaqueFlow_pt flow_pt = it->second.flow_pt;
    solClient_returnCode_t retCode = solClient_flow_destroy(&flow_pt);
    if (retCode != SOLCLIENT_OK)
        printf("[%ld] Solace unsubscribe failure for %s\n", THREAD_ID, subname);
    else
        printf("[%ld] Solace subscription removed for %s\n", THREAD_ID, subname);

    FLOW_INFO.erase(it);

    return ki(retCode);
}
