\l sol_init.q

-1"### Sending persistent message";
.solace.sendPersistent["i"$`queue=params`dtype;params`dest;params`data;params`corr];

exit 0
