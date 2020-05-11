
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

## Building Interface From Source

### Linux/Mac

Building the interface from source requires `gcc`, `gcc c++`, `make` and `cmake` packages installed on your development machine (e.g. xcode for Mac).

Follow these steps:

 1. Download the Solace C Messaging API ( https://solace.com/downloads/ ).
 2. Set an environment variable `SOLACE_API_DIR` to the location of the unzipped Solace C API.
 3. Run `cmake`
 4. Run `make install`


For example, on Linux/Mac, to create a build within the build directory

```bash
export SOLACE_API_DIR=/home/myuser/solaceapi/
cd build
cmake ../
make install
```

You should then find the resulting files in a newly created api dir, which should reside within the build directory.

### Windows

Building the interface from source requires Visual Studio

Follow these steps for Visual Studio 15 (2017) and above:

  1. Download the Solace C Messaging API ( https://solace.com/downloads/ ).
  2. Set an environment variable `SOLACE_API_DIR` to the location of the unzipped Solace C API.
  3. Run `cmake -G "Visual Studio 15 2017 Win64"` 

This will generate a visual studio build package. 

## Running

### Solace PubSub+ Event Broker

In order to pub/sub with Solace, you'll need to be connected to a [PubSub+ event broker](https://solace.com/products/event-broker/).

If you don't already have an event broker running, you can avail of a free standard edition [docker based broker](https://github.com/SolaceLabs/solace-single-docker-compose) or a [free cloud based instance](https://console.solace.cloud/login/new-account) (NOTE: other versions and enterprise solutions are also available).

### kdb+ Solace Integration

#### Download

Download the latest release from our [releases page](https://github.com/KxSystems/solace/releases)

#### Installation

##### Linux

Before installing, install the Solace C API if you have not yet done so:

  1. Download Solace C API from [here](https://solace.com/downloads/)(please select for your relevant machine type).
  2. Unzip the Solace API to a directory on the machine which the end user can read from.
  3. Add the lib directory to the `LD_LIBRARY_PATH` environment variable e.g. if unzipped to `/usr/local/solaceapi/`
     - `export LD_LIBRARY_PATH /usr/local/solaceapi/lib/:$LD_LIBRARY_PATH`

To install the library and scripts, either

- run the provided install.sh

or

- copy `kdbsolace.so` which was built or downloaded earlier to your kdb+ folder. If on a Linux 64bit machine with kdb+ installed in `/usr/local/q`, place the shared library into `/usr/local/q/l64/`.
- the q script to load the solace API (`solace.q`) can be placed in the current working directory or within the kdb+ install directory.


See examples and API documentation on how to tailor the interface for your needs.

##### Windows

Before installing, install the Solace C API if you have not yet done so:

1. Download Solace C API from [here](https://solace.com/downloads/)(please select for your relevant machine type).
2. Unzip the Solace API to a directory on the machine which the end user can read from.
3. Add the libsolclient.dll lib directory to the kdb lib (directory e.g. C:\q\w64)

To install the library and scripts, either

- run the provided install.bat

or

- copy `kdbsolace.dll` which was built or downloaded earlier, to your kdb+ install binary dir e.g. if kdb+ installed at `C:\q`, place the shared library into `C:\q\w64\`.
- copy the q script to load the solace API (`solace.q`) can be placed in the current working directory or within the kdb+ install directory.

See examples and API documentation on how to tailor the interface for your needs.

##### Mac 

Before installing, install the Solace C API if you have not yet done so:

  1. Download Solace C API from [here](https://solace.com/downloads/)(please select for your relevant machine type).
  2. Unzip the Solace API to a directory on the machine which the end user can read from.
  3. Add the lib directory to the `DYLD_LIBRARY_PATH` environment variable e.g. if unzipped to `/Users/jim/solaceapi/`
     - `export DYLD_LIBRARY_PATH /Users/jim/solaceapi/lib/:$DYLD_LIBRARY_PATH`

To install the library and scripts, either

- run the provided install.sh

or

- Copy `kdbsolace.so` which was built or downloaded earlier, to your kdb+ install binary dir e.g. if kdb+ installed at `/usr/local/q`, place the shared library into `/usr/local/q/m64/`..
- The q script to load the solace API (`solace.q`) can be placed in the current working directory or within the kdb+ install directory.

## Unsupported Functionality

Currently transactional based messaging is unsupported.

## Documentation

Documentation outlining the functionality available for this interface can be found [here](https://code.kx.com/q/interfaces/solace).

## Status
The Solace interface is provided here under an Apache 2.0 license.

If you find issues with the interface or have feature requests please consider raising an issue [here](https://github.com/KxSystems/solace/issues).

If you wish to contribute to this project please follow the contributing guide [here](CONTRIBUTING.md).
