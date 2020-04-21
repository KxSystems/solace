\l solace.q

\c 25 1000

default_nm:`host`vpn`user`pass`destname
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

/ receiving and acknowledging a persistent msg (TODO loop over times when >1 msg)
subUpdate:{[x;y] 0N!("RECEIVED MSG: ####";y;" payload: ";{`char$x}x);.solace.sendAck'[y`flowDestName;y`msgId]};

bindopts:(`FLOW_BIND_BLOCKING;`FLOW_BIND_ENTITY_ID;`FLOW_ACKMODE;`FLOW_BIND_NAME)!(`1;`2;`2;`$first params`destname)

.solace.callbackQueue[`subUpdate]
.solace.bindQueue[bindopts]

/ dont disconnect or quit, in order to receive any messages
