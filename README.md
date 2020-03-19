# kdb+ interface for Solace Messaging

## Introduction

TODO - what is that, whats required, drawing of potential use case

This API permits the use of Solace Messaging with KDB+. It also details the use of Solace REST messaging to create publishers and consumers.

## Building

TODO - linux/docker build - TODO windows/mac/etc support, tools required

Download the Solace C Messaging API ( https://solace.com/downloads/ ).
Set an environment variable to the location of the unzipped Solace C API.
For example, on Linux/Mac
```
export SOLACE_API=/home/myuser/solaceapi/
cmake source
make
```


## Running

### Solace PubSub+ Event Broker 

In order to pub/sub with Solace, you'll need to be connected to a [PubSub+ event broker](https://solace.com/products/event-broker/). 

If you don't already have an event broker running, you can avail of a free standard edition [docker based broker](https://github.com/SolaceLabs/solace-single-docker-compose) or a [cloud based instance](https://console.solace.cloud/login/new-account) (NOTE: other versions and enterprise solutions also available).

### KDB+ Solace Integration

#### Installation

TODO - running (e.g. library path/etc)

#### Examples

See contents of the [examples](examples/README.md) folder

## API Doc for Solace Messaging

### Event Notification

```
K setsessioncallback_solace(K callbackFunction);
```

Associates the provided function with session events such as connection notifications or session errors (e.g. too many subscriptions, login failure, etc)

The callback function should take 3 parameters

- eventType: int type. Possible values listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#a6b74c1842f26be7bc10f6f4dd6995388) e.g. 0 is SOLCLIENT_SESSION_EVENT_UP_NOTICE
- responseCode: int type. Response code that is returned for some events, otherwise zero.
- eventInfo: string type. Further information about the event

```
K setflowcallback_solace(K callbackFunction);
```

Associates the provided function with flow events.

The callback function should take 5 parameters

- eventType: int type. Possible values listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#a992799734a2bbe3dd8506ef332e5991f). e.g. 0 is SOLCLIENT_FLOW_EVENT_UP_NOTICE
- responseCode: int type. Response code that is returned for some events, otherwise zero.
- eventInfo: string type. Further information about the event
- destType: int type. Possible values listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#aa7d0b19a8c8e8fbec442272f3e05b485). e.g. 0 is SOLCLIENT_NULL_DESTINATION
- destName: string type. Name of the destination

### Connecting

```
K init_solace(K options);
```

Consumes a dictionary of options, consisting of symbol to symbol mappings. A list of possible session properties are listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html). Common properties are SESSION_HOST,  SESSION_VPN_NAME, SESSION_USERNAME, SESSION_PASSWORD, SESSION_RECONNECT_RETRIES.

Connects and creates a session.

*You must be connected before running any subsequent solace functions.*

### Disconnecting

```
K destroy_solace(K a);
```

Destroys the previously created session.

### Utility Functions

```
K iscapable_solace(K capabilityName);
```

Requires a capability as a symbol/string, from the list of features listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#sessioncapabilities). Returns a boolean on whether that capability is enabled for the current session.

```
K getcapability_solace(K capabilityName);
```

Requires a capability as a symbol/string, from the list of features that your solace environment may provide as listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#sessioncapabilities) e.g. version information, whether you can create temporary endpoints, etc..

The returned type may be symbol, int, bool, etc, depending on the type of capability requested e.g. requesting SESSION_PEER_SOFTWARE_VERSION will return a symbol data type.

### EndPoint Management

These functions may be used to create or destroy endpoints from the KDB+ application. In some deployments, end points may already be created for you by an admin.
*Endpoint management must be enabled for the user in order to use this functionality*

```
K createendpoint_solace(K options,K provFlags);
```

Provision, on the appliance, a durable Queue or Topic Endpoint using the current Session

