\d .solace

/ initialise the solace api function
setSessionCallback:`libkdbsolace 2:(`setsessioncallback_solace;1)
setFlowCallback:`libkdbsolace 2:(`setflowcallback_solace;1)
init:`libkdbsolace 2:(`init_solace;1)
version:`libkdbsolace 2:(`version_solace;1)
getCapability:`libkdbsolace 2:(`getcapability_solace;1)
createEndpoint:`libkdbsolace 2:(`createendpoint_solace;2)
destroyEndpoint:`libkdbsolace 2:(`destroyendpoint_solace;2)
sendDirect:`libkdbsolace 2:(`senddirect_solace;2)
sendDirectRequest:`libkdbsolace 2:(`senddirectrequest_solace;5)
sendPersistent:`libkdbsolace 2:(`sendpersistent_solace;4)
sendPersistentRequest:`libkdbsolace 2:(`sendpersistentrequest_solace;6)
callbackTopic:`libkdbsolace 2:(`callbacktopic_solace;1)
subscribeTopic:`libkdbsolace 2:(`subscribetopic_solace;2)
unSubscribeTopic:`libkdbsolace 2:(`unsubscribetopic_solace;1)
callbackQueue:`libkdbsolace 2:(`callbackqueue_solace;1)
bindQueue:`libkdbsolace 2:(`bindqueue_solace;1)
unBindQueue:`libkdbsolace 2:(`unbindqueue_solace;1)
sendAck:`libkdbsolace 2:(`sendack_solace;2)
endpointTopicSubscribe:`libkdbsolace 2:(`endpointtopicsubscribe_solace;3)
destroy:`libkdbsolace 2:(`destroy_solace;1)

\d .