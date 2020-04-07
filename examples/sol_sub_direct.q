\c 25 1000

default_nm:`host`vpn`user`pass`topic
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ initialise the solace api function
.solace.setsessioncallback_solace:`libdeltasolace 2:(`setsessioncallback_solace;1)
.solace.setflowcallback_solace:`libdeltasolace 2:(`setflowcallback_solace;1)
.solace.init_solace:`libdeltasolace 2:(`init_solace;1)
.solace.callbacktopic_solace:`libdeltasolace 2:(`callbacktopic_solace;1)
.solace.subscribetopic_solace:`libdeltasolace 2:(`subscribetopic_solace;1)
.solace.unsubscribetopic_solace:`libdeltasolace 2:(`unsubscribetopic_solace;1)
.solace.destroy_solace:`libdeltasolace 2:(`destroy_solace;1)

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
.solace.subscribetopic_solace[`$first params`topic]

/ dont disconnect or quit, in order to receive any messages
