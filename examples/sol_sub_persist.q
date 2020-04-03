\c 25 1000

default_nm:`host`vpn`user`pass`destname
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ initialise the solace api function
.solace.setsessioncallback_solace:`libdeltasolace 2:(`setsessioncallback_solace;1)
.solace.setflowcallback_solace:`libdeltasolace 2:(`setflowcallback_solace;1)
.solace.init_solace:`libdeltasolace 2:(`init_solace;1)
.solace.subscribepersistent_solace:`libdeltasolace 2:(`subscribepersistent_solace;2)
.solace.unsubscribepersistent_solace:`libdeltasolace 2:(`unsubscribepersistent_solace;1)
.solace.sendack_solace:`libdeltasolace 2:(`sendack_solace;2)
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
subUpdate:{[r] 0N!("RECEIVED MSG: ####";r;" payload: ";`char$first r`payload);.solace.sendack_solace[first r`flowPtr;first r`msgId]};
.solace.subscribepersistent_solace[`$first params`destname;`subUpdate]

/ dont disconnect or quit, in order to receive any messages
