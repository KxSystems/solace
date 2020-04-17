\d .solace

/ initialise the solace api function
setsessioncallback:`libkdbsolace 2:(`setsessioncallback_solace;1)
setflowcallback:`libkdbsolace 2:(`setflowcallback_solace;1)
init:`libkdbsolace 2:(`init_solace;1)
version:`libkdbsolace 2:(`version_solace;1)
getcapability:`libkdbsolace 2:(`getcapability_solace;1)
createendpoint:`libkdbsolace 2:(`createendpoint_solace;2)
destroyendpoint:`libkdbsolace 2:(`destroyendpoint_solace;2)
senddirect:`libkdbsolace 2:(`senddirect_solace;2)
senddirectrequest:`libkdbsolace 2:(`senddirectrequest_solace;5)
sendpersistent:`libkdbsolace 2:(`sendpersistent_solace;4)
callbacktopic:`libkdbsolace 2:(`callbacktopic_solace;1)
subscribetopic:`libkdbsolace 2:(`subscribetopic_solace;2)
unsubscribetopic:`libkdbsolace 2:(`unsubscribetopic_solace;1)
bindqueue:`libkdbsolace 2:(`bindqueue_solace;2)
unbindqueue:`libkdbsolace 2:(`unbindqueue_solace;1)
sendack:`libkdbsolace 2:(`sendack_solace;2)
endpointtopicsubscribe:`libkdbsolace 2:(`endpointtopicsubscribe_solace;3)
destroy:`libkdbsolace 2:(`destroy_solace;1)

\d .