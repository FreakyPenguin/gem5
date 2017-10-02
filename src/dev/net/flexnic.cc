/*
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
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

#include "dev/net/flexnic.hh"

#include <deque>
#include <limits>
#include <string>

#include "base/compiler.hh"
#include "base/debug.hh"
#include "base/inet.hh"
#include "base/types.hh"
#include "config/the_isa.hh"
#include "debug/EthernetAll.hh"
#include "dev/net/etherlink.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "sim/eventq.hh"
#include "sim/stats.hh"

using namespace std;
using namespace Net;
using namespace TheISA;

namespace FlexNIC {

///////////////////////////////////////////////////////////////////////
//
// Sinic PCI Device
//
Device::Device(const Params *p)
    : EtherDevBase(p)
{
    interface = new Interface(name() + ".int0", this);
    //reset();

}

Device::~Device()
{}


EtherInt*
Device::getEthPort(const std::string &if_name, int idx)
{
    if (if_name == name() + ".int0") {
        if (interface->getPeer())
            panic("interface already connected to\n");

        return interface;
    }
    return NULL;
}


/**
 * I/O read of device register
 */
Tick
Device::read(PacketPtr pkt)
{
    DPRINTF(Ethernet, "Receiving read(abs=%x, sz=%x, rel=%x)\n", pkt->getAddr(),
        pkt->getSize(), pkt->getAddr() - BARAddrs[0]);
#if 0
    assert(config.command & PCI_CMD_MSE);
    assert(pkt->getAddr() >= BARAddrs[0] && pkt->getSize() < BARSize[0]);

    ContextID cpu = pkt->req->contextId();
    Addr daddr = pkt->getAddr() - BARAddrs[0];
    Addr index = daddr >> Regs::VirtualShift;
    Addr raddr = daddr & Regs::VirtualMask;

    if (!regValid(raddr))
        panic("invalid register: cpu=%d vnic=%d da=%#x pa=%#x size=%d",
              cpu, index, daddr, pkt->getAddr(), pkt->getSize());

    const Regs::Info &info = regInfo(raddr);
    if (!info.read)
        panic("read %s (write only): "
              "cpu=%d vnic=%d da=%#x pa=%#x size=%d",
              info.name, cpu, index, daddr, pkt->getAddr(), pkt->getSize());

        panic("read %s (invalid size): "
              "cpu=%d vnic=%d da=%#x pa=%#x size=%d",
              info.name, cpu, index, daddr, pkt->getAddr(), pkt->getSize());

    prepareRead(cpu, index);

    uint64_t value M5_VAR_USED = 0;
    if (pkt->getSize() == 4) {
        uint32_t reg = regData32(raddr);
        pkt->set(reg);
        value = reg;
    }

    if (pkt->getSize() == 8) {
        uint64_t reg = regData64(raddr);
        pkt->set(reg);
        value = reg;
    }

    DPRINTF(EthernetPIO,
            "read %s: cpu=%d vnic=%d da=%#x pa=%#x size=%d val=%#x\n",
            info.name, cpu, index, daddr, pkt->getAddr(), pkt->getSize(), value);

    // reading the interrupt status register has the side effect of
    // clearing it
    if (raddr == Regs::IntrStatus)
        devIntrClear();

    return pioDelay;
#endif

    switch (pkt->getSize()) {
      case 1: pkt->set((uint8_t) 0); break;
      case 2: pkt->set((uint16_t) 0); break;
      case 4: pkt->set((uint32_t) 0); break;
      case 8: pkt->set((uint64_t) 0); break;
      default:
        panic("read: invalid size: %d\n", pkt->getSize());
    }

    pkt->makeAtomicResponse();

    return 0;
}

/**
 * I/O write of device register
 */
