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
static std::string KDB_DIRECT_MSG_CALLBACK_FUNC;

enum KdbSolaceEndpointType
{
    KDB_SOLACE_ENDPOINT_TYPE_TOPIC      = 0,
    KDB_SOLACE_ENDPOINT_TYPE_QUEUE      = 1,
    KDB_SOLACE_ENDPOINT_TYPE_TMP_TOPIC  = 2,
    KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE  = 3
};

struct GurananteedSubInfo
{
    solClient_opaqueFlow_pt flow_pt;
    std::string             flowKdbCbFunc;
};
static std::map<std::string, GurananteedSubInfo> GUARANTEED_SUB_INFO;

typedef std::map<std::string, K> batchInfoMap;
static batchInfoMap BATCH_PER_DESTINATION;

static void socketWrittableCbFunc(solClient_opaqueContext_pt opaqueContext_p, solClient_fd_t fd, solClient_fdEvent_t events, void *user_p)
{
    batchInfoMap::iterator itr = BATCH_PER_DESTINATION.begin();
    while (itr!=BATCH_PER_DESTINATION.end())
    {
        batchInfoMap::iterator currentDest = itr;
        ++itr;
        // attempt to send the batch over the pipe
        KdbSolaceEvent msgAndSource;
        msgAndSource._type = GUARANTEED_MSG_EVENT;
        msgAndSource._event._subMsg = new KdbSolaceEventGuarSubMsg;
        msgAndSource._event._subMsg->_destName.assign(currentDest->first);
        msgAndSource._event._subMsg->_vals = (currentDest->second);
        int numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
        if (numWritten != sizeof(msgAndSource))
        {
            delete (msgAndSource._event._subMsg);
            return;
        }
        BATCH_PER_DESTINATION.erase(currentDest);
    }
    solClient_context_unregisterForFdEvents(opaqueContext_p,fd,events);
}

K createBatch()
{
    K vals = knk(0);
    jk(&(vals), ktn(KI, 0));//[0]
    jk(&(vals), knk(0));    //[1]
    jk(&(vals), ktn(KI,0)); //[2]
    jk(&(vals), knk(0));    //[3]
    jk(&(vals), knk(0));    //[4]
    jk(&(vals), ktn(KJ,0)); //[5]
    jk(&(vals), ktn(KJ,0)); //[6]
    jk(&(vals), knk(0));    //[7]
    return vals;
}

