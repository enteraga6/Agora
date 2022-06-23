#!/bin/bash -x

docker run --privileged --network udp_network --ip 192.168.10.11 --hostname udp_client --name udp_client --rm -it --ipc container:udp_server udp_client