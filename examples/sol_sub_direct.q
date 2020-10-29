-1"### Enter '\\\\' to exit\n";

\l sol_init.q

-1"### Registering topic message callback";
subUpdate:{[dest;msg;dict]
 -1"### Message received";
 payload:.solace.getPayloadAsBinary[msg];
 show(`payload`dest!("c"$payload;dest)),dict;
 }
.solace.setTopicRawMsgCallback`subUpdate;

-1"### Subscribing to topic : ",string params`topic;
.solace.subscribeTopic[params`topic;1b];
