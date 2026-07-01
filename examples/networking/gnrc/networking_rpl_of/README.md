# gnrc_networking_rpl OF Experiment

This example tests different objective functions.

To showcase the difference between different objective functions and metrics, 
each node in the mesh periodically sends a message to the root node.

The following parameters can be adjusted before running the experiment: 
- wireless standard: 0 for IEEE 108.15.4 and 1 for BLE
- network size in number of nodes up to 100 nodes
- packet interval in milliseconds
- OF: 0 for OF0, 1 for MRHOF-ETX, 2 for MRHOF-Energy

The resulting data is recorded by an external logging device.

This allows for direct comparison between routing strategies.

## Setup on mesh nodes

The following commands are used to conduct the experiment:

| start_root | start root to start experiment officially and start recording data |
| start_send | alert this child node start sending data |
| stop_send | notify this child node to step sending data to stop experiment |
| set_params | set the parameters for the experiment (see above) |
