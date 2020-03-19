\l tests/solace.q

d:.Q.opt .z.x;
.solace.sessionProps:@[d;key d;first `$];
.solace.default:(k:`SESSION_VPN_NAME`SESSION_HOST`SESSION_USERNAME`SESSION_PASSWORD)!(`default;`$"10.152.20.156";`admin;`admin);
.solace.sessionProps:k#.solace.default,.solace.sessionProps; 

/ setup session event callbacks
sessionUpdate:{[eventType;responseCode;eventInfo]0N!r:enlist each (`int$eventType;responseCode;eventInfo);0N!("SESSION EVENT: ####";r);r};
.solace.setsessioncallback_solace[`sessionUpdate];

/ setup flow event callbacks
flowUpdate:{[eventType;responseCode;eventInfo;destType;destName]0N!r:enlist each (`int$eventType;responseCode;eventInfo;destType;destName);0N!("FLOW EVENT: ####";r);r};
.solace.setflowcallback_solace[`flowUpdate];

/connect to Solace 3
show "Attempting solace connection";
/ init function return codes.  Logic needs to check for success/failure
.solace.initResult:.solace.init_solace[.solace.sessionProps];
if[not .solace.connected:(.solace.initResult in where `SOLCLIENT_OK = .solace.returnCodes);
  show "Error connecting to solace: ",string .solace.returnCodes[.solace.initResult]
 ];

show `$"SESSION_CAPABILITY_PUB_GUARANTEED=",string .solace.getcapability_solace(`SESSION_CAPABILITY_PUB_GUARANTEED);
\\
