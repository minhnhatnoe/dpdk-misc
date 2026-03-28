# A guide on kernel-bypass high-frequency low-latency pinging

## Introduction

## Data Plane Developement Kit (DPDK)

"DPDK is a set of libraries and drivers for fast packet processing" -- dpdk's README

DPDK is a very nice tool etc.

DPDK lets you control basically everything you need to make packets come in and get out fast: CPU frequency settings, custom drivers for GPUs, which core(s) your code runs on, etc. For the purpose of spamming pings though, we only need to care about DPDK's Elastic Network Adapter (ENA) drivers.

### Setting up

`sudo apt install dpdk dpdk-dev dpdk-doc dpdk-kmods-dkms`

`sudo modprobe igb_uio`

`dpdk-devbind.py -s`

`sudo dpdk-devbind.py -b igb_uio 0000:28:00.0 --force`

`sudo dpdk-hugepages.py --setup 512M`

Testing:


## A DPDK example

Let me introduce you to DPDK using the most contrived example I could possibly think of, which is to print out all packets that we receive over the network.

We start with dpdk skeleton `/usr/share/dpdk/examples/skeleton`. Copy of skeleton provided in repo. Note on promiscuous.

`cp -r /usr/share/dpdk/examples/skeleton ping/`

Remove promiscuous

## The underlying layers

Before we get to ping, recall that ARP

`https://doc.dpdk.org/guides/prog_guide/mbuf_lib.html`

Heartbeat


Parse ether to see what it is. Use pktmbuf_read, look at ether_type. In general, we refer to `https://github.com/ceph/dpdk/blob/master/lib/librte_net/rte_net.c`'s `rte_net_get_ptype`.


