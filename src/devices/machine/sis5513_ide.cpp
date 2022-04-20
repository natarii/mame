// license: BSD-3-Clause
// copyright-holders: Angelo Salese
/**************************************************************************************************

	SiS 5513 IDE controller

    TODO:
	- Stub interface, to be improved;

**************************************************************************************************/

#include "emu.h"
#include "sis5513_ide.h"

#define LOG_IO     (1U << 1) // log PCI register accesses
#define LOG_TODO   (1U << 2) // log unimplemented registers
#define LOG_MAP    (1U << 3) // log full remaps

#define VERBOSE (LOG_GENERAL | LOG_IO | LOG_TODO | LOG_MAP)
//#define LOG_OUTPUT_FUNC osd_printf_warning

#include "logmacro.h"

#define LOGIO(...)     LOGMASKED(LOG_IO,   __VA_ARGS__)
#define LOGMAP(...)    LOGMASKED(LOG_MAP,  __VA_ARGS__)
#define LOGTODO(...)   LOGMASKED(LOG_TODO, __VA_ARGS__)

DEFINE_DEVICE_TYPE(SIS5513_IDE, sis5513_ide_device, "sis5513_ide", "SiS 5513 IDE Controller")

sis5513_ide_device::sis5513_ide_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: pci_device(mconfig, SIS5513_IDE, tag, owner, clock)
	, m_ide1(*this, "ide1")
	, m_ide2(*this, "ide2")
	, m_irq_pri_callback(*this)
	, m_irq_sec_callback(*this)
{
	// IDE controller with 0xd0 as programming i/f ATA Host Adapters standard
	set_ids(0x10395513, 0xd0, 0x010100, 0x00);
}

void sis5513_ide_device::device_add_mconfig(machine_config &config)
{
	BUS_MASTER_IDE_CONTROLLER(config, m_ide1).options(ata_devices, "hdd", nullptr, false);
	m_ide1->irq_handler().set([this](int state) { m_irq_pri_callback(state); });
	m_ide1->set_bus_master_space(":maincpu", AS_PROGRAM);

	BUS_MASTER_IDE_CONTROLLER(config, m_ide2).options(ata_devices, "cdrom", nullptr, false);
	m_ide1->irq_handler().set([this](int state) { m_irq_sec_callback(state); });
	m_ide2->set_bus_master_space(":maincpu", AS_PROGRAM);
}

void sis5513_ide_device::config_map(address_map &map)
{
	pci_device::config_map(map);
	map(0x10, 0x4f).unmaprw();
	map(0x10, 0x52).rw(FUNC(sis5513_ide_device::unmap_log_r), FUNC(sis5513_ide_device::unmap_log_w));

	// base I/O relocation (effective only when native mode is on)
//	map(0x10, 0x13) Primary channel command base
//	map(0x14, 0x17) Primary channel control base
//	map(0x18, 0x1b) Secondary channel command base
//	map(0x1c, 0x1f) Secondary channel control base

//	map(0x20, 0x23) Bus master IDE control register base (0x10 I/O regs)

//	map(0x2c, 0x2f) subsystem ID (written once)

//	map(0x3c, 0x3d) interrupt line/pin

//	map(0x40, 0x40) IDE Primary channel master data recovery time
//	map(0x41, 0x41) IDE Primary channel master data active time (Ultra DMA)
//	map(0x42, 0x42) IDE Primary channel slave data recovery time
//	map(0x43, 0x43) IDE Primary channel slave data active time (Ultra DMA)
//	map(0x44, 0x47) ^ Same for IDE Secondary

//	map(0x48, 0x48) IDE status
//	map(0x4a, 0x4b) IDE general control regs

//	map(0x4c, 0x4d) prefetch count of primary channel
//	map(0x4e, 0x4f) prefetch count of secondary channel

//	map(0x52, 0x52) IDE misc control regs
}

void sis5513_ide_device::io_map(address_map &map)
{
	map(0x0170, 0x0177).rw(FUNC(sis5513_ide_device::ide2_read32_cs0_r), FUNC(sis5513_ide_device::ide2_write32_cs0_w));
	map(0x01f0, 0x01f7).rw(FUNC(sis5513_ide_device::ide1_read32_cs0_r), FUNC(sis5513_ide_device::ide1_write32_cs0_w));
	map(0x0376, 0x0376).rw(FUNC(sis5513_ide_device::ide2_read_cs1_r), FUNC(sis5513_ide_device::ide2_write_cs1_w));
	map(0x03f6, 0x03f6).rw(FUNC(sis5513_ide_device::ide1_read_cs1_r), FUNC(sis5513_ide_device::ide1_write_cs1_w));
}

void sis5513_ide_device::map_extra(uint64_t memory_window_start, uint64_t memory_window_end, uint64_t memory_offset, address_space *memory_space,
							uint64_t io_window_start, uint64_t io_window_end, uint64_t io_offset, address_space *io_space)
{
	io_space->install_device(0, 0x03ff, *this, &sis5513_ide_device::io_map);
}

void sis5513_ide_device::device_start()
{
	pci_device::device_start();

	m_irq_pri_callback.resolve();
	m_irq_sec_callback.resolve();
#if 0
	memory_window_start = 0;
	memory_window_end   = 0xffffffff;
	memory_offset       = 0;
	io_window_start = 0;
	io_window_end   = 0xffff;
	io_offset       = 0;
#endif
}


void sis5513_ide_device::device_reset()
{
	pci_device::device_reset();
	
	command = 0x0000;
	status = 0x0000;
}

/*
 * Debugging
 */

u8 sis5513_ide_device::unmap_log_r(offs_t offset)
{
	LOGTODO("IDE Unemulated [%02x] R\n", offset + 0x10);
	return 0;
}

void sis5513_ide_device::unmap_log_w(offs_t offset, u8 data)
{
	LOGTODO("IDE Unemulated [%02x] %02x W\n", offset + 0x10, data);
}

/*
 * Start of legacy handling, to be moved out
 */

uint32_t sis5513_ide_device::ide1_read32_cs0_r(offs_t offset, uint32_t mem_mask)
{
	if (!(command & 1))
		return 0xffffffff;
	return m_ide1->read_cs0(offset, mem_mask);
}

void sis5513_ide_device::ide1_write32_cs0_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	if (!(command & 1))
		return;
	m_ide1->write_cs0(offset, data, mem_mask);
}

uint32_t sis5513_ide_device::ide2_read32_cs0_r(offs_t offset, uint32_t mem_mask)
{
	if (!(command & 1))
		return 0xffffffff;
	return m_ide2->read_cs0(offset, mem_mask);
}

void sis5513_ide_device::ide2_write32_cs0_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	if (!(command & 1))
		return;
	m_ide2->write_cs0(offset, data, mem_mask);
}

uint8_t sis5513_ide_device::ide1_read_cs1_r()
{
	if (!(command & 1))
		return 0xff;
	return m_ide1->read_cs1(1, 0xff0000) >> 16;
}

void sis5513_ide_device::ide1_write_cs1_w(uint8_t data)
{
	if (!(command & 1))
		return;
	m_ide1->write_cs1(1, data << 16, 0xff0000);
}

uint8_t sis5513_ide_device::ide2_read_cs1_r()
{
	if (!(command & 1))
		return 0xff;
	return m_ide2->read_cs1(1, 0xff0000) >> 16;
}

void sis5513_ide_device::ide2_write_cs1_w(uint8_t data)
{
	if (!(command & 1))
		return;
	m_ide2->write_cs1(1, data << 16, 0xff0000);
}
