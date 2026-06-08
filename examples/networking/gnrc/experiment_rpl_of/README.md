# Experiment comparing RPL OFs

## Introduction

This experiment builds a RPL DODAG and sends messages between the root and other nodes, meanwhile collecting data on routing metrics.
In this tutorial we build a **multi-hop IPv6 network using RPL** (Routing Protocol for Low-Power and Lossy Networks) on RIOT OS.

## How to Run

Flash **one node as the root** and the others as regular nodes from within the root_experiment and node_experiment folder respectively:

```bash
# Root node
make all flash term PORT=/dev/ttyACM0

# Non-root nodes
make all flash term PORT=/dev/ttyACM1
make all flash term PORT=/dev/ttyACM2
...
```

All nodes must be within radio range to form an RPL topology.

If there are multiple Root Nodes change the Instance ID in these:

```
gnrc_rpl_root_init(1, &dodag_id_a, false, false);
gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(1);
```

---

Data is printed automatically in the console.

This experiment was designed with testing intended to be done on the Fit IoT Lab.