int addMsgToBatch(K* batch, int subsType, const char* topicName, int replyType, const char* replyName, const char* correlationId, solClient_opaqueFlow_pt flowPtr, solClient_msgId_t msgId, void* data, solClient_uint32_t dataSize)
{
    ja(&(((K*)(*batch)->G0)[0]), &subsType);
    jk(&(((K*)(*batch)->G0)[1]), kp((char*)topicName));
    ja(&(((K*)(*batch)->G0)[2]), &replyType);
    jk(&(((K*)(*batch)->G0)[3]), kp((char*)replyName));
    jk(&(((K*)(*batch)->G0)[4]), kp((char*)correlationId));
    ja(&(((K*)(*batch)->G0)[5]), &flowPtr);
    ja(&(((K*)(*batch)->G0)[6]), &msgId);
    K payload = ktn(KG, dataSize);
    memcpy(payload->G0, data, dataSize);
    jk(&(((K*)(*batch)->G0)[7]), payload);
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
                K eventtype = ki(msgAndSource._event._session->_eventType);
                K responsecode = ki(msgAndSource._event._session->_responseCode);
                K eventinfo = kp((char*)msgAndSource._event._session->_eventInfo.c_str()); 
                K result = k(0, (char*)KDB_SESSION_EVENT_CALLBACK_FUNC.c_str(), eventtype, responsecode, eventinfo, (K)0);
                if(-128 == result->t)
                {
                    printf("[%ld] Solace not able to call kdb function %s with received data (eventtype:%d responsecode:%d eventinfo:%s)\n", 
                                    THREAD_ID, 
                                    KDB_SESSION_EVENT_CALLBACK_FUNC.c_str(),
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
                K eventtype = ki(msgAndSource._event._flow->_eventType);
                K responsecode = ki(msgAndSource._event._flow->_responseCode);
                K eventinfo = kp((char*)msgAndSource._event._flow->_eventInfo.c_str());
                K destType = ki(msgAndSource._event._flow->_destType);
                K destName = kp((char*)msgAndSource._event._flow->_destName.c_str());
                K result = k(0, (char*)KDB_FLOW_EVENT_CALLBACK_FUNC.c_str(), eventtype, responsecode, eventinfo, destType, destName, (K)0);
                if(-128 == result->t)
                {
                    printf("[%ld] Solace not able to call kdb function %s\n", THREAD_ID, KDB_FLOW_EVENT_CALLBACK_FUNC.c_str()); 
                }
            }
            delete (msgAndSource._event._flow);
            return (K)0;
        }
        else if (msgAndSource._type == DIRECT_MSG_EVENT)
        {
            solClient_opaqueMsg_pt msg = msgAndSource._event._directMsg;
            if (KDB_DIRECT_MSG_CALLBACK_FUNC.empty())
            {
                solClient_msg_free(&msg);
                return (K)0;
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
                if (solClient_session_sendReply(session_p,msg,replyMsg) != SOLCLIENT_OK)
                    solClient_msg_free(&replyMsg);
            }
            solClient_msg_free(&msg);
            return (K)0;
        }
        else if (msgAndSource._type != GUARANTEED_MSG_EVENT)
            return (K)0;
        
        std::map<std::string,GurananteedSubInfo>::const_iterator it = GUARANTEED_SUB_INFO.find(msgAndSource._event._subMsg->_destName);
        if (it == GUARANTEED_SUB_INFO.end())
        {
            printf ( "[%ld] Solace received callback message on flow with no callback function subscription:%s)\n", THREAD_ID, msgAndSource._event._subMsg->_destName.c_str());
            // clean up the sub item
            delete (msgAndSource._event._subMsg);
            return (K)0;
        }
        
        K keys = ktn(KS,8);
        kS(keys)[0]=ss((char*)"subsType");
        kS(keys)[1]=ss((char*)"topicName");
        kS(keys)[2]=ss((char*)"replyType");
        kS(keys)[3]=ss((char*)"replyName");
        kS(keys)[4]=ss((char*)"correlationId");
        kS(keys)[5]=ss((char*)"flowPtr");
        kS(keys)[6]=ss((char*)"msgId");
        kS(keys)[7]=ss((char*)"payload");
        K dict = xD(keys, msgAndSource._event._subMsg->_vals);
        K result = k(0, (char*)it->second.flowKdbCbFunc.c_str(), dict, (K)0);
        if(-128 == result->t)
            printf("[%ld] Solace not able to call kdb function %s with received data (destination:%s)\n", THREAD_ID, it->second.flowKdbCbFunc.c_str(), msgAndSource._event._subMsg->_destName.c_str());
        
        // clean up the sub item
        delete (msgAndSource._event._subMsg);
    }
    return (K)0;
}

/* The message receive callback function is mandatory for session creation. Gets called with msgs from direct subscriptions. */
solClient_rxMsgCallback_returnCode_t defaultSubCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{
    KdbSolaceEvent msgAndSource;
    msgAndSource._type = DIRECT_MSG_EVENT;
    msgAndSource._event._directMsg=msg_p;
    ssize_t numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
    if (numWritten != sizeof(msgAndSource))
    {
        //TODO printf("Blocked!\n");
        return SOLCLIENT_CALLBACK_OK;
    }
    // take control of the msg_p memory
    return SOLCLIENT_CALLBACK_TAKE_MSG;
}

