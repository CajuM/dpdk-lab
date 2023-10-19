#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#define die(cond) if (!(cond)) do { printf("%s: %s\n", __PRETTY_FUNCTION__, (#cond)); exit(-1); } while(false)

#define MTU 1500
#define IFACE_ID 0
#define RX_QUEUE_ID 0
#define TX_QUEUE_ID 0
#define RX_NB_MBUF 1024
#define TX_NB_MBUF 1024
#define MEMPOOL_CACHE_SIZE 512
#define MBUF_SIZE 2048
#define NB_RX_QUEUES 1
#define NB_TX_QUEUES 1
#define NB_RX_DESCRIPTORS 2048
#define NB_TX_DESCRIPTORS 2048

int main(int argc, char **argv) {
	int argc_rte = rte_eal_init(argc, argv);
	die(argc_rte >= 0);

	argc -= argc_rte;
	argv += argc_rte;

	static struct option long_options[] = {
		{ "send-first", no_argument, 0,  0 },
		{ NULL, 0, 0, 0 }
	};

	bool send_first = false;

	char opt;
	int option_index;

	while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
		die(opt == 0);

		switch (option_index) {
			case 0:
				send_first = true;
				break;

			default:
				die(false);
				break;
		}
	}

	struct rte_eth_conf dev_conf;
	memset(&dev_conf, 0, sizeof(struct rte_eth_conf));
        dev_conf.rxmode.mtu = MTU;

	die(rte_eth_dev_configure(IFACE_ID, NB_RX_QUEUES, NB_TX_QUEUES, &dev_conf) >= 0);

	struct rte_mempool *pool_rx = rte_pktmbuf_pool_create("dpdk-lab_pool_rx", RX_NB_MBUF, MEMPOOL_CACHE_SIZE, 0, MBUF_SIZE, rte_socket_id());
	die(pool_rx != NULL);

	struct rte_mempool *pool_tx = rte_pktmbuf_pool_create("dpdk-lab_pool_tx", TX_NB_MBUF, MEMPOOL_CACHE_SIZE, 0, MBUF_SIZE, rte_socket_id());
	die(pool_tx != NULL);

	die(rte_eth_rx_queue_setup(IFACE_ID, RX_QUEUE_ID, NB_RX_DESCRIPTORS, rte_eth_dev_socket_id(IFACE_ID), NULL, pool_rx) >= 0);
	die(rte_eth_tx_queue_setup(IFACE_ID, TX_QUEUE_ID, NB_TX_DESCRIPTORS, rte_eth_dev_socket_id(IFACE_ID), NULL) >= 0);

	die(rte_eth_dev_start(IFACE_ID) >= 0);

	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	die(fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) >= 0);

	printf("App started. Press ENTER to stop...\n");

	if (send_first) {
		struct rte_mbuf *mb = rte_pktmbuf_alloc(pool_tx);
		die(mb != NULL);
		while (!rte_eth_tx_burst(IFACE_ID, TX_QUEUE_ID, &mb, 1));
	}

	while (true) {
		struct rte_mbuf *mb;
		uint16_t cnt = rte_eth_rx_burst(IFACE_ID, RX_QUEUE_ID, &mb, 1);
		if (cnt == 1) {
			printf("Received one packet.\n");
			sleep(1);
		}

		char c;
		if (read(STDIN_FILENO, &c, 1) > 0)
			break;

	}

	return 0;
}
