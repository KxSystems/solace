\l solace.q

\c 25 1000

default_nm:`host`vpn`user`pass`topic
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

/ perform solace actions
.solace.init_solace[soloptions]

/ receiving a direct msg 
subUpdate:{[x;y;z] 0N!("RECEIVED MSG: #### Destination ";x;" Payload ";`char$y;" Dict ";z)};
.solace.callbacktopic_solace[`subUpdate]
.solace.subscribetopic_solace[`$first params`topic;1b]

/ dont disconnect or quit, in order to receive any messages