Tick
Device::write(PacketPtr pkt)
{
    DPRINTF(Ethernet, "Receiving write(abs=%x, sz=%x, rel=%x)\n", pkt->getAddr(),
        pkt->getSize(), pkt->getAddr() - BARAddrs[0]);


    uint64_t val;
    switch (pkt->getSize()) {
      case 1: val = pkt->get<uint8_t>(); break;
      case 2: val = pkt->get<uint16_t>(); break;
      case 4: val = pkt->get<uint32_t>(); break;
      case 8: val = pkt->get<uint64_t>(); break;
      default:
        panic("write: invalid size: %d\n", pkt->getSize());
    }

    pkt->makeAtomicResponse();

    DPRINTF(Ethernet, "Wrote: %llx\n", val);

/*
    assert(config.command & PCI_CMD_MSE);
    assert(pkt->getAddr() >= BARAddrs[0] && pkt->getSize() < BARSize[0]);

    ContextID cpu = pkt->req->contextId();
    Addr daddr = pkt->getAddr() - BARAddrs[0];
    Addr index = daddr >> Regs::VirtualShift;
    Addr raddr = daddr & Regs::VirtualMask;

    if (!regValid(raddr))
        panic("invalid register: cpu=%d, da=%#x pa=%#x size=%d",
                cpu, daddr, pkt->getAddr(), pkt->getSize());

    const Regs::Info &info = regInfo(raddr);
    if (!info.write)
        panic("write %s (read only): "
              "cpu=%d vnic=%d da=%#x pa=%#x size=%d",
              info.name, cpu, index, daddr, pkt->getAddr(), pkt->getSize());

    if (pkt->getSize() != info.size)
        panic("write %s (invalid size): "
              "cpu=%d vnic=%d da=%#x pa=%#x size=%d",
              info.name, cpu, index, daddr, pkt->getAddr(), pkt->getSize());

    VirtualReg &vnic = virtualRegs[index];

    DPRINTF(EthernetPIO,
            "write %s vnic %d: cpu=%d val=%#x da=%#x pa=%#x size=%d\n",
            info.name, index, cpu, info.size == 4 ? pkt->get<uint32_t>() :
            pkt->get<uint64_t>(), daddr, pkt->getAddr(), pkt->getSize());

    prepareWrite(cpu, index);

    switch (raddr) {
      case Regs::Config:
        changeConfig(pkt->get<uint32_t>());
        break;

      case Regs::Command:
        command(pkt->get<uint32_t>());
        break;

      case Regs::IntrStatus:
        devIntrClear(regs.IntrStatus & pkt->get<uint32_t>());
        break;

      case Regs::IntrMask:
        devIntrChangeMask(pkt->get<uint32_t>());
        break;

      case Regs::RxData:
        if (Regs::get_RxDone_Busy(vnic.RxDone))
            panic("receive machine busy with another request! rxState=%s",
                  RxStateStrings[rxState]);

        vnic.rxUnique = rxUnique++;
        vnic.RxDone = Regs::RxDone_Busy;
        vnic.RxData = pkt->get<uint64_t>();
        rxBusyCount++;

        if (Regs::get_RxData_Vaddr(pkt->get<uint64_t>())) {
            panic("vtophys not implemented in newmem");
#ifdef SINIC_VTOPHYS
            Addr vaddr = Regs::get_RxData_Addr(reg64);
            Addr paddr = vtophys(req->xc, vaddr);
            DPRINTF(EthernetPIO, "write RxData vnic %d (rxunique %d): "
                    "vaddr=%#x, paddr=%#x\n",
                    index, vnic.rxUnique, vaddr, paddr);

            vnic.RxData = Regs::set_RxData_Addr(vnic.RxData, paddr);
#endif
        } else {
            DPRINTF(EthernetPIO, "write RxData vnic %d (rxunique %d)\n",
                    index, vnic.rxUnique);
        }

        if (vnic.rxIndex == rxFifo.end()) {
            DPRINTF(EthernetPIO, "request new packet...appending to rxList\n");
            rxList.push_back(index);
        } else {
            DPRINTF(EthernetPIO, "packet exists...appending to rxBusy\n");
            rxBusy.push_back(index);
        }

        if (rxEnable && (rxState == rxIdle || rxState == rxFifoBlock)) {
            rxState = rxFifoBlock;
            rxKick();
        }
        break;

      case Regs::TxData:
        if (Regs::get_TxDone_Busy(vnic.TxDone))
            panic("transmit machine busy with another request! txState=%s",
                  TxStateStrings[txState]);

        vnic.txUnique = txUnique++;
        vnic.TxDone = Regs::TxDone_Busy;

        if (Regs::get_TxData_Vaddr(pkt->get<uint64_t>())) {
            panic("vtophys won't work here in newmem.\n");
#ifdef SINIC_VTOPHYS
            Addr vaddr = Regs::get_TxData_Addr(reg64);
            Addr paddr = vtophys(req->xc, vaddr);
            DPRINTF(EthernetPIO, "write TxData vnic %d (txunique %d): "
                    "vaddr=%#x, paddr=%#x\n",
                    index, vnic.txUnique, vaddr, paddr);

            vnic.TxData = Regs::set_TxData_Addr(vnic.TxData, paddr);
#endif
        } else {
            DPRINTF(EthernetPIO, "write TxData vnic %d (txunique %d)\n",
                    index, vnic.txUnique);
        }

        if (txList.empty() || txList.front() != index)
            txList.push_back(index);
        if (txEnable && txState == txIdle && txList.front() == index) {
            txState = txFifoBlock;
            txKick();
        }
        break;
    }

    return pioDelay;
*/
    return 0;
}

