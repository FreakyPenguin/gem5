/*
 * Copyright (c) 2017 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Antoine Kaufmann
 */

#ifndef __DEV_NET_FLEXNIC_HH__
#define __DEV_NET_FLEXNIC_HH__

#include "base/inet.hh"
#include "base/statistics.hh"
#include "dev/io_device.hh"
#include "dev/net/etherdevice.hh"
#include "dev/net/etherint.hh"
#include "dev/net/etherpkt.hh"
#include "dev/net/pktfifo.hh"
#include "dev/pci/device.hh"
#include "params/FlexNIC.hh"
#include "sim/eventq.hh"

/*
 * Memory layout for BAR0:
 *  - 0: Global registers
 *    - 0x000 4B RO: number of doorbells
 *    - 0x004 4B RO: doorbell offset
 *    - 0x008 4B RO: size of internal memory
 *    - 0x00c 4B RO: internal memory offset
 *    - 0x010 4B RW: pipeline allocation mask RX
 *    - 0x014 4B RW: pipeline allocation mask DB
 *  - 4KB + i * 4KB: Doorbells
 *    - Writes are only supported to the base address of the doorbell, in
 *      sizes: 1, 2, 4, 8, 16, 32, 64B
 *  - 4KB * (NUM_DOORBELLS + 1): Internal memory
 */

namespace FlexNIC {

class Interface;

typedef void (*PipelineFun)(void *packet, void *metadata);

class Device : public EtherDevBase
{
/**
 * device ethernet interface
 */
  protected:
    Interface *interface;
  public:
    bool recvPacket(EthPacketPtr packet);
    void transferDone();
    EtherInt *getEthPort(const std::string &if_name, int idx) override;

/**
 * Memory Interface
 */
  protected:
    std::string pcfgPath;
    void *pcfgHandle;
    PipelineFun pcfgFunRx;
    PipelineFun pcfgFunTx;
    PipelineFun pcfgFunDb;
    PipelineFun pcfgFunDma;

    uint16_t doorbellsNum;
    uint32_t doorbellsOff;
    uint32_t internalMemOff;
    uint32_t internalMemSize;
    uint8_t *internalMem;
    uint8_t plAllocRx;
    uint8_t plAllocDb;

    Tick pioDoorbellDelay;
    Tick pioRegReadDelay;
    Tick pioRegWriteDelay;
    Tick pioMemReadDelay;
    Tick pioMemWriteDelay;

    void doorbellWrite(uint16_t dbIdx, uint64_t val);
  public:
    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;

/**
 * Serialization stuff
 */
  public:
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

  public:
    typedef FlexNICParams Params;
    const Params *params() const {
        return dynamic_cast<const Params *>(_params);
    }

    Device(const Params *p);
    ~Device();
};

/*
 * Ethernet Interface for an Ethernet Device
 */
class Interface : public EtherInt
{
  private:
    Device *dev;

  public:
    Interface(const std::string &name, Device *d)
        : EtherInt(name), dev(d)
    { }

    virtual bool recvPacket(EthPacketPtr pkt) { return dev->recvPacket(pkt); }
    virtual void sendDone() { dev->transferDone(); }
};

} // namespace FlexNIC

#endif // __DEV_NET_FLEXNIC_HH__
