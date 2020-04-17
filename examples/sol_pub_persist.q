\l solace.q

default_nm:`host`vpn`user`pass`desttype`destname`data`correlationid
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "queue";enlist "Q/test";enlist "kdb data";enlist "")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback[`flowUpdate];

/ perform solace actions
q:`$"queue"
.solace.dest_type:`int$(q=`$first params`desttype)

.solace.init[soloptions]

/ sending a persistent message to a queue
.solace.sendpersistent[.solace.dest_type;`$first params`destname;`$first params`data;`$first params`correlationid]

.solace.destroy[1i]

exit 0
