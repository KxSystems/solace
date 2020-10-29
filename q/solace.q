\d .solace

/ initialise the solace api function
setSessionCallback:`kdbsolace 2:(`setsessioncallback_solace;1)
setFlowCallback:`kdbsolace 2:(`setflowcallback_solace;1)
init:`kdbsolace 2:(`init_solace;1)
version:`kdbsolace 2:(`version_solace;1)
getCapability:`kdbsolace 2:(`getcapability_solace;1)
createEndpoint:`kdbsolace 2:(`createendpoint_solace;2)
destroyEndpoint:`kdbsolace 2:(`destroyendpoint_solace;2)
sendDirect:`kdbsolace 2:(`senddirect_solace;2)
sendDirectRequest:`kdbsolace 2:(`senddirectrequest_solace;5)
sendPersistent:`kdbsolace 2:(`sendpersistent_solace;4)
sendPersistentRequest:`kdbsolace 2:(`sendpersistentrequest_solace;6)
subscribeTopic:`kdbsolace 2:(`subscribetopic_solace;2)
unSubscribeTopic:`kdbsolace 2:(`unsubscribetopic_solace;1)
bindQueue:`kdbsolace 2:(`bindqueue_solace;1)
unBindQueue:`kdbsolace 2:(`unbindqueue_solace;1)
sendAck:`kdbsolace 2:(`sendack_solace;2)
endpointTopicSubscribe:`kdbsolace 2:(`endpointtopicsubscribe_solace;3)
endpointTopicUnsubscribe:`kdbsolace 2:(`endpointtopicunsubscribe_solace;3)
destroy:`kdbsolace 2:(`destroy_solace;1)

setTopicMsgCallback:`kdbsolace 2:(`callbacktopic_solace;1)
setQueueMsgCallback:`kdbsolace 2:(`callbackqueue_solace;1)

setTopicRawMsgCallback:`kdbsolace 2:(`callbacktopicraw_solace;1)
setQueueRawMsgCallback:`kdbsolace 2:(`callbackqueueraw_solace;1)

getPayloadAsXML:`kdbsolace 2:(`getPayloadAsXML;1)
getPayloadAsString:`kdbsolace 2:(`getPayloadAsString;1)
getPayloadAsBinary:`kdbsolace 2:(`getPayloadAsBinary;1)

\d .
