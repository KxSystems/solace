default_nm:`host`vpn`user`pass`topic`data
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test";enlist "kdb data")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD`SESSION_SEND_TIMESTAMP!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass;`$"1");

/ initialise the solace api function
.solace.setsessioncallback_solace:`libdeltasolace 2:(`setsessioncallback_solace;1)
.solace.setflowcallback_solace:`libdeltasolace 2:(`setflowcallback_solace;1)
.solace.init_solace:`libdeltasolace 2:(`init_solace;1)
.solace.senddirectrequest_solace:`libdeltasolace 2:(`senddirectrequest_solace;5)
.solace.destroy_solace:`libdeltasolace 2:(`destroy_solace;1)

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

/ perform solace actions
.solace.init_solace[soloptions]

/ sending a direct message to topic requiring a reply
reply:.solace.senddirectrequest_solace[`$first params`topic;`$first params`data;5000i;"";""]
replyType:type reply
if[replyType=4h;0N!"Got reply with contents:",`char$reply]
if[replyType=-6h;0N!"Request failed with code :";0N!reply]
.solace.destroy_solace[1i]

exit 0
