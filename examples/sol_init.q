\l ../q/solace.q
\c 25 2000

default.host :"host.docker.internal"
default.vpn  :"default"
default.user :"admin"
default.pass :"admin"
default.opt  :"SESSION_PEER_PLATFORM"
default.name :"Q/test"
default.topic:"Q/test"
default.queue:"Q/test"
default.dest :"Q/test"
default.dtype:"queue"
default.data :"kdb data"
default.corr :""
default.trust:"cert"

params:.Q.def[`$1_default].Q.opt .z.x

-1"### Registering session event callback";
sessionUpdate:{[eventType;responseCode;eventInfo]
 -1"### Session event";
 show`eventType`responseCode`eventInfo!(`int$eventType;responseCode;eventInfo);
 }
.solace.setSessionCallback`sessionUpdate;

-1"### Registering flow event callback";
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]
 -1"### Flow event";
 show`eventType`responseCode`eventInfo`destType`destName!("i"$eventType;responseCode;eventInfo;destType;destName);
 }
.solace.setFlowCallback`flowUpdate;

-1"### Initializing session";
show initparams:params`SESSION_HOST`SESSION_VPN_NAME`SESSION_USERNAME`SESSION_PASSWORD`SESSION_SSL_TRUST_STORE_DIR!`host`vpn`user`pass`trust;
if[0>.solace.init initparams;-2"### Initialization failed";exit 1];

.z.exit:{-1"### Destroying session";.solace.destroy[];}
