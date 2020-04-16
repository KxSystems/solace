\l solace.q

default_nm:`host`vpn`user`pass`topic`data
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test";enlist "kdb data")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD`SESSION_SEND_TIMESTAMP!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass;`$"1");

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

/ perform solace actions
.solace.init_solace[soloptions]

/ sending a direct message to a topic
.solace.senddirect_solace[`$first params`topic;`$first params`data]

.solace.destroy_solace[1i]

exit 0
