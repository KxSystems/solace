\l examples/sol_init.q

.solace.endpointTopicSubscribe[;2i;params`topic]`ENDPOINT_ID`ENDPOINT_NAME!(`2;params`queue);

exit 0
