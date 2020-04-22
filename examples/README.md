# kdb+ interface for Solace (Examples)

## Installation

Add C solace client library to your LD_LIBRARY_PATH environment variable.

Place the libkdbsolace.so (from your release) into the same directory as your 'q' executable.
Place solace.q into your KDB+ installation directory.

## Running Examples

### Utilities

#### sol_capabilities.q

Requests capability value settings from the Solace broker 

Example (whether endpoint creation/deletion is allowed by user) 

```C
q sol_capabilities.q -opt SESSION_CAPABILITY_ENDPOINT_MANAGEMENT
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -opt - Solace capability name (possible values listed [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#sessioncapabilities))

### EndPoint Interaction

Creates an queue on the Solace broker (permission permitting)

#### sol_endpoint_create.q

Example: 

```C
q sol_endpoint_create.q -name "Q/test" 
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -name - name of the endpoint to be created

#### sol_endpoint_destroy.q

Removes a previously created endpoint on the Solace broker (permission permitting)

Example: 

```C
q sol_endpoint_destroy.q -name "Q/test"
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -name - name of the endpoint to be created

#### sol_topic_to_queue_mapping.q

Add a topic subscription to an existing endpoint queue (permission permitting)

Example:

```C
q sol_topic_to_queue_mapping.q -queue "Q/test" -topic "Q/topic"
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -queue - name of the exiting queue endpoint to alter
- -topic - name of the topic to add to the existing queue

### Pub/Sub With Direct Msgs 

#### sol_pub_direct.q

Sends a direct message via a topic (also has an example of using session properties to enable send timestamps on each msg). Can be used with sol_sub_direct.q example or any solace example program.

Example:

```c
q sol_pub_direct.q -topic "Q/1" -data "hello world" 
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -topic - topic name to publish the message to
- -data - message payload to send

#### sol_sub_direct.q

Subscribes to a topic for direct msgs 

Example:

```c
q sol_sub_direct.q -host 192.168.65.2:55111 -topic "Q/>"
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -topic - topic name to subscribe to (Solace wildcard format supported)

#### sol_pub_directrequestor.q

As per sol_pub_direct.q, but adds a request for a reply as part of the published message. Can be used with sol_sub_directreplier.q

#### sol_sub_directreplier.q

As per sol_sub_direct.q, but replies to any message received

### Pub/Sub With Guaranteed Msgs


#### sol_pub_persist.q

Sends a persistent/guaranteed message to an existing endpoint (see sol_endpoint_create.q)

Example:

```c
q sol_pub_persist.q -desttype "queue" -destname "Q/1" -data "hello world"  -correlationid 555
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -data - message payload to send
- -desttype - (optional) type of the destination (can be 'queue' or 'topic'), defaults to queue
- -destname - (optional) name of the endpoint to be created
- -correlationid - (optional) correlation id

#### sol_sub_persist.q

Subscribes, while printing and acknowledging each message

Example:

```C
q sol_sub_persist.q -destname "Q/1"
```

Params:

- -host - Broker hostname
- -vpn - VPN name
- -user - username
- -pass - password
- -destname - (optional) name of the endpoint queue to be used
