//  Program:      nes-py
//  File:         emulator.cpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "emulator.hpp"
#include "log.hpp"

Emulator::Emulator(std::string rom_path) {
    // set the read callbacks
    bus.set_read_callback(PPUSTATUS, [&](void) {return ppu.getStatus();});
    bus.set_read_callback(PPUDATA, [&](void) {return ppu.getData(picture_bus);});
    bus.set_read_callback(JOY1, [&](void) {return controller1.read();});
    bus.set_read_callback(JOY2, [&](void) {return controller2.read();});
    bus.set_read_callback(OAMDATA, [&](void) {return ppu.getOAMData();});
    // set the write callbacks
    bus.set_write_callback(PPUCTRL, [&](NES_Byte b) {ppu.control(b);});
    bus.set_write_callback(PPUMASK, [&](NES_Byte b) {ppu.setMask(b);});
    bus.set_write_callback(OAMADDR, [&](NES_Byte b) {ppu.setOAMAddress(b);});
    bus.set_write_callback(PPUADDR, [&](NES_Byte b) {ppu.setDataAddress(b);});
    bus.set_write_callback(PPUSCROL, [&](NES_Byte b) {ppu.setScroll(b);});
    bus.set_write_callback(PPUDATA, [&](NES_Byte b) {ppu.setData(picture_bus, b);});
    bus.set_write_callback(OAMDMA, [&](NES_Byte b) {DMA(b);});
    bus.set_write_callback(JOY1, [&](NES_Byte b) {controller1.strobe(b); controller2.strobe(b);});
    bus.set_write_callback(OAMDATA, [&](NES_Byte b) {ppu.setOAMData(b);});
    // set the interrupt callback for the PPU
    ppu.setInterruptCallback([&](){ cpu.interrupt(bus, CPU::NMI_INTERRUPT); });
    // load the ROM from disk, expect that the Python code has validated it
    cartridge.loadFromFile(rom_path);
    // create the mapper based on the mapper ID in the iNES header of the ROM
    mapper = Mapper::create(cartridge, [&](){ picture_bus.update_mirroring(); });
    bus.set_mapper(mapper);
    picture_bus.set_mapper(mapper);
}

void Emulator::DMA(NES_Byte page) {
    // skip the DMA cycles on the CPU
    cpu.skipDMACycles();
    // do the DMA page change on the PPU
    ppu.doDMA(bus.get_page_pointer(page));
}

void Emulator::step(unsigned char action) {
    // write the controller state to player 1
    controller1.write_buttons(action);
    // approximate a frame
    for (int i = 0; i < STEPS_PER_FRAME; i++) {
        ppu.step(picture_bus);
        cpu.step(bus);
    }
}

void Emulator::backup() {
    backup_bus = bus;
    backup_picture_bus = picture_bus;
    backup_cpu = cpu;
    backup_ppu = ppu;
}

void Emulator::restore() {
    bus = backup_bus;
    picture_bus = backup_picture_bus;
    cpu = backup_cpu;
    ppu = backup_ppu;
}
