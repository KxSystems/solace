# kdb+ Solace Example

Follow the [installation instructions](install.md) for set-up.

If you don't already have an event broker running, you can avail of a free standard edition [docker-based broker](https://github.com/SolaceLabs/solace-single-docker-compose) or a [free cloud based instance](https://console.solace.cloud/login/new-account) (other versions and enterprise solutions are also available). 

## Using Solace Cloud

### Requirements

- An account on [Solace Cloud](https://console.solace.cloud/)

### Testing a connection to a service

Create a new service under the cluster manager e.g. 'testservice'. Select the newly created service, and visit the 'Connect' page, followed by 'Solace Messaging'. Download the pem file to a new directory & note all the connection details. Run the following (replacing the params with your connection info & changing 'cert' to the directory that contains your pem file). It should connect to the broker and ask the name of the platform e.g. 'Solace PubSub+ Enterprise'

```bash
solace]$ cd examples
examples]$ q sol_capabilities.q -host tcps://mr906gris.messaging.solace.cloud:55443 -user solace-cloud-client -pass ggut5g2v84pegdibs -vpn testservice -trust cert
```

## Using Docker-based broker

### Requirements

- [Solace PubSub+ Event Broker](https://solace.com/products/event-broker/)


### Start Event Broker

**Note:** Ensure that Docker is running.

```bash
]$ git clone https://github.com/SolaceLabs/solace-single-docker-compose.git
]$ cd solace-single-docker-compose/template
template]$ docker-compose -f PubSubStandard_singleNode.yml up -d
```

Please view the docs on https://github.com/SolaceLabs/solace-single-docker-compose to find out how to use the web interface & configure the SMF port.

### Launch subscriber

A mock subscriber is provided in `examples/` folder of the repository. We assume that you are in the cloned source repository.

The subscriber subscribes to a topic `Q` with a wildcard: `Q/>` connecting to a Solace host 192.168.65.2 (configured to be a SMF host on port 55111)

```bash
solace]$ cd examples
examples]$ q sol_sub_direct.q -host tcp://192.168.65.2:55111 -topic "Q/>"
```

### Launch publisher

Open another console and launch a publisher to send a message `"hello world"` via topic `Q/1`.

```bash
examples]$ q sol_pub_direct.q -host tcp://192.168.65.2:55111 -topic "Q/1" -data "hello world"
```