solClient_rxMsgCallback_returnCode_t guaranteedSubCallback ( solClient_opaqueFlow_pt opaqueFlow_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{
    //solClient_msg_dump(msg_p,NULL,0);
    solClient_destination_t destination;
    if (solClient_flow_getDestination(opaqueFlow_p,&destination,sizeof(destination)) != SOLCLIENT_OK)
    {
        printf("[%ld] Solace cant get destination of flow\n",THREAD_ID);
        return SOLCLIENT_CALLBACK_OK;
    }

    KdbSolaceEvent msgAndSource;
    msgAndSource._type = GUARANTEED_MSG_EVENT;
    msgAndSource._event._subMsg = new KdbSolaceEventGuarSubMsg;
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
    if(BATCH_PER_DESTINATION.empty())
    {
        // not baching - yet
        K vals = createBatch();
        addMsgToBatch(&(vals), destination.destType, tmpDest.c_str(), replyto.destType, replyToName, correlationid, opaqueFlow_p, msgId, dataPtr, dataSize);
        msgAndSource._event._subMsg->_vals = vals;
        int numWritten = write(CALLBACK_PIPE[1], &msgAndSource, sizeof(msgAndSource));
        if (numWritten != sizeof(msgAndSource))
        {
            // start batching now
            delete (msgAndSource._event._subMsg);
            BATCH_PER_DESTINATION.insert(std::make_pair(tmpDest, vals));

            solClient_returnCode_t rc;
            if ((rc = solClient_context_registerForFdEvents(context,CALLBACK_PIPE[1],SOLCLIENT_FD_EVENT_WRITE,socketWrittableCbFunc,NULL)) != SOLCLIENT_OK)
                printf("[%ld] Solace problem create fd monitor\n", THREAD_ID);
        }
        return SOLCLIENT_CALLBACK_OK;
    }
    else
    {
        batchInfoMap::iterator it = BATCH_PER_DESTINATION.find(tmpDest);
        if (it == BATCH_PER_DESTINATION.end())
            it = BATCH_PER_DESTINATION.insert(it,std::make_pair(tmpDest, createBatch()));
        // add current msg data and get batch size
        int currentBatchSize = addMsgToBatch(&(it->second), destination.destType, tmpDest.c_str(), replyto.destType, replyToName, correlationid, opaqueFlow_p, msgId, dataPtr, dataSize);
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

static solClient_opaqueMsg_pt createDirectMsg(K topic,K data)
{
    solClient_destination_t destination;
    destination.destType = SOLCLIENT_TOPIC_DESTINATION;
    destination.dest = topic->s;

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

 K senddirectrequest_solace(K topic, K data, K timeout)
 {
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"senddirectrequest_solace");
    CHECK_PARAM_DATA_TYPE(data,"senddirectrequest_solace");
    CHECK_PARAM_TYPE(timeout,-KI,"senddirectrequest_solace");
    if (timeout->i <= 0)
        krr((char*)"senddirectrequest_solace must be provided with timeout greater than zero");
    solClient_opaqueMsg_pt msg_p = createDirectMsg(topic,data);
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

K callbackdirect_solace(K func)
{
    CHECK_PARAM_STRING_TYPE(func,"callbackdirect_solace");
    char cbStr[getStringSize(func)];
    setString(cbStr,func,sizeof(cbStr));
    KDB_DIRECT_MSG_CALLBACK_FUNC.assign(cbStr);
    return ki(SOLCLIENT_OK);
}

K subscribedirect_solace(K topic)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"subscribedirect_solace");
    solClient_returnCode_t ret = solClient_session_topicSubscribeExt ( session_p,
                                          SOLCLIENT_SUBSCRIBE_FLAGS_REQUEST_CONFIRM,
                                          topic->s );
    return ki(ret);
}

K unsubscribedirect_solace(K topic)
{
    CHECK_SESSION_CREATED;
    CHECK_PARAM_TYPE(topic,-KS,"subscribedirect_solace");
    solClient_returnCode_t ret = solClient_session_topicUnsubscribeExt (session_p,
                                        SOLCLIENT_SUBSCRIBE_FLAGS_REQUEST_CONFIRM,
                                        topic->s );
    return ki(ret);
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
    flowFuncInfo.rxMsgInfo.callback_p = guaranteedSubCallback;
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
    GurananteedSubInfo subInfo;
    subInfo.flow_pt = flow_p;
    subInfo.flowKdbCbFunc = callbackFunction->s;
    GUARANTEED_SUB_INFO.insert(std::pair<std::string,GurananteedSubInfo>(replyToAddr.dest,subInfo));

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
        if (GUARANTEED_SUB_INFO.find(endpointname->s) != GUARANTEED_SUB_INFO.end())
        {
            printf("[%ld] Solace subscription not being created for %s, as existing subscription to queue exists\n",THREAD_ID,endpointname->s);
            return ki(SOLCLIENT_OK);
        }
    }
    else
    {
        if (GUARANTEED_SUB_INFO.find(topicname->s) != GUARANTEED_SUB_INFO.end())
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
    flowFuncInfo.rxMsgInfo.callback_p = guaranteedSubCallback;
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
    GurananteedSubInfo subInfo;
    subInfo.flow_pt = flow_p;
    subInfo.flowKdbCbFunc = callbackFunction->s;
    if ((type->i == KDB_SOLACE_ENDPOINT_TYPE_QUEUE) || (type->i == KDB_SOLACE_ENDPOINT_TYPE_TMP_QUEUE))
        GUARANTEED_SUB_INFO.insert(std::pair<std::string,GurananteedSubInfo>(endpointname->s,subInfo));
    else
        GUARANTEED_SUB_INFO.insert(std::pair<std::string,GurananteedSubInfo>(topicname->s,subInfo));

    return ki(SOLCLIENT_OK);
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

    std::map<std::string,GurananteedSubInfo>::iterator it;
    it = GUARANTEED_SUB_INFO.find(subname);
    if (it == GUARANTEED_SUB_INFO.end())
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

    GUARANTEED_SUB_INFO.erase(it);

    return ki(retCode);
}
