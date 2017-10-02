#!/bin/sh
docker run -v `pwd`:/gem5 -w /gem5 gem5-builder scons build/X86/gem5.opt -j2
