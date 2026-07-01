# Experiment comparing RPL OFs

## Introduction

This experiment is based on the UDP benchmark module, 
sending packets and replies back and forth from other nodes to the root, 
meanwhile collecting data on routing metrics.
This allows for direct comparison between routing strategies.

## How to Run

Flash **one node as the root** and the others as regular nodes from within the root_experiment and node_experiment folder respectively. Start with the root node. The root should be initialized as the root beforehand so the other nodes can find it:

```bash
# Root node
make all flash term PORT=/dev/ttyACM0

# Non-root nodes
make all flash term PORT=/dev/ttyACM1
make all flash term PORT=/dev/ttyACM2
...
```

All nodes must be within radio range to form an RPL topology.

### Preparation

Before starting the experiment, you should set the experiment parameters in experiment.h. The parameters are within a block marked "Parameter configuration". You may change the

- load of the experiment (duration between the messages) to per second, every half second or every 50 ms
- number of ping (duration of experiment)
- number of nodes (network size)
- packet size

The Objective Function (OF) which is used can also be changed. In order to do that, change the modules used in the Makefile under RPL specific modules of the root and mesh nodes.
To run OF0, add no other besides the required
```
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
USEMODULE += gnrc_ipv6_router_default
```
To run MRHOF with the standard ETX implementation, add these modules
```
USEMODULE += gnrc_rpl_mrhof
USEMODULE += gnrc_rpl_mrhof_lqi
```

To run MRHOF with the LQI metric, add these modules
```
USEMODULE += gnrc_rpl_mrhof
USEMODULE += gnrc_rpl_mrhof_lqi
```

To run MRHOF with the energy metric, add these modules
```
USEMODULE += gnrc_rpl_mrhof
USEMODULE += gnrc_rpl_mrhof_energy
FEATURES_REQUIRED += periph_adc
CFLAGS += -DADC_BATTERY_LINE=ADC_LINE\(5\)
```
In order to use the energy metric, a battery has to be attached to the node. You should also adjust the battery defines in /node_experiment/battery.h according to your battery. Pay special attention to desired lifetime variable T.

### On root node
As the program is running, the root node will print out data of each packet received.
The data includes: Round Trip Time (RTT), Packet Delivery Ratio (PDR), ETX, Hop Count, RSSI, Energy.
This data is not saved during runtime. You should save the console prints as they come.

When the other nodes have finished sending their packets, run 
```
experiment stop
```
to stop the receiving messages.

Then run
```
test start
```
to print out the data collected.

This data includes the node's last two bytes as its name, its last parent's last two bytes as their name, its PDR, average RTT, average ETX, average RSSI, the amount of times a node switched its parent, average Hop Count and average Energy as well as the network's size, average PDR average RTT, average ETX, average RSSI, average Hop Count and average Energy and how many times nodes changed their parent on average and the network's topology.

### On mesh node
Simply flash the application onto the nodes. The mesh nodes will stop sending packets when the previously set amount of pings has been reached.