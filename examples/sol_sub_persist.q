-1"### Enter '\\\\' to exit\n";

\l sol_init.q

-1"### Registering queue message callback";
subUpdate:{[dest;payload;dict]
 -1"### Message received";
 show(`payload`dest!("c"$payload;dest)),dict;
 .solace.sendAck[dest;dict`msgId];
 }
.solace.setQueueMsgCallback`subUpdate;

.solace.bindQueue`FLOW_BIND_BLOCKING`FLOW_BIND_ENTITY_ID`FLOW_ACKMODE`FLOW_BIND_NAME!`1`2`2,params`dest;
