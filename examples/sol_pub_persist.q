default_nm:`host`vpn`user`pass`destname`type`data`correlationid`replytype`replydestname
default_val:(enlist "host.docker.internal";enlist "default";enlist "admin";enlist "admin";enlist "Q/test";enlist "queue";enlist "kdb data";enlist "";enlist "";enlist "")
params:.Q.def[default_nm!default_val].Q.opt .z.x

soloptions:`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD!(`$first params`host;`$first params`vpn;`$first params`user;`$first params`pass);

/ initialise the solace api function
.solace.setsessioncallback_solace:`libdeltasolace 2:(`setsessioncallback_solace;1)
.solace.setflowcallback_solace:`libdeltasolace 2:(`setflowcallback_solace;1)
.solace.init_solace:`libdeltasolace 2:(`init_solace;1)
.solace.sendpersistent_solace:`libdeltasolace 2:(`sendpersistent_solace;6)
.solace.destroy_solace:`libdeltasolace 2:(`destroy_solace;1)

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

/ perform solace actions
q:`$"queue"
.solace.endpoint_type:`int$(q=`$first params`type)
.solace.reply_type:`int$(q=`$first params`replytype)

.solace.init_solace[soloptions]

/ sending a persistent message to a queue
.solace.sendpersistent_solace[.solace.endpoint_type;`$first params`destname;.solace.reply_type;`$first params`replydestname;`$first params`data;`$first params`correlationid]

.solace.destroy_solace[1i]

exit 0
