// license: BSD-3-Clause
// copyright-holders: Angelo Salese

#ifndef MAME_MACHINE_SIS630_HOST_H
#define MAME_MACHINE_SIS630_HOST_H

#pragma once

#include "pci.h"
//#include "sis630_gui.h"

class sis630_host_device : public pci_host_device 
{
public:
	sis630_host_device(
		const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock, 
		const char *cpu_tag, int ram_size
	) : sis630_host_device(mconfig, tag, owner, clock)
	{
		// Revision 1 -> A1
		set_ids(0x10390630, 0x01, 0x060000, 0x00);
		//set_multifunction_device(true);
		m_host_cpu.set_tag(cpu_tag);
		set_ram_size(ram_size);
	}

	sis630_host_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	void set_ram_size(int ram_size) { m_ram_size = ram_size; }


protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;

	virtual bool map_first() const override { return true; }

//	virtual void reset_all_mappings() override;

	virtual void map_extra(uint64_t memory_window_start, uint64_t memory_window_end, uint64_t memory_offset, address_space *memory_space,
						   uint64_t io_window_start, uint64_t io_window_end, uint64_t io_offset, address_space *io_space) override;

	virtual void config_map(address_map &map) override;
	
	void memory_map(address_map &map);
	void io_map(address_map &map);

private:
	required_device<cpu_device> m_host_cpu;
	std::vector<uint32_t> m_ram;

	void map_shadowram(address_space *memory_space, offs_t start_offs, offs_t end_offs, bool read_enable, bool write_enable);
	
	int m_ram_size = 0;
	u32 m_gfx_window_base = 0;
	u8 m_dram_status = 0;
	u32 m_shadow_ram_ctrl = 0;
	u8 m_agp_mailbox[12]{};
	u8 m_smram = 0;
	u32 m_agp_priority_timer = 0;

	u8 dram_status_r();
	void dram_status_w(u8 data);

	u32 gfx_window_base_r(offs_t offset, uint32_t mem_mask = ~0);
	void gfx_window_base_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	
	u32 shadow_ram_ctrl_r(offs_t offset, uint32_t mem_mask = ~0);
	void shadow_ram_ctrl_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);

	u8 smram_r();
	void smram_w(u8 data);

	u32 agp_priority_timer_r(offs_t offset, uint32_t mem_mask = ~0);
	void agp_priority_timer_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);

	u8 agp_mailbox_r(offs_t offset);
	void agp_mailbox_w(offs_t offset, u8 data);

	u32 agp_id_r(offs_t offset, uint32_t mem_mask = ~0);
	u32 agp_status_r(offs_t offset, uint32_t mem_mask = ~0);

	virtual uint8_t capptr_r() override;
	
	u8 unmap_log_r(offs_t offset);
	void unmap_log_w(offs_t offset, u8 data);
};

DECLARE_DEVICE_TYPE(SIS630_HOST, sis630_host_device)


#endif
