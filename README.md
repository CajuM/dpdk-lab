# Introduction

DPDK(Data Plane Development Kit) is a set of user-space libraries that can be used to write high-performance network applications. Its primary advantage over kernel-space network drivers is that it can use SIMD instruction that are not available in kernel-space. SIMD instructions give applications a performance boost when copying memory buffers such as packet descriptors. Another advantage being that one does not need to develop a kernel module for their network protocols and eases protocol and application development and adoption. In order to bypass the kernel's networking stack, the NICs that are to be used must be bound to a driver that allows user-space applications to perform directly read and write PCI memory areas. DPDK's implements NIC drivers as PMDs(Poll Mode Driver) so called because on the receive side, the application continuously polls the NICs receive ring for available packets as opposed to reading packet descriptors asynchronously on events such as interrupts. This gives it a performance advantage in high performance applications due to the absence of a scheduling over-head. Thus a single thread consumes an entire core's execution time processing incoming packets. Packet transmission is non-blocking and asynchronous, thus the application enqueues packets in the NICs queues and transmits further packets when new TX descriptors are available.

# Prerequisites

## Operating System and Software
This lab requires a working installation of Linux, either on a physical machine or on a virtual one. If you do not have a working installation already, consult[^fn2] for an easy way of running Ubuntu VMs.

The only required user-space software is a working docker installation with support for docker-compose v2. On Ubuntu, docker can be installed by running `apt-get -y install docker-compose-v2`. For other distributions consult[^fn3] or their package manager's manual and package repositories.

## Kernel Parameters
DPDK requires pages of 2MiB or 1GiB to be available to the operating system. In order to allocate such pages the Linux kernel's command line needs to be changed to include the arguments: `default_hugepagesz=2M hugepagesz=2M hugepages=1024`. This will allocate 2GiB of RAM into 2MiB pages instead of the default of 4KiB. The rationale for this requirement is that translating virtual addresses to physical ones has a computational and memory overhead caused by TLB(Translation Lookaside Buffer) misses. As such if we use 2MiB pages the TLB will allocate one entry for each page instead of 512 as for 4KiB pages. This has the effect of reducing the number of TLB misses. If we reduce the number of entries required, there will be fewer evictions. And for new entries we will have to walk the paging hierarchy 512 less times in the optimal case of continuous memory access.

A further optimization that, although not required, speeds-up DPDK-based applications is to allocate CPU cores exclusively for the application. The required kernel command line arguments are: `isolcpus=1-4 nohz_full=1-4 rcu_nocbs=1-4`. `isolcpus` and `nohz_full` will disable the preemptive multitasking tick while `rcu_nocbs` will offload certain interrupts to other cores. This ensures that a single task(thread) will run uninterrupted on a single core. In this case, we reserve the second through fifth cores, the first core which is the boot CPU can not be included in this list as it is required by the kernel.

For more details on kernel command line parameters see[^fn1].

In order to apply these chages, one usually modifies `/etc/default/grub` and changes the line `GRUB_CMDLINE_LINUX_DEFAULT="quiet spalsh"` to `GRUB_CMDLINE_LINUX_DEFAULT="default_hugepagesz=2M hugepagesz=2M hugepages=1024 isolcpus=1-4 nohz_full=1-4 rcu_nocbs=1-4 quiet splash"`, as an example. Afterwards GRUB's configuration must be updated, this can be done by running `sudo update-grub` on Ubuntu. For more details on changing the kernel's command line, consult your distribution's documentation.

[^fn1]: [The kernel's command-line parameters](https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html)
[^fn2]: [Install Multipass](https://multipass.run/install)
[^fn3]: [Install Docker Engine](https://docs.docker.com/engine/install/)

# Exercise 1

In the same directory as this file run `docker compose up`. This will launch two containers containing a DPDK installation.
After the containers have started-up launch an interactive shell in each of them: `docker compose exec n1 su -l` and `docker compose exec n2 su -l`.
Ensure that DPDK works by running a test application.

On n1: `dpdk-testpmd -l 1-2 --no-pci --file-prefix=n1 --vdev=net_memif,socket-abstract=no,role=server -- --auto-start --port-topology=loop --forward-mode=rxonly`.

On n2: `dpdk-testpmd -l 3-4 --no-pci --file-prefix=n2 --vdev=net_memif,socket-abstract=no,role=client -- --auto-start --port-topology=loop --forward-mode=txonly --tx-first`.

In order to stop the experiment, first press enter in n1's console and then in n2's. Check that under "Forward statistics for port 0" n1 shows a number greater than zero for RX-packets and that TX-packets is greater than zero for n2.

dpdk-testpmd is a test application provided by DPDK that functions as a virtual switch and simple packet generator. The `-l` flag informs the application about the cores that have been reserved for its use. The `--no-pci` flag disables the usage of PCI-based NIC drivers. The `--file-prefix` flag is provided such that there are no filename collisions between temporary DPDK files. The `--vdev` flag creates a memif virtual interface that communicates through a UNIX domain socket and shared memory, the socket must not be abstract as the containers have different namespaces, n1 will act as the server, while n2 will act as the client. These flags are specific to the DPDK runtime(EAL) and can be provided to any application that initializes itself with `rte_eal_init`. The flags following `--` are specific to the application, in this case it tells n1 to only receive packets and n2 to indefinitely send packets.

For more information on dpdk-testpmd's command line arguments consult[^fn4]. For EAL command line arguments consult[^fn5]. For information on memif consult[^fn6].

[^fn4]: [Running the Application](https://doc.dpdk.org/guides/testpmd_app_ug/run_app.html)
[^fn5]: [EAL parameters](https://doc.dpdk.org/guides/linux_gsg/linux_eal_parameters.html)
[^fn6]: [Memif Poll Mode Driver](https://doc.dpdk.org/guides/nics/memif.html)

# Exercise 2

For this exercise you must build and run the application that is found in the app folder. You will do so in the containerised environment as it has already been prepared for this exercise. The application's source code and previously provided instructions should be sufficient to solve this exercise. As an observation, the application requires just one core to function instead of two, like testpmd.

# Exercise 3

For this exercise you must modify the application such that it sends back the received packet. No functions other than those already used in the program need to be called. For further documentation consult[^fn7].

[^fn7]: [DPDK: API](https://doc.dpdk.org/api/)

# Exercise 4

For this exercise you must modify the application such that the first sent packet contains a sequence number starting from `0`. On each received packet the application must increment the sequence number in the packet before sending it back. The application will then expect a reply packet with the appropriate sequence number. If the sequence number does not match the expected value, the application must terminate immediately with an error message. You must also take into account that upon allocation mbuf's have a data area of zero length.

# Bonus 1

Answer the following questions:
* Why is it not required to free allocated packets?
* Why are there no out of order or dropped packets?

# Bonus 2

Remove the `sleep` and the previous `printf` from the application and display the number of packets received in one second on application exit.
