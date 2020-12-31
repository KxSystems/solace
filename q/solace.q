\d .solace

/ initialise the solace api function
setSessionCallback:`solacekdb 2:(`setsessioncallback_solace;1)
setFlowCallback:`solacekdb 2:(`setflowcallback_solace;1)
init:`solacekdb 2:(`init_solace;1)
version:`solacekdb 2:(`version_solace;1)
getCapability:`solacekdb 2:(`getcapability_solace;1)
createEndpoint:`solacekdb 2:(`createendpoint_solace;2)
destroyEndpoint:`solacekdb 2:(`destroyendpoint_solace;2)
sendDirect:`solacekdb 2:(`senddirect_solace;2)
sendDirectRequest:`solacekdb 2:(`senddirectrequest_solace;5)
sendPersistent:`solacekdb 2:(`sendpersistent_solace;4)
sendPersistentRequest:`solacekdb 2:(`sendpersistentrequest_solace;6)
subscribeTopic:`solacekdb 2:(`subscribetopic_solace;2)
unSubscribeTopic:`solacekdb 2:(`unsubscribetopic_solace;1)
bindQueue:`solacekdb 2:(`bindqueue_solace;1)
unBindQueue:`solacekdb 2:(`unbindqueue_solace;1)
sendAck:`solacekdb 2:(`sendack_solace;2)
endpointTopicSubscribe:`solacekdb 2:(`endpointtopicsubscribe_solace;3)
endpointTopicUnsubscribe:`solacekdb 2:(`endpointtopicunsubscribe_solace;3)
destroy:`solacekdb 2:(`destroy_solace;1)

setTopicMsgCallback:`solacekdb 2:(`callbacktopic_solace;1)
setQueueMsgCallback:`solacekdb 2:(`callbackqueue_solace;1)

setTopicRawMsgCallback:`solacekdb 2:(`callbacktopicraw_solace;1)
setQueueRawMsgCallback:`solacekdb 2:(`callbackqueueraw_solace;1)

getPayloadAsXML:`solacekdb 2:(`getPayloadAsXML;1)
getPayloadAsString:`solacekdb 2:(`getPayloadAsString;1)
getPayloadAsBinary:`solacekdb 2:(`getPayloadAsBinary;1)

\d .
