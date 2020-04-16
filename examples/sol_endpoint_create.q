\l solace.q

default_nm:`host`vpn`user`pass`name
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

epoptions:`ENDPOINT_ID`ENDPOINT_NAME`ENDPOINT_PERMISSION`ENDPOINT_ACCESSTYPE!(`2;`$first params`name;`c;`1);
.solace.createendpoint_solace[epoptions;1i]

.solace.destroy_solace[1i]

exit 0
