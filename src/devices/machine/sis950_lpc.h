// license: BSD-3-Clause
// copyright-holders: Angelo Salese

#ifndef MAME_MACHINE_SIS950_LPC_H
#define MAME_MACHINE_SIS950_LPC_H

#pragma once

#include "pci.h"

#include "bus/ata/ataintf.h"
#include "bus/isa/isa.h"
#include "bus/pc_kbd/pc_kbdc.h"
#include "lpc-acpi.h"
#include "pci-smbus.h"

#include "cpu/i386/i386.h"

#include "machine/am9517a.h"
#include "machine/at.h"
#include "machine/at_keybc.h"
#include "machine/ds128x.h"
#include "machine/ins8250.h"
//#include "machine/intelfsh.h"
#include "machine/pc_lpt.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/ram.h"
#include "machine/nvram.h"

#include "sound/spkrdev.h"


class sis950_lpc_device : public pci_device 
{
public:
	sis950_lpc_device(
		const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock, 
		const char *cpu_tag
	) : sis950_lpc_device(mconfig, tag, owner, clock)
	{
		// Revision 0 -> A0
		set_ids(0x10390008, 0x00, 0x060100, 0x00);
		//set_multifunction_device(true);
		m_host_cpu.set_tag(cpu_tag);
	}
	
	sis950_lpc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);


protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;

//	virtual void reset_all_mappings() override;

	virtual void map_extra(uint64_t memory_window_start, uint64_t memory_window_end, uint64_t memory_offset, address_space *memory_space,
						   uint64_t io_window_start, uint64_t io_window_end, uint64_t io_offset, address_space *io_space) override;

	virtual void config_map(address_map &map) override;
	
	void memory_map(address_map &map);
	void io_map(address_map &map);
	
private:
	required_device<cpu_device> m_host_cpu;
	required_device<pic8259_device> m_pic_master;
	required_device<pic8259_device> m_pic_slave;
	required_device<am9517a_device> m_dmac_master;
	required_device<am9517a_device> m_dmac_slave;
	required_device<pit8254_device> m_pit;
	required_device<isa16_device> m_isabus;
	required_device<ps2_keyboard_controller_device> m_keybc;
	required_device<speaker_sound_device> m_speaker;
	required_device<ds12885_device> m_rtc;
	required_device<pc_kbdc_device> m_pc_kbdc;
	required_device<lpc_acpi_device> m_acpi;
	required_device<smbus_device> m_smbus;

	// PCI interface
	u8 bios_control_r();
	void bios_control_w(u8 data);
	u8 flash_ctrl_r();
	void flash_ctrl_w(u8 data);
	u8 acpi_base_r();
	void acpi_base_w(u8 data);
	u8 init_enable_r();
	void init_enable_w(u8 data);
	u8 keybc_reg_r();
	void keybc_reg_w(u8 data);
	u8 unmap_log_r(offs_t offset);
	void unmap_log_w(offs_t offset, u8 data);
	
	u8 m_bios_control = 0;
	u8 m_flash_control = 0;
	u16 m_acpi_base = 0x0000;
	u8 m_init_reg = 0;
	u8 m_keybc_reg = 0;

	// LPC vendor specific, verify if it's common for all
	u8 lpc_fast_init_r();
	void lpc_fast_init_w(offs_t offset, u8 data);
	struct {
		u8 fast_init;
	} m_lpc_legacy;

	// SB implementation, to be moved out
	DECLARE_WRITE_LINE_MEMBER(pit_out0);
	DECLARE_WRITE_LINE_MEMBER(pit_out1);
	DECLARE_WRITE_LINE_MEMBER(pit_out2);
	uint8_t pc_dma8237_0_dack_r();
	uint8_t pc_dma8237_1_dack_r();
	uint8_t pc_dma8237_2_dack_r();
	uint8_t pc_dma8237_3_dack_r();
	uint8_t pc_dma8237_5_dack_r();
	uint8_t pc_dma8237_6_dack_r();
	uint8_t pc_dma8237_7_dack_r();
	void pc_dma8237_0_dack_w(uint8_t data);
	void pc_dma8237_1_dack_w(uint8_t data);
	void pc_dma8237_2_dack_w(uint8_t data);
	void pc_dma8237_3_dack_w(uint8_t data);
	void pc_dma8237_5_dack_w(uint8_t data);
	void pc_dma8237_6_dack_w(uint8_t data);
	void pc_dma8237_7_dack_w(uint8_t data);
	DECLARE_WRITE_LINE_MEMBER(at_dma8237_out_eop);
	DECLARE_WRITE_LINE_MEMBER(pc_dack0_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack1_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack2_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack3_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack4_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack5_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack6_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack7_w);
	uint8_t pc_dma_read_byte(offs_t offset);
	void pc_dma_write_byte(offs_t offset, uint8_t data);
	uint8_t pc_dma_read_word(offs_t offset);
	void pc_dma_write_word(offs_t offset, uint8_t data);
	DECLARE_WRITE_LINE_MEMBER(pc_dma_hrq_changed);
	DECLARE_WRITE_LINE_MEMBER(iochck_w);
	void pc_select_dma_channel(int channel, bool state);

	uint8_t m_at_pages[0x10]{};
	uint8_t m_dma_offset[2][4]{};
	uint8_t m_at_speaker = 0;
	uint8_t m_refresh = 0;
	bool m_pit_out2 = 0;
	bool m_at_spkrdata = 0;
	uint8_t m_channel_check = 0;
	int m_dma_channel = -1;
	bool m_cur_eop = false;
	uint16_t m_dma_high_byte = 0;

	DECLARE_WRITE_LINE_MEMBER(cpu_a20_w);
	DECLARE_WRITE_LINE_MEMBER(cpu_reset_w);
	
	uint8_t at_page8_r(offs_t offset);
	void at_page8_w(offs_t offset, uint8_t data);
	u8 at_portb_r();
	void at_portb_w(uint8_t data);

	void at_speaker_set_spkrdata(uint8_t data);
};

DECLARE_DEVICE_TYPE(SIS950_LPC, sis950_lpc_device)


#endif