bool
Device::recvPacket(EthPacketPtr packet)
{
    DPRINTF(Ethernet, "Receiving packet from wire\n");
/*
    if (!rxEnable) {
        DPRINTF(Ethernet, "receive disabled...packet dropped\n");
        return true;
    }

    if (rxFilter(packet)) {
        DPRINTF(Ethernet, "packet filtered...dropped\n");
        return true;
    }

    if (rxFifo.size() >= regs.RxFifoHigh)
        devIntrPost(Regs::Intr_RxHigh);

    if (!rxFifo.push(packet)) {
        DPRINTF(Ethernet,
                "packet will not fit in receive buffer...packet dropped\n");
        return false;
    }

    // If we were at the last element, back up one ot go to the new
    // last element of the list.
    if (rxFifoPtr == rxFifo.end())
        --rxFifoPtr;

    devIntrPost(Regs::Intr_RxPacket);
    rxKick();
*/
    return true;
}

void
Device::transferDone()
{
    DPRINTF(Ethernet, "transfer complete\n");
/*
    if (txFifo.empty()) {
        DPRINTF(Ethernet, "transfer complete: txFifo empty...nothing to do\n");
        return;
    }

    DPRINTF(Ethernet, "transfer complete: data in txFifo...schedule xmit\n");

    reschedule(txEvent, clockEdge(Cycles(1)), true);
*/
}


//=====================================================================
//
//

