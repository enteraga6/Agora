#!/bin/bash -x

docker run --privileged --network udp_network --ip 192.168.10.10 --hostname upd_server --name udp_server --rm -it --ipc shareable udp_server