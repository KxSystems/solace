\l examples/sol_init.q

// sending a direct message to topic requiring a reply
reply:.solace.sendDirectRequest[params`topic;params`data;5000i;"";""]

if[4<>type reply;
 -2"### Request failed with code : ",string reply;
 exit 1;
 ]

-1"### Reply received : ","c"$reply;

exit 0
