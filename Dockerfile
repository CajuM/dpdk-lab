FROM ubuntu:23.10

RUN \
	export DEBIAN_FRONTEND=noninteractive && \
	apt-get update && \
	apt-get -y dist-upgrade && \
	apt-get -y install wget gcc pkg-config meson python3-pyelftools && \
	wget https://fast.dpdk.org/rel/dpdk-23.07.tar.xz && \
	tar xvf dpdk-23.07.tar.xz && \
	cd dpdk-23.07 && \
	meson setup -Denable_drivers=net/memif -Dmachine=generic -Dmax_numa_nodes=1 --prefix=/usr build && \
	cd build && \
	ninja && \
	meson install
