/ goal to internalise these in the c/so

.solace.returnCodes:(0 1 2 3 4 5 6 7 8 -1)!(`SOLCLIENT_OK`SOLCLIENT_WOULD_BLOCK`SOLCLIENT_IN_PROGRESS`SOLCLIENT_NOT_READY`SOLCLIENT_EOS`SOLCLIENT_NOT_FOUND`SOLCLIENT_NOEVENT`SOLCLIENT_INCOMPLETE`SOLCLIENT_ROLLBACK`SOLCLIENT_FAIL);
.solace.replyToCodes:(0 1 2 3 -1)!(`SOLCLIENT_TOPIC_DESTINATION,`SOLCLIENT_QUEUE_DESTINATION,`SOLCLIENT_TOPIC_TEMP_DESTINATION,`SOLCLIENT_QUEUE_TEMP_DESTINATION,`SOLCLIENT_NULL_DESTINATION);
.solace.sessionEventCodes:(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20)!(`SOLCLIENT_SESSION_EVENT_UP_NOTICE`SOLCLIENT_SESSION_EVENT_DOWN_ERROR`SOLCLIENT_SESSION_EVENT_CONNECT_FAILED_ERROR`SOLCLIENT_SESSION_EVENT_REJECTED_MSG_ERROR`SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_ERROR`SOLCLIENT_SESSION_EVENT_RX_MSG_TOO_BIG_ERROR`SOLCLIENT_SESSION_EVENT_ACKNOWLEDGEMENT`SOLCLIENT_SESSION_EVENT_ASSURED_CONNECT_FAILED`SOLCLIENT_SESSION_EVENT_ASSURED_DELIVERY_DOWN`SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_ERROR`SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_OK`SOLCLIENT_SESSION_EVENT_CAN_SEND`SOLCLIENT_SESSION_EVENT_RECONNECTING_NOTICE`SOLCLIENT_SESSION_EVENT_RECONNECTED_NOTICE`SOLCLIENT_SESSION_EVENT_PROVISION_ERROR`SOLCLIENT_SESSION_EVENT_PROVISION_OK`SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_OK`SOLCLIENT_SESSION_EVENT_VIRTUAL_ROUTER_NAME_CHANGED`SOLCLIENT_SESSION_EVENT_MODIFYPROP_OK`SOLCLIENT_SESSION_EVENT_MODIFYPROP_FAIL`SOLCLIENT_SESSION_EVENT_REPUBLISH_UNACKED_MESSAGES);
.solace.flowEventCodes:(0 1 2 3 4 5 6)!(`SOLCLIENT_FLOW_EVENT_UP_NOTICE`SOLCLIENT_FLOW_EVENT_DOWN_ERROR`SOLCLIENT_FLOW_EVENT_BIND_FAILED_ERROR`SOLCLIENT_FLOW_EVENT_REJECTED_MSG_ERROR`SOLCLIENT_FLOW_EVENT_SESSION_DOWN`SOLCLIENT_FLOW_EVENT_ACTIVE`SOLCLIENT_FLOW_EVENT_INACTIVE);

solace:` sv(`$":",getenv`DELTASOLACE_HOME;`clib;`libdeltasolace); //load in the clib

/ wrapper function to load in C functions from the shared object
loadSharedObj:{[libFile;cFunc;nparams]
  kdbFunc:` sv `.solace,cFunc;
  kdbFunc set @[
    {[cFunc;nparams;solace]solace 2:(cFunc;nparams)}[cFunc;nparams];
    libFile;
    {[cFunc;nparams;err].log.err[`SolaceInit;"Solace Error loading solace shared object";cFunc]; value"{[",(";"sv string nparams#.Q.a),"]}" }[cFunc;nparams]
  ];
 }[solace];
loadSharedObj[`init_solace;1];
loadSharedObj[`iscapable_solace;1];
loadSharedObj[`getcapability_solace;1];
loadSharedObj[`createendpoint_solace;2];
loadSharedObj[`destroyendpoint_solace;2];
loadSharedObj[`destroy_solace;2];
loadSharedObj[`send_solace;5];
loadSharedObj[`sendpersistent_solace;7];
loadSharedObj[`subscribepersistent_solace;4];
loadSharedObj[`unsubscribepersistent_solace;3];
loadSharedObj[`setsessioncallback_solace;1];
loadSharedObj[`setflowcallback_solace;1];
loadSharedObj[`sendack_solace;2]
loadSharedObj[`subscribetmp_solace;3];
