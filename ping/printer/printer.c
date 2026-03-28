/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_net.h>
#include <arpa/inet.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */

/* Main functional part of port initialization. 8< */
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	memset(&port_conf, 0, sizeof(struct rte_eth_conf));

	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
				rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Starting Ethernet port. 8< */
	retval = rte_eth_dev_start(port);
	/* >8 End of starting of ethernet port. */
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval != 0)
		return retval;

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port, RTE_ETHER_ADDR_BYTES(&addr));

	return 0;
}
/* >8 End of main functional part of port initialization. */

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */

 /* Basic forwarding application lcore. 8< */
static __rte_noreturn void
lcore_main(void)
{
	uint16_t port;
	uint32_t ip_addrs[RTE_MAX_ETHPORTS];
	memset(ip_addrs, 0, sizeof ip_addrs);

	struct rte_ether_addr mac_addrs[RTE_MAX_ETHPORTS];
	RTE_ETH_FOREACH_DEV(port) {
		if (rte_eth_macaddr_get(port, &mac_addrs[port])) {
			rte_exit(EXIT_FAILURE, "Failed to get MAC address for port %u\n", port);
		}
	}

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	RTE_ETH_FOREACH_DEV(port)
		if (rte_eth_dev_socket_id(port) >= 0 &&
				rte_eth_dev_socket_id(port) !=
						(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	/* Main work of application loop. 8< */
	for (uint32_t s = 0;; s++) {
		if (s % (1 << 26) == 0) printf("MAIN: heartbeat %d\n", s / (1 << 26));
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */
		RTE_ETH_FOREACH_DEV(port) {

			/* Get burst of RX packets, from first port of pair. */
			struct rte_mbuf *bufs[BURST_SIZE];
			const uint16_t nb_rx = rte_eth_rx_burst(port, 0,
					bufs, BURST_SIZE);

			if (unlikely(nb_rx == 0))
				continue;

			printf("MAIN: Received %d packets on port %u.\n", nb_rx, port);
			for (uint16_t i = 0; i < nb_rx; i++) {
				struct rte_mbuf *mbuf = bufs[i];

				printf("MAIN: packet number %d with size %d\n",
					i, mbuf->buf_len);

				uint32_t offset = 0;
				struct rte_ether_hdr ether_hdr_tmp;
				struct rte_ether_hdr const *ether_hdr = rte_pktmbuf_read(
					mbuf, offset, sizeof ether_hdr_tmp, &ether_hdr_tmp);
				offset += sizeof ether_hdr_tmp;

				if (!rte_is_same_ether_addr(&ether_hdr->dst_addr, &mac_addrs[port]) &&
					!rte_is_broadcast_ether_addr(&ether_hdr->dst_addr)) {
					rte_exit(EXIT_FAILURE, "MAIN: WARNING: packet is not for us\n");
				}
				if (ether_hdr == NULL) {
					rte_exit(EXIT_FAILURE, "MAIN: WARNING: packet too short for Ethernet header\n");
				}

				uint16_t ether_type = rte_be_to_cpu_16(ether_hdr->ether_type);
				if (ether_type == RTE_ETHER_TYPE_ARP) {
					printf("MAIN: is ARP\n");

					struct rte_arp_hdr arp_hdr_tmp;
					struct rte_arp_hdr const *arp_hdr = rte_pktmbuf_read(
						mbuf, offset, sizeof arp_hdr_tmp, &arp_hdr_tmp);
					offset += sizeof arp_hdr_tmp;

					if (arp_hdr == NULL) {
						rte_exit(EXIT_FAILURE, "MAIN: WARNING: packet too short for ARP header\n");
					}

					if (rte_be_to_cpu_16(arp_hdr->arp_hardware) != RTE_ARP_HRD_ETHER ||
						rte_be_to_cpu_16(arp_hdr->arp_protocol) != RTE_ETHER_TYPE_IPV4 ||
						arp_hdr->arp_hlen != sizeof(struct rte_ether_addr) ||
						arp_hdr->arp_plen != sizeof(rte_be32_t)) {
						rte_exit(EXIT_FAILURE, "MAIN: WARNING: ARP header has unexpected format\n");
					}

					struct rte_arp_ipv4 const *arp_data = &arp_hdr->arp_data;
					if (rte_be_to_cpu_16(arp_hdr->arp_opcode) == RTE_ARP_OP_REQUEST) {
						char ip_str_target[INET_ADDRSTRLEN];
						if (!inet_ntop(AF_INET, &arp_data->arp_tip, ip_str_target, sizeof ip_str_target)) {
							rte_exit(EXIT_FAILURE, "MAIN: WARNING: inet_ntop failed\n");
						}

						if (arp_data->arp_tip != ip_addrs[port] && ip_addrs[port]) {
							char ip_str_port[INET_ADDRSTRLEN];
							if (!inet_ntop(AF_INET, &ip_addrs[port], ip_str_port, sizeof ip_str_port)) {
								rte_exit(EXIT_FAILURE, "MAIN: WARNING: inet_ntop failed\n");
							}
							rte_exit(EXIT_FAILURE, "MAIN: port %d's ip address is %s, but ARP request is for %s\n",
								port, ip_str_port, ip_str_target);
						}

						printf("MAIN: is ARP request for us\n");
						if (!ip_addrs[port]) {
							printf("MAIN: port %d's ip address is %s\n", port, ip_str_target);
							ip_addrs[port] = arp_data->arp_tip;
						}

						struct {
							struct rte_ether_hdr ether_hdr;
							struct rte_arp_hdr arp_hdr;
						} __attribute__((packed)) arp_reply_payload = {
							.ether_hdr = {
								.dst_addr = ether_hdr->src_addr,
								.src_addr = mac_addrs[port],
								.ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP),
							},
							.arp_hdr = {
								.arp_hardware = rte_cpu_to_be_16(RTE_ARP_HRD_ETHER),
								.arp_protocol = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4),
								.arp_hlen = sizeof(struct rte_ether_addr),
								.arp_plen = sizeof(rte_be32_t),
								.arp_opcode = rte_cpu_to_be_16(RTE_ARP_OP_REPLY),
								.arp_data = {
									.arp_sha = mac_addrs[port],
									.arp_sip = ip_addrs[port],
									.arp_tha = arp_data->arp_sha,
									.arp_tip = arp_data->arp_sip,
								},
							},
						};

						struct rte_mbuf *reply_mbuf = rte_pktmbuf_alloc(mbuf->pool);
						if (reply_mbuf == NULL) {
							rte_exit(EXIT_FAILURE, "MAIN: WARNING: failed to allocate mbuf for ARP reply\n");
						}

						char *reply_data = rte_pktmbuf_append(reply_mbuf, sizeof arp_reply_payload);
						if (reply_data == NULL) {
							rte_exit(EXIT_FAILURE, "MAIN: WARNING: failed to append data to mbuf for ARP reply\n");
						}
						memcpy(reply_data, &arp_reply_payload, sizeof arp_reply_payload);
						
						const uint16_t nb_tx = rte_eth_tx_burst(port, 0, &reply_mbuf, 1);
						if (nb_tx < 1) {
							rte_pktmbuf_free(reply_mbuf);
							rte_exit(EXIT_FAILURE, "MAIN: WARNING: failed to send ARP reply\n");
						} else {
							printf("MAIN: Sent ARP reply on port %u.\n", port);
						}
					} else {
						rte_exit(EXIT_FAILURE, "MAIN: WARNING: rogue ARP packet\n");
					}
				} else {
					printf("MAIN: WARNING: Unknown ether type\n");
				}

				rte_pktmbuf_free(mbuf);
			}
		}
	}
	/* >8 End of loop. */
}
/* >8 End Basic forwarding application lcore. */

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	unsigned nb_ports;
	uint16_t portid;

	/* Initializion the Environment Abstraction Layer (EAL). 8< */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	/* >8 End of initialization the Environment Abstraction Layer (EAL). */

	argc -= ret;
	argv += ret;

	/* Check that there is an even number of ports to send/receive on. */
	nb_ports = rte_eth_dev_count_avail();

	/* Creates a new mempool in memory to hold the mbufs. */

	/* Allocates mempool to hold the mbufs. 8< */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	/* >8 End of allocating mempool to hold mbuf. */

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool %s\n", rte_strerror(rte_errno));

	/* Initializing all ports. 8< */
	RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
					portid);
	/* >8 End of initializing all ports. */

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the main core only. Called on single lcore. 8< */
	lcore_main();
	/* >8 End of called on single lcore. */

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}
