\l sol_init.q

-1"### Sending message";
.solace.sendDirect . 0N!params`topic`data;

exit 0