void
Device::serialize(CheckpointOut &cp) const
{
    // Serialize the PciDevice base class
    PciDevice::serialize(cp);

#if 0
    int count;
    if (rxState == rxCopy)
        panic("can't serialize with an in flight dma request rxState=%s",
              RxStateStrings[rxState]);

    if (txState == txCopy)
        panic("can't serialize with an in flight dma request txState=%s",
              TxStateStrings[txState]);

    /*
     * Serialize the device registers that could be modified by the OS.
     */
    SERIALIZE_SCALAR(regs.Config);
    SERIALIZE_SCALAR(regs.IntrStatus);
    SERIALIZE_SCALAR(regs.IntrMask);
    SERIALIZE_SCALAR(regs.RxData);
    SERIALIZE_SCALAR(regs.TxData);

    /*
     * Serialize the virtual nic state
     */
    int virtualRegsSize = virtualRegs.size();
    SERIALIZE_SCALAR(virtualRegsSize);
    for (int i = 0; i < virtualRegsSize; ++i) {
        const VirtualReg *vnic = &virtualRegs[i];

        std::string reg = csprintf("vnic%d", i);
        paramOut(cp, reg + ".RxData", vnic->RxData);
        paramOut(cp, reg + ".RxDone", vnic->RxDone);
        paramOut(cp, reg + ".TxData", vnic->TxData);
        paramOut(cp, reg + ".TxDone", vnic->TxDone);

        bool rxPacketExists = vnic->rxIndex != rxFifo.end();
        paramOut(cp, reg + ".rxPacketExists", rxPacketExists);
        if (rxPacketExists) {
            int rxPacket = 0;
            auto i = rxFifo.begin();
            while (i != vnic->rxIndex) {
                assert(i != rxFifo.end());
                ++i;
                ++rxPacket;
            }

            paramOut(cp, reg + ".rxPacket", rxPacket);
            paramOut(cp, reg + ".rxPacketOffset", vnic->rxPacketOffset);
            paramOut(cp, reg + ".rxPacketBytes", vnic->rxPacketBytes);
        }
        paramOut(cp, reg + ".rxDoneData", vnic->rxDoneData);
    }

    int rxFifoPtr = -1;
    if (this->rxFifoPtr != rxFifo.end())
        rxFifoPtr = rxFifo.countPacketsBefore(this->rxFifoPtr);
    SERIALIZE_SCALAR(rxFifoPtr);

    SERIALIZE_SCALAR(rxActive);
    SERIALIZE_SCALAR(rxBusyCount);
    SERIALIZE_SCALAR(rxDirtyCount);
    SERIALIZE_SCALAR(rxMappedCount);

    VirtualList::const_iterator i, end;
    for (count = 0, i = rxList.begin(), end = rxList.end(); i != end; ++i)
        paramOut(cp, csprintf("rxList%d", count++), *i);
    int rxListSize = count;
    SERIALIZE_SCALAR(rxListSize);

    for (count = 0, i = rxBusy.begin(), end = rxBusy.end(); i != end; ++i)
        paramOut(cp, csprintf("rxBusy%d", count++), *i);
    int rxBusySize = count;
    SERIALIZE_SCALAR(rxBusySize);

    for (count = 0, i = txList.begin(), end = txList.end(); i != end; ++i)
        paramOut(cp, csprintf("txList%d", count++), *i);
    int txListSize = count;
    SERIALIZE_SCALAR(txListSize);

    /*
     * Serialize rx state machine
     */
    int rxState = this->rxState;
    SERIALIZE_SCALAR(rxState);
    SERIALIZE_SCALAR(rxEmpty);
    SERIALIZE_SCALAR(rxLow);
    rxFifo.serialize("rxFifo", cp);

    /*
     * Serialize tx state machine
     */
    int txState = this->txState;
    SERIALIZE_SCALAR(txState);
    SERIALIZE_SCALAR(txFull);
    txFifo.serialize("txFifo", cp);
    bool txPacketExists = txPacket != nullptr;
    SERIALIZE_SCALAR(txPacketExists);
    if (txPacketExists) {
        txPacket->serialize("txPacket", cp);
        SERIALIZE_SCALAR(txPacketOffset);
        SERIALIZE_SCALAR(txPacketBytes);
    }

    /*
     * If there's a pending transmit, store the time so we can
     * reschedule it later
     */
    Tick transmitTick = txEvent.scheduled() ? txEvent.when() - curTick() : 0;
    SERIALIZE_SCALAR(transmitTick);
#endif
}

