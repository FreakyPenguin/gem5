#!/bin/sh
docker run --rm --name gem5 -it -v `pwd`:/gem5 -w /gem5 gem5-runner build/X86/gem5.opt --debug-flags=PciDevice,Ethernet configs/full_system/run.py --script=test.rcS