Options type should be a dictionary of symbols. The names use the Solace endpoint property names e.g. ENDPOINT_NAME (Ref: https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#endpointProps ).
The provFlags param (type int) indicates the provision flags used by solace (Ref: https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#provisionflags )

```
K destroyendpoint_solace(K options,K provFlags);
```

Destroys endpoints. Reference createendpoint_solace. 

```
K endpointtopicsubscribe_solace(K options, K provFlags, K topic);
```

Add a Topic subscription to an existing endpoint. Can be added to queues or remote clients. 

```
K endpointtopicunsubscribe_solace(K options, K provFlags, K topic);
```

Removes a Topic subscription to an existing endpoint. Can be used with queues or remote clients. 

### Direct Messaging

```
K senddirect_solace(K topic, K data);
```

Used for sending direct messages (Ref: https://docs.solace.com/PubSub-Basics/Direct-Messages.htm ). Each message will automatically be populated with a sender timestamp, message eliding eligibility enabled and dead message queue (DMQ) eligibility enabled.

- topic: Topic to sending the message. Symbol type.
- data: string/symbol/byte data, which forms the basis of the payload for the message

#### Persistent Messaging

```
K sendpersistent_solace(K type, K dest, K replyType, K replydest, K data, K correlationId);
```

Used for sending persistent messages. Will persist until acknowledged (see sendack_solace) or conditions met (see [Solace docs](https://docs.solace.com/PubSub-Basics/Guaranteed-Messages.htm)) 

- type: Type should be an int, 0 for topic, 1 for queue. 
- dest: Destination name of the queue/topic. Symbol type.
- replyType<optional> : can be NULL, otherwise should be an int, 0 for topic, 1 for queue. The type of the reply destination that can be used to reply to this message
- replyDest<optional>: can be NULL. Destination name of the queue/topic that can be used to reply to this message. Can be string/symbol.
- data: string/symbol/byte data, which forms the basis of the payload for the message
- correlationId<optional>: can be NULL. Correlation Id is carried in the Solace message headers unmodified by the appliance and may be used for peer-to-peer message synchronization

```
K subscribepersistent_solace(K type, K endpointname, K topicname, K callbackFunction);
```

Type should be an int, 0 for topic, 1 for queue. 

The endpoint symbol specifies the queue/topic endpoint that is the target of the bind.

The topicname symbol is optional. When binding to a Topic endpoint, the Topic may be set in the bind. 

The callbackFuction should be a q function that will be called when any message is send to that endpoint. The callbackFunction signature takes a single parameter, that will be a dictionary with message keys:

subsType,topicName,replyType,replyName,correlationId,flowPtr,msgId,stringData

Each value in the dictionary consists of a list of values (more than 1 message can be received on a callback)

```
K subscribetmp_solace(K type, K callbackFunction);
```

Creates and subscribes to a temp queue or topic. These do not remain when the client session ends. Only this client will be a consumer of the endpoint. Can typically be used as a reply-to endpoint for client communications.

Type should be an int, 0 for topic, 1 for queue. 

Reference subscribepersistent_solace for details on the callbackFunction

The function will return the name of the newly created endpoint (which can then be used in other functions)

```
K sendack_solace(K flow, K msgid);
```

This should be called by the subscriptions callbackFunction to acknowledge the message has been processed, in order to prevent the message from being consuming on a subsequent subscription.

The parameters required can be found in the input to the callbackFunction on the subscription.

```
K unsubscribepersistent_solace(K type, K endpointname, K topicname);
```

Removes subscription created via subscribepersistent_solace.

## API doc for REST messaging for Solace

This is an alternative to messaging using the solace C API, for low rate data. Ref: https://docs.solace.com/Open-APIs-Protocols/REST-get-start.htm

### REST Publishing to Solace

Please view the KX documentation for version support of the **.Q.hp** function, in order to sent HTTP posts. The Solace documentation should be reference for HTTP params and how to test with curl/etc prior to running the KDB+ example in order to test connectivity.

Example of q code to publish the string 'hello world' to Solace queue called 'KDB_QUEUE'

```
.Q.hp["http://localhost:9000/QUEUE/KDB_QUEUE";.h.ty`text]"hello world"
```

Example of q code to publish the string 'hello world' to Solace topic called 'Q/topic' via direct messaging

```
.Q.hp["http://localhost:9000/TOPIC/Q/test";.h.ty`text]"hello world"
```

### REST Subscription

Please view the KX documentation for version support of the **.z.pp** function, in order to receive HTTP posts. You can configure the Solace framework to publish to the KDB+ instance using its IP/port, remembering to preface any 'Post Request Target' with a '/' character.

This basic example return a HTTP 200 response on receipt of a message & prints out the payload. Running 'q -p 12341' and configuring a Solace REST Consumer using your host/port, with a queue binding will allow messages on the queue to be received in KDB+ (view Solace documentation on configuring REST consumers)
```
.z.pp:{[x] 0N!((first where x[0]=" ")+1)_x[0];r:.h.hn["200 OK";`txt;""];r}
```
It is possible to configure multiple REST endpoints in KDB+, with data tranformations & callbacks.

## References

TODO

## Unsupported Functionality

Currently transactional based messaging is unsupported.