void
Device::unserialize(CheckpointIn &cp)
{
    // Unserialize the PciDevice base class
    PciDevice::unserialize(cp);
#if 0
    /*
     * Unserialize the device registers that may have been written by the OS.
     */
    UNSERIALIZE_SCALAR(regs.Config);
    UNSERIALIZE_SCALAR(regs.IntrStatus);
    UNSERIALIZE_SCALAR(regs.IntrMask);
    UNSERIALIZE_SCALAR(regs.RxData);
    UNSERIALIZE_SCALAR(regs.TxData);

    UNSERIALIZE_SCALAR(rxActive);
    UNSERIALIZE_SCALAR(rxBusyCount);
    UNSERIALIZE_SCALAR(rxDirtyCount);
    UNSERIALIZE_SCALAR(rxMappedCount);

    int rxListSize;
    UNSERIALIZE_SCALAR(rxListSize);
    rxList.clear();
    for (int i = 0; i < rxListSize; ++i) {
        int value;
        paramIn(cp, csprintf("rxList%d", i), value);
        rxList.push_back(value);
    }

    int rxBusySize;
    UNSERIALIZE_SCALAR(rxBusySize);
    rxBusy.clear();
    for (int i = 0; i < rxBusySize; ++i) {
        int value;
        paramIn(cp, csprintf("rxBusy%d", i), value);
        rxBusy.push_back(value);
    }

    int txListSize;
    UNSERIALIZE_SCALAR(txListSize);
    txList.clear();
    for (int i = 0; i < txListSize; ++i) {
        int value;
        paramIn(cp, csprintf("txList%d", i), value);
        txList.push_back(value);
    }

    /*
     * Unserialize rx state machine
     */
    int rxState;
    UNSERIALIZE_SCALAR(rxState);
    UNSERIALIZE_SCALAR(rxEmpty);
    UNSERIALIZE_SCALAR(rxLow);
    this->rxState = (RxState) rxState;
    rxFifo.unserialize("rxFifo", cp);

    int rxFifoPtr;
    UNSERIALIZE_SCALAR(rxFifoPtr);
    if (rxFifoPtr >= 0) {
        this->rxFifoPtr = rxFifo.begin();
        for (int i = 0; i < rxFifoPtr; ++i)
            ++this->rxFifoPtr;
    } else {
        this->rxFifoPtr = rxFifo.end();
    }

    /*
     * Unserialize tx state machine
     */
    int txState;
    UNSERIALIZE_SCALAR(txState);
    UNSERIALIZE_SCALAR(txFull);
    this->txState = (TxState) txState;
    txFifo.unserialize("txFifo", cp);
    bool txPacketExists;
    UNSERIALIZE_SCALAR(txPacketExists);
    txPacket = 0;
    if (txPacketExists) {
        txPacket = make_shared<EthPacketData>(16384);
        txPacket->unserialize("txPacket", cp);
        UNSERIALIZE_SCALAR(txPacketOffset);
        UNSERIALIZE_SCALAR(txPacketBytes);
    }

    /*
     * unserialize the virtual nic registers/state
     *
     * this must be done after the unserialization of the rxFifo
     * because the packet iterators depend on the fifo being populated
     */
    int virtualRegsSize;
    UNSERIALIZE_SCALAR(virtualRegsSize);
    virtualRegs.clear();
    virtualRegs.resize(virtualRegsSize);
    for (int i = 0; i < virtualRegsSize; ++i) {
        VirtualReg *vnic = &virtualRegs[i];
        std::string reg = csprintf("vnic%d", i);

        paramIn(cp, reg + ".RxData", vnic->RxData);
        paramIn(cp, reg + ".RxDone", vnic->RxDone);
        paramIn(cp, reg + ".TxData", vnic->TxData);
        paramIn(cp, reg + ".TxDone", vnic->TxDone);

        vnic->rxUnique = rxUnique++;
        vnic->txUnique = txUnique++;

        bool rxPacketExists;
        paramIn(cp, reg + ".rxPacketExists", rxPacketExists);
        if (rxPacketExists) {
            int rxPacket;
            paramIn(cp, reg + ".rxPacket", rxPacket);
            vnic->rxIndex = rxFifo.begin();
            while (rxPacket--)
                ++vnic->rxIndex;

            paramIn(cp, reg + ".rxPacketOffset",
                    vnic->rxPacketOffset);
            paramIn(cp, reg + ".rxPacketBytes", vnic->rxPacketBytes);
        } else {
            vnic->rxIndex = rxFifo.end();
        }
        paramIn(cp, reg + ".rxDoneData", vnic->rxDoneData);
    }

    /*
     * If there's a pending transmit, reschedule it now
     */
    Tick transmitTick;
    UNSERIALIZE_SCALAR(transmitTick);
    if (transmitTick)
        schedule(txEvent, curTick() + transmitTick);

    pioPort.sendRangeChange();
#endif
}

} // namespace Sinic

FlexNIC::Device *
FlexNICParams::create()
{
    return new FlexNIC::Device(this);
}
