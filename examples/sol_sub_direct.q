\c 25 1000

default_nm:`host`vpn`user`pass`topic
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ initialise the solace api function
.solace.setsessioncallback_solace:`libdeltasolace 2:(`setsessioncallback_solace;1)
.solace.setflowcallback_solace:`libdeltasolace 2:(`setflowcallback_solace;1)
.solace.init_solace:`libdeltasolace 2:(`init_solace;1)
.solace.callbackdirect_solace:`libdeltasolace 2:(`callbackdirect_solace;1)
.solace.subscribedirect_solace:`libdeltasolace 2:(`subscribedirect_solace;1)
.solace.unsubscribedirect_solace:`libdeltasolace 2:(`unsubscribedirect_solace;1)
.solace.destroy_solace:`libdeltasolace 2:(`destroy_solace;1)

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

/ perform solace actions
.solace.init_solace[soloptions]

/ receiving and acknowledging a persistent msg (TODO loop over times when >1 msg)
subUpdate:{[r] 0N!("RECEIVED MSG: ####";`char$r)};
.solace.callbackdirect_solace[`subUpdate]
.solace.subscribedirect_solace[`$first params`topic]

/ dont disconnect or quit, in order to receive any messages
