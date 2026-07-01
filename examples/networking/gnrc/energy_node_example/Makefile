APPLICATION = raw_ipv6_rpl

# If no BOARD is found in the environment, use this default:
BOARD ?= feather-nrf52840-sense

CFLAGS += -Wno-unused-parameter

RIOTBASE ?= $(CURDIR)/../../../..

# Required modules
USEMODULE += xtimer
USEMODULE += shell

# For UDP
USEMODULE += sock_udp
USEMODULE += sock
USEMODULE += sock_util
USEMODULE += sock
USEMODULE += sock_async_event

USEMODULE += netdev_default
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
USEMODULE += saul
USEMODULE += saul_default
USEMODULE += ws281x

FEATURES_REQUIRED += periph_adc
CFLAGS += -DADC_BATTERY_LINE=ADC_LINE\(5\)
# in case border router is up
CFLAGS += -DCONFIG_IEEE802154_DEFAULT_CHANNEL=15

# Disable shell to save space and enforce no-shell operation
#DISABLE_MODULE += shell
UF2_MOUNTPOINT = /media/pi/feather

include $(RIOTBASE)/Makefile.include

ifeq ($(ROOT),1)
  CFLAGS += -DROOT=1
else
  CFLAGS += -DROOT=0
endif
