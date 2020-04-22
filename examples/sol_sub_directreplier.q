\l solace.q

\c 25 1000

default_nm:`host`vpn`user`pass`topic
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setSessionCallback[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setFlowCallback[`flowUpdate];

/ perform solace actions
.solace.init[soloptions]

/ receiving and reply to a direct msg (reply only if incoming msg has isRequest set to true)
subUpdate:{[x;y;z] 0N!("RECEIVED MSG: #### Destination ";x;" Payload ";`char$y;" Dict ";z);$[z`isRequest;`byte$"reply contents";0i]};
.solace.setTopicMsgCallback[`subUpdate] 

.solace.subscribeTopic[`$first params`topic;1b]

/ dont disconnect or quit, in order to receive any messages
