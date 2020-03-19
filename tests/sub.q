testSimon:{[subsType;topicName;stringData]0N!r:enlist each (`int$subsType;topicName;stringData);r}
solace:`$"/home/sshanks/243test/csource/build/build/lib/libdeltasolace"
.solace.init_solace:solace 2:(`init_solace;4)
.solace.iscapable_solace:solace 2:(`iscapable_solace;1)
.solace.getcapability_solace:solace 2:(`getcapability_solace;1)
.solace.createendpoint_solace:solace 2:(`createendpoint_solace;2)
.solace.destroyendpoint_solace:solace 2:(`destroyendpoint_solace;2)
.solace.subscribepersistent_solace:solace 2:(`subscribepersistent_solace;4)
.solace.sendpersistent_solace:solace 2:(`sendpersistent_solace;3)
.solace.unsubscribepersistent_solace:solace 2:(`unsubscribepersistent_solace;3)
.solace.destroy_solace:solace 2:(`destroy_solace;1)
.solace.init_solace[`10.152.20.167;`default;`user;`password]
.solace.iscapable_solace[`SESSION_CAPABILITY_PUB_GUARANTEED]
.solace.getcapability_solace[`SESSION_PEER_PLATFORM]
.solace.getcapability_solace[`SESSION_PEER_SOFTWARE_VERSION]
.solace.subscribepersistent_solace[0i;`$"my_sample_topicendpoint";`$"my/sample/topic";`$"testSimon"]
.solace.subscribepersistent_solace[1i;`$"Q/tutorial";`$"";`$"testSimon"]
.solace.sendpersistent_solace[1i;`$"Q/tutorial";`$"test data content for queue"]
.solace.sendpersistent_solace[0i;`$"my/sample/topic";`$"test data content for topic"]
.solace.sendpersistent_solace[1i;`$"Q/tutorial";`$"test data content for queue"]
.solace.sendpersistent_solace[0i;`$"my/sample/topic";`$"test data content for topic"]

