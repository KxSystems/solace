# ![Solace](docs/solace.jpeg) kdb+ interface for Solace PubSub+ broker

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/kxsystems/solace?include_prereleases)](https://github.com/kxsystems/solace/releases) [![Travis (.org) branch](https://img.shields.io/travis/kxsystems/solace/master?label=travis%20build)](https://travis-ci.org/kxsystems/solace/branches)

A KX [Fusion interface](https://code.kx.com/q/interfaces/#fusion-interfaces)


This interface provides a mechanism for Solace PubSub+ brokers to interact with kdb+. The interface is a thin wrapper around the Solace C api documented [here](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/index.html)

## New to kdb+ ?

Kdb+ is the world's fastest timeseries database, optimized for ingesting, analyzing and storing massive amounts of structured data. To get started with kdb+, please visit https://code.kx.com/q/learn/ for downloads and developer information. For general information, visit https://kx.com/

## New to Solace PubSub+ ?

Solace PubSub+ Event Broker efficiently streams events and information across cloud, on-premises and within IoT environments. The “+” in PubSub+ means it supports a wide range of message exchange patterns beyond publish/subscribe. These include request/reply, streaming and replay, as well as different qualities of service, such as best effort and guaranteed delivery.

You can get started quickly by using free Standard Edition of the Solace [software broker](https://solace.com/products/event-broker/software/) or spin up a free instance on [Solace Cloud](https://console.solace.cloud/login/new-account).

If you have any Solace-related questions, you can raise them at [Solace Community](https://solace.community/).

## Installation

### Requirements

- kdb+ ≥ 3.5 64-bit (Linux/macOS/Windows)
- [Solace Messaging C API](https://solace.com/downloads/) (select for your relevant machine type).

### Installing a release

1.  Ensure [Solace Messaging C API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols) is installed.

2.  Make the Solace library available from kdb+:

    -   Linux: Add the lib directory which includes `include` and `lib` to the `LD_LIBRARY_PATH` environment variable e.g. if unzipped to `/usr/local/solaceapi/`, run:

        ```bash
        export LD_LIBRARY_PATH=/usr/local/solaceapi/lib/:$LD_LIBRARY_PATH
        ```
    -   macOS: Add the lib directory which includes `include` and `lib`to the `DYLD_LIBRARY_PATH` environment variable e.g. if unzipped to `/Users/jim/solaceapi/`, run:

        ```bash
        export DYLD_LIBRARY_PATH=/Users/jim/solaceapi/lib/:$DYLD_LIBRARY_PATH
        ```

    -   Windows: Add the `libsolclient.dll` to the kdb+ lib directory e.g. `C:\q\w64` for 64-bit
  
3.  Download the latest release of Solace-kdb+ interface from our [releases page](https://github.com/KxSystems/solaceses). To install shared library and q files, use:

        # Linux/MacOS
        ./install.sh
        
        # Windows
        install.bat

    or copy `solacekdb.so` or `solacekdb.dll` into `QHOME/[l|m|w]64`


## Documentation

-   [Overview](docs/README.md)
-   [Reference](docs/reference.md)
-   [RESTful API](docs/solacerest.md)
-   [Examples](examples/README.md)

## Quick start

If you don't already have an event broker running, you can avail of a free standard edition [docker-based broker](https://github.com/SolaceLabs/solace-single-docker-compose) or a [free cloud based instance](https://console.solace.cloud/login/new-account) (other versions and enterprise solutions are also available). 

### Using Solace Cloud

#### Requirements

- An account on [Solace Cloud](https://console.solace.cloud/)

#### Testing a connection to a service

Create a new service under the cluster manager e.g. 'testservice'. Select the newly created service, and visit the 'Connect' page, followed by 'Solace Messaging'. Download the pem file to a new directory & note all the connection details. Run the following (replacing the params with your connection info & changing 'cert' to the directory that contains your pem file). It should connect to the broker and ask the name of the platform e.g. 'Solace PubSub+ Enterprise'

```bash
solace]$ cd examples
examples]$ q sol_capabilities.q -host tcps://mr906gris.messaging.solace.cloud:55443 -user solace-cloud-client -pass ggut5g2v84pegdibs -vpn testservice -trust cert
```

### Using Docker-based broker

#### Requirements

- [Solace PubSub+ Event Broker](https://solace.com/products/event-broker/)


#### Start Event Broker

**Note:** Ensure that Docker is running.

```bash
]$ git clone https://github.com/SolaceLabs/solace-single-docker-compose.git
]$ cd solace-single-docker-compose/template
template]$ docker-compose -f PubSubStandard_singleNode.yml up -d
```

Please view the docs on https://github.com/SolaceLabs/solace-single-docker-compose to find out how to use the web interface & configure the SMF port.

#### Launch subscriber

A mock subscriber is provided in `examples/` folder of the repository. We assume that you are in the cloned source repository.

The subscriber subscribes to a topic `Q` with a wildcard: `Q/>` connecting to a Solace host 192.168.65.2 (configured to be a SMF host on port 55111)

```bash
solace]$ cd examples
examples]$ q sol_sub_direct.q -host tcp://192.168.65.2:55111 -topic "Q/>"
```

#### Launch publisher

Open another console and launch a publisher to send a message `"hello world"` via topic `Q/1`.

```bash
examples]$ q sol_pub_direct.q -host tcp://192.168.65.2:55111 -topic "Q/1" -data "hello world"
```

## Unsupported functionality

Currently transactional-based messaging is unsupported.

## Building interface from source

### Requirements

-   [Solace Messaging C API](https://solace.com/downloads/)(please select for your relevant machine type).
-   C++11 or later [^1] 
-   CMake ≥ 3.1 [^1]

[^1]: Required when building from source

### Linux/MacOSX

1.  Download the [Solace C Messaging API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols).
2.  Set `SOLACE_INSTALL_DIR` to the instalation directory of the Solace Messaging C API where `include` and `lib` exist. This environmental variable will be used to link the library to Solace-kdb+ interface.
3.  Add `SOLACE_INSTALL_DIR/lib` to `LD_LIBRARY_PATH` (Linux) or `DYLD_LIBRARY_PATH` (MacOSX).

```bash
]$ mkdir solclient
]$ tar xzf solclient_[machine and version].tar.gz -C ./solclient --strip-components=1
]$ cd solclient
solclient]$ export SOLACE_INSTALL_DIR=$(pwd)
```

4. Clone Solace-kdb+ repository and build with `cmake`.

```bash
]$ git clone https://github.com/KxSystems/solace.git
]$ cd solace
solace]$ mkdir build && cd build
build]$ cmake ../
build]$ cmake --build . --target install
```

**Note:** `cmake --build . --target install` as used in the Linux/MacOS builds installs the required share object and q files to the `QHOME/[ml]64` and `QHOME` directories respectively. If you do not wish to install these files directly, you can execute `cmake --build .` instead of `cmake --build . --target install` and move the files from their build location at `build/kdbsolace`.

### Windows

1.  Download the [Solace C Messaging API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols).
2.  Set `SOLACE_INSTALL_DIR` to the installation directory of the Solace Messaging C API where `include` and `lib` exist. This environmental variable will be used to link the library to Solace-kdb+ interface.
3.  Add the `libsolclient.dll` to the kdb+ lib directory e.g. `C:\q\w64` for 64-bit

     ```bat
     > 7z e solclient_[machine and version].tar.gz && 7z x solclient_[machine and version].tar -spe
     > rename solclient-[version] solclient
     > cd solclient
     solclient> set SOLACE_INSTALL_DIR=%cd%
     solclient> cd %QHOME%\w64
     w64> MKLINK libsolclient.dll %SOLACE_INSTALL_DIR%\bin\Win64\libsolclient.dll
     ```

4.  Clone Solace-kdb+ repository and build with `cmake`. Building the interface from source requires Visual Studio (assuming `-G "Visual Studio 15 2017 Win64"` is not necessary).

     ```bat
     > git clone https://github.com/KxSystems/solace.git
     > cd solace
     solace> mkdir build && build
     build> cmake --config Release ..
     build> cmake --build . --config Release --target install
     ```

**Notes:** 

1. `cmake --build . --config Release --target install` installs the required share object and q files to the `QHOME\w64` and `QHOME` directories respectively. If you do not wish to install these files directly, you can execute `cmake --build . --config Release` instead of `cmake --build . --config Release --target install` and move the files from their build location at `build/kdbsolace`.
2.  You can use flag `cmake -G "Visual Studio 16 2019" -A Win32` if building 32-bit version.

## Status

The Solace interface is provided here under an Apache 2.0 license.

If you find issues with the interface or have feature requests please  [raise an issue](../../issues).

To contribute to this project please follow the [contribution guide](CONTRIBUTING.md).
