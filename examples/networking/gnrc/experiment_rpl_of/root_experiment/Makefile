APPLICATION = experiment_root

# If no BOARD is found in the environment, use this default:
BOARD ?= feather-nrf52840-sense

CFLAGS += -Wno-unused-parameter

RIOTBASE ?= $(CURDIR)/../../../../..

# Required modules
USEMODULE += ztimer
USEMODULE += ztimer_sec
USEMODULE += ztimer_usec
USEMODULE += sema_inv

# Neighbor Stats
USEMODULE += netstats_neighbor
USEMODULE += netstats_neighbor_etx
USEMODULE += netstats_neighbor_rssi
USEMODULE += netstats_neighbor_lqi

# For UDP
USEMODULE += sock_udp
USEMODULE += sock_util
USEMODULE += sock
USEMODULE += sock_async_event

USEMODULE += netdev_default
USEMODULE += netutils
USEMODULE += gnrc_ipv6_default
USEMODULE += auto_init_gnrc_netif

# Include GNRC network stack
USEMODULE += gnrc_netif
USEMODULE += gnrc_neterr
USEMODULE += gnrc_pktdump

# RPL specific modules
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_rpl_mrhof
#USEMODULE += gnrc_rpl_mrhof_etx
# optional alternative metric, hardware must support it
USEMODULE += gnrc_rpl_mrhof_lqi
# use only when battery connected
#USEMODULE += gnrc_rpl_mrhof_energy

# adjust these parameters to the number of nodes and the number of parent set
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_NUMOF=10
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_OFFL_NUMOF=10
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_DEFAULT_ROUTER_NUMOF=3

# UDP queue size (standard=3)
#CFLAGS += -DCONFIG_GNRC_UDP_MSG_QUEUE_SIZE_EXP=4

# Shell
USEMODULE += shell
USEMODULE += shell_cmds_default

# in case border router is up
CFLAGS += -DCONFIG_IEEE802154_DEFAULT_CHANNEL=15

# extra module
SRC += main.c
SRC += experiment.c
SRC += structures.c

# Disable shell to save space and enforce no-shell operation
#DISABLE_MODULE += shell
UF2_MOUNTPOINT = /media/pi/feather

include $(RIOTBASE)/Makefile.include