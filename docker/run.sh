#!/bin/sh
set -e
make -C software
docker run --rm --name gem5 -it -v `pwd`:/gem5 -w /gem5 --net=host --device=/dev/net/tun:/dev/net/tun gem5-runner docker/run_inner.sh
