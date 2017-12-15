#!/bin/sh
build/X86/gem5.opt --debug-flags=PciDevice,Ethernet configs/full_system/run.py --script=software/test.rcS
