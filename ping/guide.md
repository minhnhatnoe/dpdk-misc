# A guide on kernel-bypass high-frequency low-latency pinging

## Introduction

## Data Plane Developement Kit (DPDK)

"DPDK is a set of libraries and drivers for fast packet processing" -- dpdk's README

DPDK is a very nice tool etc.

DPDK lets you control basically everything you need to make packets come in and get out fast: CPU frequency settings, custom drivers for GPUs, which core(s) your code runs on, etc. For the purpose of spamming pings though, we only need to care about DPDK's Elastic Network Adapter (ENA) drivers.

### AWS ENA

`https://docs.aws.amazon.com/ec2/latest/instancetypes/co.html`. Scroll to network specifications

Security group allows all incoming and outgoing traffic

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

Heartbeat

`https://doc.dpdk.org/guides/prog_guide/mbuf_lib.html`

## The underlying layers

### Ethernet

Parse ether to see what it is. Use pktmbuf_read, look at ether_type. In general, we refer to `https://github.com/ceph/dpdk/blob/master/lib/librte_net/rte_net.c`'s `rte_net_get_ptype`.

https://en.wikipedia.org/wiki/EtherType

### ARP

Before we get to ping, recall that ARP is needed. Do we actually need it?

https://en.wikipedia.org/wiki/Address_Resolution_Protocol

People need to know our address and our address need to be known.

We first have to know our address. Coincidentally AWS ENA interfaces receive only requests for themselves and requests are not visible. Hence, we can infer our own addr. Verify:

> Temporarily bind one device to ena. Run `ip a`, we see its address is `172.31.36.251/20`. Now rebind
> 
> Run `sudo arping 172.31.36.251`. Notice:
> - We receive response about 40
> - We don't hear about this request on the igb_uio-bound devices
>
> If you want to be even more sure, run `sudo tcpdump arp` to verify in-out packets of current node. Make sure 0 packets dropped by kernel when you ctrl-c.

We also verify that all packets are addressed to us.

### Ping
