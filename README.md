
# kdb+ interface for Solace PubSub+ broker

## Introduction

This API permits the use of interacting with Solace PubSub+ broker with KDB+. It also details the use of Solace's RESTful API to create publishers and consumers.

## New to KDB+ ?

kdb+ is the world's fastest time-series database. Kdb+ is optimized for ingesting, analyzing, and storing massive amounts of structured data. To access the free editions of kdb+, please visit https://code.kx.com/q/learn/ for downloads and developer information. For general information, visit https://kx.com/

## New to Solace PubSub+ ?

Solace PubSub+ Event Broker efficiently streams events and information  
across cloud, on-premises and IoT environments. The “+” in PubSub+ means it supports a wide range of message exchange patterns beyond publish/subscribe, including request/reply, streaming and replay, as well as different qualities of service, such as best effort and guaranteed delivery.

You can get started quickly by using free Standard Edition of our [software broker](https://solace.com/products/event-broker/software/) or spin up a free instance on [Solace Cloud](https://console.solace.cloud/login/new-account).

If you have any Solace related questions, you can raise them at [Solace Community](https://solace.community/).

## Building Interface From Source

Building the interface from source requires `gcc`, `gcc c++`, `make` and `cmake` packages installed on your development machine.

Follow these steps:

 1. Download the Solace C Messaging API ( https://solace.com/downloads/ ).
 2. Set an environment variable `SOLACE_API_DIR` to the location of the unzipped Solace C API.
 3. Run `cmake`
 4. Run `cmake install`


For example, on Linux/Mac

```
export SOLACE_API_DIR=/home/myuser/solaceapi/
cd build
cmake ../
make install
```

You should then find the resulting files in a newly created api dir, that should reside within the build directory.

## Running

### Solace PubSub+ Event Broker

In order to pub/sub with Solace, you'll need to be connected to a [PubSub+ event broker](https://solace.com/products/event-broker/).

If you don't already have an event broker running, you can avail of a free standard edition [docker based broker](https://github.com/SolaceLabs/solace-single-docker-compose) or a [free cloud based instance](https://console.solace.cloud/login/new-account) (NOTE: other versions and enterprise solutions are also available).

### KDB+ Solace Integration

#### Installation
Here are the steps for installation:

 1. Download Solace C API from [here](https://solace.com/downloads/)(please select for your relevant machine type).
 2. Unzip the Solace API to a directory on the machine which the end user can read from.
 3. Add the lib directory to the `LD_LIBRARY_PATH` environment variable e.g. if unzipped to `/usr/local/solaceapi/`
	 - `export LD_LIBRARY_PATH /usr/local/solaceapi/lib/:$LD_LIBRARY_PATH`
 4.  Copy `libkdbsolace.so` which was built earlier (see earlier section: *Building Interface from Source*) to your KDB+ folder. If on a Linux 64bit machine with KDB+ installed in `/usr/local/q`, place the shared library into `/usr/local/q/l64/`.

The q script to load the solace API (`solace.q`) can be placed in the current working directory or within the KDB+ install directory.

See examples and API documentation on how to tailor the interface for your needs.

#### Examples

See contents of the [examples](examples/) folder and its associated [README.md](examples/README.md) for further details.

## API Doc for Solace Messaging

  - [Event Notification](#event-notification)
      * [.solace.setSessionCallback](#solacesetsessioncallback)
      * [.solace.setFlowCallback](#solacesetflowcallback)
  - [Connecting](#connecting)
      * [.solace.init](#solaceinit)
  - [Disconnecting](#disconnecting)
      * [.solace.destroy](#solacedestroy)
  - [Utility Functions](#utility-functions)
      * [.solace.version](#solaceversion)
      * [.solace.iscapable](#solaceiscapable)
      * [.solace.getCapability](#solacegetcapability)
  - [EndPoint Management](#endpoint-management)
      * [.solace.createEndpoint](#solacecreateendpoint)
      * [.solace.destroyEndpoint](#solacedestroyendpoint)
      * [.solace.endpointTopicSubscribe](#solaceendpointtopicsubscribe)
      * [.solace.endpointTopicUnsubscribe](#solaceendpointTopicUnsubscribe)
  - [Direct Messaging](#direct-messaging)
      * [.solace.sendDirect](#solacesenddirect)
      * [.solace.sendDirectRequest](#solacesenddirectrequest)
  - [Topic Subscriptions](#topic-subscriptions)
      * [.solace.setTopicMsgCallback](#solacesettopicmsgcallback)
      * [.solace.subscribeTopic](#solacesubscribetopic)
      * [.solace.unSubscribeTopic](#solaceunsubscribetopic)
  - [Persistent/Guaranteed Message Publishing](#persistent-guaranteed-message-publishing)
      * [.solace.sendPersistent](#solacesendpersistent)
      * [.solace.sendPersistentRequest](#solacesendpersistentrequest)
  - [Flow Binding](#flow-binding)
      * [.solace.setQueueMsgCallback](#solacesetqueuemsgcallback)
      * [.solace.bindQueue](#solacebindqueue)
      * [.solace.sendAck](#solacesendack)
      * [.solace.unBindQueue](#solaceunbindqueue)

### Event Notification

#### .solace.setSessionCallback
```
.solace.setSessionCallback[callbackFunction]
```
Associates the provided function with session events such as connection notifications or session errors (e.g. too many subscriptions, login failure, etc)

The callback function should take 3 parameters

- `eventType`: int type. Possible values listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#a6b74c1842f26be7bc10f6f4dd6995388) e.g. 0 is `SOLCLIENT_SESSION_EVENT_UP_NOTICE`
- `responseCode`: int type. Response code that is returned for some events, otherwise zero.
- `eventInfo`: string type. Further information about the event

#### .solace.setFlowCallback
```
.solace.setFlowCallback[callbackFunction]
```

Associates the provided function with flow events.

The callback function should take 5 parameters:
- `eventType`: int type. Possible values listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#a992799734a2bbe3dd8506ef332e5991f). e.g. 0 is `SOLCLIENT_FLOW_EVENT_UP_NOTICE`
- `responseCode`: int type. Response code that is returned for some events, otherwise zero.
- `eventInfo`: string type. Further information about the event
- `destType`: int type. Possible values listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#aa7d0b19a8c8e8fbec442272f3e05b485). e.g. 0 is `SOLCLIENT_NULL_DESTINATION`
- `destName`: string type. Name of the destination

### Connecting
#### .solace.init
```
.solace.init[options]
```
Connects and creates a session.

Consumes a dictionary of options, consisting of symbol to symbol mappings. A list of possible session properties are listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html). Common properties are `SESSION_HOST`,  `SESSION_VPN_NAME`, `SESSION_USERNAME`, `SESSION_PASSWORD`, `SESSION_RECONNECT_RETRIES`.

*You must be connected before running any subsequent solace functions.*

### Disconnecting

#### .solace.destroy

```
.solace.destroy[unused]
```

Destroys the previously created session.

### Utility Functions
#### .solace.version
```
.solace.version[unused]
```
Returns a dictionary of Solace API version info. Useful for checking current deployment/build versions.

#### .solace.iscapable
```
.solace.iscapable[capabilityName]
```

Requires a capability as a symbol/string, from the list of features listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#sessioncapabilities). Returns a boolean on whether that capability is enabled for the current session.

#### .solace.getCapability
```
.solace.getCapability[capabilityName]
```

Requires a capability as a symbol/string, from the list of features that your solace environment may provide as listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#sessioncapabilities) e.g. version information, whether you can create temporary endpoints, etc..

The returned type may be symbol, int, bool, etc, depending on the type of capability requested e.g. requesting `SESSION_PEER_SOFTWARE_VERSION` will return a symbol data type.

### EndPoint Management

These functions may be used to create or destroy endpoints from the KDB+ application. In some deployments, endpoints may already be created for you by an admin.
*NOTE: Endpoint management must be enabled for the user in order to use this functionality*

#### .solace.createEndpoint

```
.solace.createEndpoint[options;provFlags]
```
Provision an endpoint on the appliance, a durable Queue or Topic Endpoint using the current Session.

Options type should be a dictionary of symbols. The names use the Solace endpoint property names e.g. `ENDPOINT_NAME` (Ref: https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#endpointProps ).
The `provFlags` param (type int) indicates the provision flags used by solace (Ref: https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#provisionflags )

#### .solace.destroyEndpoint

```
.solace.destroyEndpoint[options;provFlags]
```

Destroys endpoints. Reference `createEndpoint`.

#### .solace.endpointTopicSubscribe

```
.solace.endpointTopicSubscribe[options;provFlags;topic]
```

Add a topic subscription to an existing endpoint. Can be added to queues or remote clients.

#### .solace.endpointTopicUnsubscribe

```
.solace.endpointTopicUnsubscribe[options;provFlags;topic]
```

Removes a topic subscription from an existing endpoint. Can be used with queues or remote clients.

### Direct Messaging

####  .solace.sendDirect
```
.solace.sendDirect[topic;data]
```
This function can be used to send [direct messages](https://docs.solace.com/PubSub-Basics/Direct-Messages.htm). Each message will automatically be populated with message eliding eligibility enabled and dead message queue (DMQ) eligibility enabled.

The parameters are:
- `topic`: Topic to sending the message. String type.
- `data`: string/symbol/byte data, which forms the basis of the payload for the message

#### .solace.sendDirectRequest

```
 .solace.sendDirectRequest[topic;data;timeout;replyType;replydest]
```

This function can be used to send direct messages that require a sync reply. Returns a byte list of message received, containing the payload. Otherwise will be an `int` to indicate the return code. If value is `7`, the reply wasn't received.

Here are the parameters:
- `topic`: Topic to sending the message. String type.
- `data`: string/symbol/byte data, which forms the basis of the payload for the message
- `timeout`:  indicates the milliseconds to block/wait (must be greater than zero). Int type.
- `replyType`:  -1 for null, 0 for topic, 1 for queue, 2 for temp topic, 3 for tmp queue. The topic/queue that you wish a reply to this message to go to. Should be an int.
- `replyDest`: The topic/queue that you wish a reply to this message to go to (empty for default session topic). Should be a symbol.

### Topic Subscriptions

#### .solace.setTopicMsgCallback
```
.solace.setTopicMsgCallback[cb]
```
This is a callback function for messages received from topic subscriptions. It registers a q function that should be called on receipt of messages from topic subscriptions. If the dict contains a value of `true` for the key `isRequest`, the function should return with the response message contents (type byte list) as this is an indication that the sender is requesting a reply.

The parameters are:
* `cb`: A q function that takes 3 parameters. The function should accept 3 parameters: symbol destination, byte list for payload and a dictionary of msg values. The dictionary consists of:
  * `isRedeliv`: boolean thats states the redelivered status
  * `isDiscard`: Returns true if one or more messages have been discarded prior to the current message, otherwise it returns false. This indicates congestion discards only, and is not affected by message eliding
  * `isRequest`: boolean that states if the client is expecting a reply (in which case the function should return a byte array for the response)
  * `sendTime`: timestamp of the client's send time (if populated)

#### .solace.subscribeTopic
```
.solace.subscribeTopic[topic;isBlocking]
```
This function can be used to subscribe to a topic. Solace format wildcards (*, >) can be used in the topic subscription value.

The parameters are:
* `topic`: Topic to subscribe to. String type.
* `isBlocking`: True to block until confirm or true to get session event callback on sub activation

#### .solace.unSubscribeTopic

```
.solace.unSubscribeTopic[topic]
```
This function can be used to unsubscribe from an existing topic subscription.

### Persistent/Guaranteed Message Publishing

#### .solace.sendPersistent

```
.solace.sendPersistent[destType;dest;data;correlationId]
```

Used for sending persistent messages onto a queue or topic.

The parameters are:
- `destType`: should be an int, -1 for null, 0 for topic, 1 for queue, 2 for temp topic, 3 for tmp queue.
- `dest`: Destination name of the queue/topic. Symbol type.
- `data`: byte array/string/symbol data, which forms the basis of the payload for the message
- `correlationId`<optional>: can be NULL. Correlation Id is carried in the Solace message headers unmodified by the appliance and may be used for peer-to-peer message synchronization

#### .solace.sendPersistentRequest
```
.solace.sendPersistentRequest[destType;dest;data;timeout;replyType;replydest]
```
This function can be used for sending guaranteed messages that requie a synchronous reply. It returns a byte list of message received, containing the payload. Otherwise, an int will be returned to indicate the return code. If the value is 7, the reply wasn't received.

The parameters are:
- `destType`: Int type, -1 for null, 0 for topic, 1 for queue, 2 for temp topic, 3 for tmp queue.
- `dest`: Destination name of the queue/topic. Symbol type.
- `data`: byte array/string/symbol data array, which forms the basis of the payload for the message
- `timeout`: Integer timeout param that indicate the millisecons to block/wait (must be greater than zero).
- `replyType`: Integer type. -1 for null, 0 for topic, 1 for queue, 2 for temp topic, 3 for tmp queue. The topic/queue that you wish a reply to this message to go to
- `replyDest`: Symbol type. The topic/queue that you wish a reply to this message to go to (empty for default session topic)

### Flow Binding

#### .solace.setQueueMsgCallback
```
.solace.setQueueMsgCallback[callbackFunction]
```
This function sets a callback function (`callbackFunction`) which should be a q function. This function will be called when any message is sent to that endpoint.

The `callbackFunction` takes three parameters:
- first is a symbol of the flow destination (i.e. the origin queue binded to from which this subscription originated)
- second is byte array for the payload
- third is a dictionary with keys:
  - `destType`: Int type. Message destination type, -1 for null, 0 for topic, 1 for queue, 2 for temp topic, 3 for tmp queue.
  - `destName`: String type. Message destination name (can be differen from flow dest, e.g. when message was sent to a topic but mapped to a queue from where it was received)
  - `replyType`: Int type. Reply destination type, -1 for null, 0 for topic, 1 for queue, 2 for temp topic, 3 for tmp queue.
  - `replyName`: String type. Reply destination name
  - `correlationId`: String type. Original messages correlationId
  - `msgId`: Long type. Used for sending acks (see `.solace.sendAck`)

#### .solace.bindQueue

```
.solace.bindQueue[bindProps]
```
This function allows you to bind to a queue.
The `bindProps` value defines a dictionary of symbol for Solace supported bind properties & values. For example: ``(`FLOW_BIND_BLOCKING;`FLOW_BIND_ENTITY_ID;`FLOW_ACKMODE;`FLOW_BIND_NAME)!(`1;`2;`1;`MYQUEUE)``

You can find possible flow binding properties [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#flowProps) along with the default settings.

#### .solace.sendAck

```
.solace.sendAck[endpointname;msgid]
```

This function allows you to acknowledge messages. It should be called by the subscriptions `callbackFunction` to acknowledge that the message has been processed, in order to prevent the message from being consumed on a subsequent subscription.

The parameters required can be found in the input to the `callbackFunction` on the subscription.

NOTE: only required when you wish to take control and run with auto acks off (e.g. `FLOW_ACKMODE` disabled in the flow binding).

#### .solace.unBindQueue

```
.solace.unBindQueue[endpointname]
```

Removes subscription/binding created via `.solace.bindQueue`.

## API doc for REST messaging for Solace

This is an alternative to messaging using the solace C API, for low rate data. Ref: https://docs.solace.com/Open-APIs-Protocols/REST-get-start.htm

### REST Publishing to Solace

Please view the KX documentation for version support of the **.Q.hp** function, in order to send HTTP posts. The Solace documentation should be reference for HTTP params and how to test with curl/etc prior to running the KDB+ example in order to test connectivity.

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

Solace - https://solace.com/
Solace Docs - https://docs.solace.com/
Solace Downloads - https://solace.com/downloads/
Solace Community - https://solace.community/

## Unsupported Functionality

Currently transactional based messaging is unsupported.
