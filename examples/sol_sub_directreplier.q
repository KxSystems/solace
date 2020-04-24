-1"### Enter '\\\\' to exit\n";

\l sol_init.q

-1"### Registering topic message callback";
subUpdate:{[dest;payload;dict]
 -1"### Message received";
 show(`payload`dest!("c"$payload;dest)),dict;
 $[dict`isRequest;"x"$"reply contents";0i]
 }
.solace.setTopicMsgCallback`subUpdate;

-1"### Subscribing to topic : ",string params`topic;
.solace.subscribeTopic[params`topic;1b];
