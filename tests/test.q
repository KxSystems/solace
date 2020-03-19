solace:`$"/home/sshanks/243test/csource/build/build/lib/libdeltasolace"
.solace.init_solace:solace 2:(`init_solace;4)
.solace.iscapable_solace:solace 2:(`iscapable_solace;1)
.solace.getcapability_solace:solace 2:(`getcapability_solace;1)
.solace.createendpoint_solace:solace 2:(`createendpoint_solace;2)
.solace.destroyendpoint_solace:solace 2:(`destroyendpoint_solace;2)
.solace.send_solace:solace 2:(`send_solace;2)
.solace.sendpersistentqueue_solace:solace 2:(`sendpersistentqueue_solace;2)
.solace.destroy_solace:solace 2:(`destroy_solace;1)
.solace.init_solace[`10.152.20.167;`default;`user;`password]
n:20;
t:([]sym:20?`a`b`c`d`e`f`g;price:n?20f;volume:n?100j);
getJSRow:{[t;index] .j.j select from t where i=index};
.solace.iscapable_solace[`SESSION_CAPABILITY_PUB_GUARANTEED]
.solace.getcapability_solace[`SESSION_PEER_PLATFORM]
.solace.getcapability_solace[`SESSION_PEER_SOFTWARE_VERSION]
.solace.createendpoint_solace[1i;`$"Q/tutorial"]
.solace.send_solace[`tradetopic;getJSRow[t;10]]
.solace.sendpersistentqueue_solace[`$"Q/tutorial";getJSRow[t;10]]
.solace.destroy_solace[`test]

