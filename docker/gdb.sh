#!/bin/sh
docker exec -it gem5 gdb -ex 'target remote localhost:7000' configs/full_system/vmlinux
