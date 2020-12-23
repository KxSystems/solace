
# kdb+ interface for Solace PubSub+ broker

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/kxsystems/solace?include_prereleases)](https://github.com/kxsystems/solace/releases) [![Travis (.org) branch](https://img.shields.io/travis/kxsystems/solace/master?label=travis%20build)](https://travis-ci.org/kxsystems/solace/branches)

## Introduction

This interface provides a mechanism for Solace PubSub+ brokers to interact with kdb+. The interface is a thin wrapper around the Solace C api documented [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/index.html)

## New to kdb+ ?

Kdb+ is the world's fastest time-series database, optimized for ingesting, analyzing and storing massive amounts of structured data. To get started with kdb+, please visit https://code.kx.com/q/learn/ for downloads and developer information. For general information, visit https://kx.com/

## New to Solace PubSub+ ?

Solace PubSub+ Event Broker efficiently streams events and information across cloud, on-premises and within IoT environments. The “+” in PubSub+ means it supports a wide range of message exchange patterns beyond publish/subscribe. These include request/reply, streaming and replay, as well as different qualities of service, such as best effort and guaranteed delivery.

You can get started quickly by using free Standard Edition of the Solace [software broker](https://solace.com/products/event-broker/software/) or spin up a free instance on [Solace Cloud](https://console.solace.cloud/login/new-account).

If you have any Solace related questions, you can raise them at [Solace Community](https://solace.community/).

## Installation

### Requirements

- kdb+ ≥ 3.5 64-bit (Linux/MacOS/Windows)
- [Solace Messaging C API](https://solace.com/downloads/)(please select for your relevant machine type).
- C++11 or later [^1] 
- CMake ≥ 3.1 [^1]

[^1]: Required when building from source

### Installing a release

1. Ensure [Solace Messaging C API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols) is installed.
2. Make the Solace library available from kdb+:
   - Linux: Add the lib directory which includes `include` and `lib` to the `LD_LIBRARY_PATH` environment variable e.g. if unzipped to `/usr/local/solaceapi/`, run:
        ```bash

        $ export LD_LIBRARY_PATH=/usr/local/solaceapi/lib/:$LD_LIBRARY_PATH
        
        ```
   - MacOS: Add the lib directory which includes `include` and `lib`to the `DYLD_LIBRARY_PATH` environment variable e.g. if unzipped to `/Users/jim/solaceapi/`, run:
        ```bash

        $ export DYLD_LIBRARY_PATH=/Users/jim/solaceapi/lib/:$DYLD_LIBRARY_PATH
      
        ```
   - Windows: Add the `libsolclient.dll` to the kdb+ lib directory e.g. `C:\q\w64` for 64-bit
  
3. Download the latest release of Solace-kdb+ interface from our [releases page](https://github.com/KxSystems/solaceses). To install shared library and q files, use:

        # Linux/MacOS
        $ ./install.sh

        # Windows
        > install.bat

    or copy `kdbsolace.so` or `kdbsolace.dll` into `QHOME/[l|m|w]64`

### Building Interface From Source

### Linux/Mac

1. Download the [Solace C Messaging API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols).
2. Set `SOLACE_INSTALL_DIR` to the instalation directory of the Solace Messaging C API. This environmental variable will be used to link the library to Solace-kdb+ interface.

```bash

]$ mkdir solclient
]$ tar xzf solclient_[machine and version].tar.gz -C ./solclient --strip-components=1
]$ cd solclient
solclient]$ export SOLACE_INSTALL_DIR=$(pwd)

```

3. Clone Solace-kdb+ repository and build with `cmake`.

```bash

]$ git clone https://github.com/KxSystems/solace.git
]$ cd solace
solace]$ mkdir build && cd build
build]$ cmake ../
build]$ cmake --build . --target install

```

**Note:** `cmake --build . --target install` as used in the Linux/MacOS builds installs the required share object and q files to the `QHOME/[ml]64` and `QHOME` directories respectively. If you do not wish to install these files directly, you can execute `cmake --build .` instead of `cmake --build . --target install` and move the files from their build location at `build/solace`.

### Windows

1. Download the [Solace C Messaging API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols).
2. Set `SOLACE_INSTALL_DIR` to the instalation directory of the Solace Messaging C API. This environmental variable will be used to link the library to Solace-kdb+ interface.

```bat

> mkdir solclient
> unzip solclient_[machine and version].zip-C ./solclient --strip-components=1
> cd solclient
solclient> set SOLACE_INSTALL_DIR=%cd%

```

1. Clone Solace-kdb+ repository and build with `cmake`. Building the interface from source requires Visual Studio (Assuming `-G "Visual Studio 15 2017 Win64"` is not necessary).

```bat

> git clone https://github.com/KxSystems/solace.git
> cd solace
solace> mkdir build && build
build> cmake --config Release ..
build> cmake --build . --config Release --target install

```

**Notes:** 

1. `cmake --build . --config Release --target install` installs the required share object and q files to the `QHOME\w64` and `QHOME` directories respectively. If you do not wish to install these files directly, you can execute `cmake --build . --config Release` instead of `cmake --build . --config Release --target install` and move the files from their build location at `build/solace`.
2. You can use flag `cmake -G "Visual Studio 16 2019" -A Win32` if building 32-bit version.

## Quick Start

### Requirements

- [Solace PubSub+ Event Broker](https://solace.com/products/event-broker/)

If you don't already have an event broker running, you can avail of a free standard edition [docker-based broker](https://github.com/SolaceLabs/solace-single-docker-compose) or a [free cloud based instance](https://console.solace.cloud/login/new-account) (other versions and enterprise solutions are also available). In this example we use the docker-based broker.

### Start Event Broker

**Note:** Ensure that docker is running.

```bash

]$ git clone https://github.com/SolaceLabs/solace-single-docker-compose.git
]$ cd solace-single-docker-compose/template
template]$ docker-compose -f PubSubStandard_singleNode.yml up -d

```

### Launch Subscriber

A mock subscriber is provided in `examples/` folder of the repository. We assume that you are in the cloned source repository.

The subscriber subscribes to a topic `Q` with a wildcard: `Q/>` listening on a port 5000.

```bash

solace]$ cd examples
examples]$ q sol_sub_direct.q -host 127.0.0.1:5000 -topic "Q/>"

```

### Launch Publiser

Open another console and launch a publisher to send a message `"hello world"` via topic `Q/1`.

```bash

examples]$ q sol_pub_direct.q -topic "Q/1" -data "hello world"

```

## Unsupported Functionality

Currently transactional based messaging is unsupported.

## Documentation

Documentation outlining the functionality available for this interface can be found [here](https://code.kx.com/q/interfaces/solace).

## Status

The Solace interface is provided here under an Apache 2.0 license.

If you find issues with the interface or have feature requests please consider raising an issue [here](https://github.com/KxSystems/solace/issues).

If you wish to contribute to this project please follow the contributing guide [here](CONTRIBUTING.md).
