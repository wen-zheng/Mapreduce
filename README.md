# Introduction

This repository is created for the mapreduce course of NJU. Our project is based on [ns-3 quic module](https://gitlab.com/diegoamc/ns-3-quic-module). After [installing this module](#Installation) successfully, you have the ability to [run our lab](#RunOurLab). Our changes to this module can be seen [here](#Changinglist).

# RunOurLab

After successfully completing the installation, you can test our simulation scripts from the __~/workspace/ns-allinone-3.27/ns-3.27__ directory as follows:

1. `mv ./src/quic/ip-cluster ./ip-cluster`
2. `mv ./src/quic/cluster-min-max-mean ./cluster-min-max-mean`
3. `mv ./src/quic/mapreduce.sh ./mapreduce.sh`
4. `sudo chmod 777 ./mapreduce.sh`
5. Test:  
`./waf --run quic-mapreduce`  
Repeate our lab:   
`./waf shell`  
`export LD_LIBRARY_PATH=/path/to/workspace/ns-allinone-3.27/ns-3.27/build/:/path/to/workspace/ns-allinone-3.27/ns-3.27/src/quic/model/libs` (absolute path)  
`./mapreduce.sh > statistics`  

If you have successfully run these command, you will see 2 output file in __~/workspace/ns-allinone-3.27/ns-3.27__ directory.(and some trace file.):  
* log.txt: this file contains connection status info collected by quic server.
* statistics: this file contains statistical info of each flow.

# Changinglist

Because there are too many changings, I only list these files path, for more details, please diff our project with [original quic module](https://gitlab.com/diegoamc/ns-3-quic-module).  


* [examples/wscript](examples/wscript)
* [examples/quic-mapreduce.cc](examples/quic-mapreduce.cc)
* [model/net/quic/core/congestion_control/rtt_stats.cc](model/net/quic/core/congestion_control/rtt_stats.cc)
* [model/net/quic/core/congestion_control/rtt_stats.h](model/net/quic/core/congestion_control/rtt_stats.h)
* [model/net/quic/core/congestion_control/send_algorithm_interface.cc](model/net/quic/core/congestion_control/send_algorithm_interface.cc)
* [model/net/quic/core/congestion_control/tcp_cubic_sender_bytes.cc](model/net/quic/core/congestion_control/tcp_cubic_sender_bytes.cc)
* [model/net/quic/core/congestion_control/tcp_cubic_sender_bytes.h](model/net/quic/core/congestion_control/tcp_cubic_sender_bytes.h)
* [model/net/quic/core/quic_connection.cc](model/net/quic/core/quic_connection.cc)
* [model/net/quic/core/quic_connection.h](model/net/quic/core/quic_connection.h)
* [model/net/quic/core/quic_connection_stats.cc](model/net/quic/core/quic_connection_stats.cc)
* [model/net/quic/core/quic_flags_list.h](model/net/quic/core/quic_flags_list.h)
* [model/net/quic/core/quic_sent_packet_manager.cc](model/net/quic/core/quic_sent_packet_manager.cc)
* [model/net/quic/core/quic_sent_packet_manager.h](model/net/quic/core/quic_sent_packet_manager.h)
* [model/net/quic/core/quic_session.cc](model/net/quic/core/quic_session.cc)
* [model/net/quic/core/quic_spdy_session.cc](model/net/quic/core/quic_spdy_session.cc)
* [model/net/quic/core/quic_stream.cc](model/net/quic/core/quic_stream.cc)
* [model/net/quic/core/quic_version_manager.cc](model/net/quic/core/quic_version_manager.cc)
* [model/net/tools/quic/quic_client_base.cc](model/net/tools/quic/quic_client_base.cc)
* [model/net/tools/quic/quic_client_message_loop_network_helper.cc](model/net/tools/quic/quic_client_message_loop_network_helper.cc)
* [model/net/tools/quic/quic_dispatcher.cc](model/net/tools/quic/quic_dispatcher.cc)
* [model/net/tools/quic/quic_dispatcher.h](model/net/tools/quic/quic_dispatcher.h)
* [model/net/tools/quic/quic_server.cc](model/net/tools/quic/quic_server.cc)
* [model/net/tools/quic/quic_simple_dispatcher.cc](model/net/tools/quic/quic_simple_dispatcher.cc)
* [model/net/tools/quic/quic_simple_dispatcher.h](model/net/tools/quic/quic_simple_dispatcher.h)
* [model/net/tools/quic/quic_simple_server.cc](model/net/tools/quic/quic_simple_server.cc)
* [model/net/tools/quic/quic_simple_server_stream.cc](model/net/tools/quic/quic_simple_server_stream.cc)
* [model/net/tools/quic/quic_spdy_client_base.cc](model/net/tools/quic/quic_spdy_client_base.cc)
* [model/net/tools/quic/quic_spdy_client_stream.cc](model/net/tools/quic/quic_spdy_client_stream.cc)
* [utils/quic-client.cc](utils/quic-client.cc)
* [utils/quic-server.cc](utils/quic-server.cc)

# QUIC module for ns-3

This is the `quic` module, which enhances the ns-3 Network Simulator with the
[QUIC](https://www.chromium.org/quic) protocol capabilities, allowing ns-3 users
to use this transport protocol on simulations. In fact, this module adapts code
from [proto-quic](https://github.com/google/proto-quic), a standalone library
for QUIC.

# Installation

__Caveat__: The module and the installation have only been tested on Ubuntu 16.04.

This installation guide assumes your are working on a __~/workspace__ directory.

### Install NS-3.27

1. Download NS-3 v3.27 source code from their [website](https://www.nsnam.org/release/ns-allinone-3.27.tar.bz2)
2. Extract the files with `tar xvjf ns-allinone-3.27.tar.bz2 ns-allinone-3.27/`
3. Change to directory ns-allinone-3.27/ns-3.27
4. Make sure ns-3.27 compiles successfully:
  - ./waf configure --enable-examples --enable-tests
  - ./waf build
  - If it doesn't, try downloading the ns-3-dev source code

### Build proto-quic to have access to system libraries ns-3-quic-module needs

From __~/workspace__:

1. git clone https://gitlab.com/diegoamc/proto-quic.git
2. `cd proto-quic`
3. ``export PROTO_QUIC_ROOT=`pwd`/src``
4. ``export PATH=$PATH:`pwd`/depot_tools``
5. `./proto_quic_tools/sync.sh`
6. `./src/build/install-build-deps.sh`  (debug dependencies are not needed)

### Install ns-3-quic-module

From __~/workspace/ns-allinone-3.27/ns-3.27/src__:

1. git clone https://gitlab.com/diegoamc/ns-3-quic-module.git quic
2. cd ~/workspace/ns-allinone-3.27/ns-3.27/
3. ./waf clean
4. ./waf configure --enable-example --enable-tests
5. export LD_LIBRARY_PATH=$LD_LIBRARY_PATH/path/to/workspace/ns-allinone-3.27/ns-3.27/src/quic/model/libs (absolute path to the quic model libs)

### Running ns-3-quic-module simulation examples

ns-3-quic-module comes with built-in examples. Their simulation scripts can be found [here](examples/).

After successfully completing the installation, you can run the examples from the __~/workspace/ns-allinone-3.27/ns-3.27__ directory as follows:

```bash
./waf --run quic-example
```

or

```bash
./waf --run quic-example-dumbbell
```

Notice that both simulations have parameters that can be configured using command line arguments. For example, to configure the total number
of bytes sent by the server to 1000 on __quic-example__, run:

```bash
./waf --run "quic-example --maxBytes=1000"
```

If you want to run examples that use TCP Cubic, please download TCP Cubic implementation [here](http://perform.wpi.edu/downloads/#cubic)
and follow the [steps to install it](http://perform.wpi.edu/downloads/cubic/README.txt).

# Running with Docker

If you just want to run the simulation scripts and don't want to configure everything, you can use the [Dockerfile](Dockerfile) provided.
It will basically run the commands described in the [Installation section](#installation) and prepare the container to run as an executable.

After installing `docker`, clone this repository and change to the root directory of the project. Then build the image with:

```bash
docker build -t ns-3-quic-module .
```

Building the image can take a long time. The image will occupy 10.1GB from your disk space. To run the simulations, execute:

```bash
docker run -it --rm ns-3-quic-module
```

__quic-example__ with the default configuration is the default simulation script. To change to another example say, __quic-example-dumbbell__, run:

```bash
docker run -it --rm ns-3-quic-module --run quic-example-dumbbell
```

And to configure simulation parameters:

```bash
docker run -it --rm ns-3-quic-module --run "quic-example --maxBytes=100"
```

# Acknowledgements

- Yan Soares Couto (@yancouto)
