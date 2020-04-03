default_nm:`host`vpn`user`pass`opt
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "SESSION_PEER_PLATFORM")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ initialise the solace api function
.solace.setsessioncallback_solace:`libdeltasolace 2:(`setsessioncallback_solace;1)
.solace.setflowcallback_solace:`libdeltasolace 2:(`setflowcallback_solace;1)
.solace.init_solace:`libdeltasolace 2:(`init_solace;1)
.solace.version_solace:`libdeltasolace 2:(`version_solace;1)
.solace.getcapability_solace:`libdeltasolace 2:(`getcapability_solace;1)
.solace.destroy_solace:`libdeltasolace 2:(`destroy_solace;1)

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

("API Version Info: ";.solace.version_solace[1i])

.solace.init_solace[soloptions]

c:`$first params`opt
(c;.solace.getcapability_solace[c])

.solace.destroy_solace[1i]

exit 0
