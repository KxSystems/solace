/ setup a table of which to get rows in json
n:20;
t:([]sym:20?`a`b`c`d`e`f`g;price:n?20f;volume:n?100j);
getJSRow:{[t;index] .j.j select from t where i=index};

/ initialise the solace api function
solace:`$"/usr/local/solace/libdeltasolace"
.solace.setsessioncallback_solace:solace 2:(`setsessioncallback_solace;1)
.solace.setflowcallback_solace:solace 2:(`setflowcallback_solace;1)
.solace.init_solace:solace 2:(`init_solace;1)
.solace.iscapable_solace:solace 2:(`iscapable_solace;1)
.solace.createendpoint_solace:solace 2:(`createendpoint_solace;2)
.solace.destroyendpoint_solace:solace 2:(`destroyendpoint_solace;2)
.solace.send_solace:solace 2:(`send_solace;5)
.solace.subscribetmp_solace:solace 2:(`subscribetmp_solace;3)
.solace.sendpersistent_solace:solace 2:(`sendpersistent_solace;7)
.solace.subscribepersistent_solace:solace 2:(`subscribepersistent_solace;4)
.solace.sendack_solace:solace 2:(`sendack_solace;2)
.solace.destroy_solace:solace 2:(`destroy_solace;1)

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

/ perform solace actions
queue_type:1i
topic_type:0i

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD`SESSION_RECONNECT_RETRIES!(`host.docker.internal;`default;`admin;`admin;`3);
.solace.init_solace[soloptions]

/ subscribing on a tmp topic
/tmpSubUpdate:{[r] 0N!("RECEIVED MSG ON TMP ENDPOINT: ####";r)};
/tmptopic:.solace.subscribetmp_solace[0;topic_type;`tmpSubUpdate]
/.solace.send_solace[tmptopic;0;0;`$"this is a test";`$"correlationid"]

/ publishing, receiving and replying to a msg 
epoptions:`ENDPOINT_ID`ENDPOINT_NAME`ENDPOINT_PERMISSION`ENDPOINT_ACCESSTYPE!(`2;`$"Q/testreplies";`c;`1);
.solace.createendpoint_solace[epoptions;1i]
subMsgUpdate:{[r] 0N!("RECEIVED MSG (replying to this message): ####";r);.solace.send_solace[`$(first r`replyName);0;0;`$"this is a reply";`$(first r`correlationId)];.solace.sendack_solace[first r`flowPtr;first r`msgId]};
.solace.subscribepersistent_solace[queue_type;`$"Q/testreplies";`$"maintopic";`subMsgUpdate]

replySubUpdate:{[r] 0N!("RECEIVED REPLY MSG ON TMP ENDPOINT: ####";r)};
replytopic:.solace.subscribetmp_solace[0;topic_type;`replySubUpdate]
.solace.sendpersistent_solace[queue_type;`$"Q/testreplies";topic_type;replytopic;`$"data sent";`;`]

/.solace.destroy_solace[1i]
