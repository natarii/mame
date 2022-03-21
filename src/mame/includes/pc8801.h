// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/********************************************************************************************

    PC-8801 (c) 1981 NEC

********************************************************************************************/

#ifndef MAME_INCLUDES_PC8801_H
#define MAME_INCLUDES_PC8801_H

#pragma once

#include "cpu/z80/z80.h"
#include "imagedev/cassette.h"
#include "imagedev/floppy.h"
#include "machine/i8214.h"
#include "machine/i8251.h"
#include "machine/i8255.h"
#include "machine/timer.h"
#include "machine/upd1990a.h"
#include "machine/pc80s31k.h"
#include "sound/beep.h"
#include "sound/ymopn.h"
#include "bus/centronics/ctronics.h"
#include "bus/pc8801/pc8801_31.h"
#include "emupal.h"
#include "screen.h"
#include "softlist.h"
#include "speaker.h"

#define I8214_TAG       "i8214"
#define UPD1990A_TAG    "upd1990a"
#define I8251_TAG       "i8251"

class pc8801_state : public driver_device
{
public:
	pc8801_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_screen(*this, "screen")
		, m_pc80s31(*this, "pc80s31")
		, m_pic(*this, I8214_TAG)
		, m_rtc(*this, UPD1990A_TAG)
		, m_cassette(*this, "cassette")
		, m_beeper(*this, "beeper")
		, m_lspeaker(*this, "lspeaker")
		, m_rspeaker(*this, "rspeaker")
		, m_palette(*this, "palette")
		, m_n80rom(*this, "n80rom")
		, m_n88rom(*this, "n88rom")
		, m_cg_rom(*this, "cgrom")
		, m_kanji_rom(*this, "kanji")
		, m_kanji_lv2_rom(*this, "kanji_lv2")
	{ }

	void pc8801(machine_config &config);

protected:
	virtual void video_start() override;
	virtual void machine_start() override;
	virtual void machine_reset() override;

	uint8_t mem_r(offs_t offset);
	void mem_w(offs_t offset, uint8_t data);

	virtual attotime mouse_limit_hz();

	virtual uint8_t dictionary_rom_r(offs_t offset);
	virtual bool dictionary_rom_enable();

	virtual uint8_t cdbios_rom_r(offs_t offset);
	virtual bool cdbios_rom_enable();
	virtual void main_io(address_map &map);

	required_device<cpu_device> m_maincpu;
	required_device<screen_device> m_screen;
	required_device<pc80s31_device> m_pc80s31;
	optional_device<i8214_device> m_pic;
	required_device<upd1990a_device> m_rtc;
	required_device<cassette_image_device> m_cassette;
	required_device<beep_device> m_beeper;
	required_device<speaker_device> m_lspeaker;
	required_device<speaker_device> m_rspeaker;
	required_device<palette_device> m_palette;
	required_region_ptr<u8> m_n80rom;
	required_region_ptr<u8> m_n88rom;
	required_region_ptr<u8> m_cg_rom;
	required_region_ptr<u8> m_kanji_rom;
	required_region_ptr<u8> m_kanji_lv2_rom;

	DECLARE_WRITE_LINE_MEMBER(int3_w);

	struct mouse_t
	{
		uint8_t phase;
		int8_t prev_dx, prev_dy;
		uint8_t lx, ly;
		attotime time;
	};

	mouse_t m_mouse;
	uint8_t m_gfx_ctrl;

private:
	void main_map(address_map &map);

	struct crtc_t
	{
		uint8_t cmd,param_count,cursor_on,status,irq_mask;
		uint8_t param[8][5];
		uint8_t inverse;
	};

	std::unique_ptr<uint8_t[]> m_work_ram;
	std::unique_ptr<uint8_t[]> m_hi_work_ram;
	std::unique_ptr<uint8_t[]> m_ext_work_ram;
	std::unique_ptr<uint8_t[]> m_gvram;

	uint8_t m_i8255_0_pc;
	uint8_t m_i8255_1_pc;
	uint8_t m_fdc_irq_opcode;
	uint8_t m_ext_rom_bank;
	uint8_t m_vram_sel;
	uint8_t m_misc_ctrl;
	uint8_t m_device_ctrl_data;
	uint8_t m_window_offset_bank;
	uint8_t m_layer_mask;
	uint16_t m_dma_counter[4];
	uint16_t m_dma_address[4];
	uint8_t m_alu_reg[3];
	uint8_t m_dmac_mode;
	uint8_t m_alu_ctrl1;
	uint8_t m_alu_ctrl2;
	uint8_t m_extram_mode;
	uint8_t m_extram_bank;
	uint8_t m_txt_width;
	uint8_t m_txt_color;

	crtc_t m_crtc;
	struct { uint8_t r, g, b; } m_palram[8];
	uint8_t m_dmac_ff;
	uint32_t m_knj_addr[2];
	uint32_t m_extram_size;

	uint8_t alu_r(offs_t offset);
	void alu_w(offs_t offset, uint8_t data);
	uint8_t wram_r(offs_t offset);
	void wram_w(offs_t offset, uint8_t data);
	uint8_t ext_wram_r(offs_t offset);
	void ext_wram_w(offs_t offset, uint8_t data);
	uint8_t nbasic_rom_r(offs_t offset);
	uint8_t n88basic_rom_r(offs_t offset);
	uint8_t gvram_r(offs_t offset);
	void gvram_w(offs_t offset, uint8_t data);
	uint8_t high_wram_r(offs_t offset);
	void high_wram_w(offs_t offset, uint8_t data);
	uint8_t ext_rom_bank_r();
	void ext_rom_bank_w(uint8_t data);
	void port30_w(uint8_t data);
	void port31_w(uint8_t data);
	uint8_t port40_r();
	void port40_w(uint8_t data);
	uint8_t vram_select_r();
	void vram_select_w(offs_t offset, uint8_t data);
	void irq_level_w(uint8_t data);
	void irq_mask_w(uint8_t data);
	uint8_t window_bank_r();
	void window_bank_w(uint8_t data);
	void window_bank_inc_w(uint8_t data);
	uint8_t misc_ctrl_r();
	void misc_ctrl_w(uint8_t data);
	void bgpal_w(uint8_t data);
	void palram_w(offs_t offset, uint8_t data);
	void layer_masking_w(uint8_t data);
	uint8_t crtc_param_r();
	void crtc_param_w(uint8_t data);
	uint8_t crtc_status_r();
	void crtc_cmd_w(uint8_t data);
	uint8_t dmac_r(offs_t offset);
	void dmac_w(offs_t offset, uint8_t data);
	uint8_t dmac_status_r();
	void dmac_mode_w(uint8_t data);
	uint8_t extram_mode_r();
	void extram_mode_w(uint8_t data);
	uint8_t extram_bank_r();
	void extram_bank_w(uint8_t data);
	void alu_ctrl1_w(uint8_t data);
	void alu_ctrl2_w(uint8_t data);
	void pcg8100_w(offs_t offset, uint8_t data);
	template <unsigned kanji_level> uint8_t kanji_r(offs_t offset);
	template <unsigned kanji_level> void kanji_w(offs_t offset, uint8_t data);
	void rtc_w(uint8_t data);
	DECLARE_WRITE_LINE_MEMBER(txdata_callback);
	DECLARE_WRITE_LINE_MEMBER(rxrdy_w);
//	uint8_t sound_board_r(offs_t offset);
//	void sound_board_w(offs_t offset, uint8_t data);
//	uint8_t opna_r(offs_t offset);
//	void opna_w(offs_t offset, uint8_t data);

	uint8_t pixel_clock(void);
	void dynamic_res_change(void);
	void draw_bitmap_3bpp(bitmap_ind16 &bitmap,const rectangle &cliprect);
	void draw_bitmap_1bpp(bitmap_ind16 &bitmap,const rectangle &cliprect);
	uint8_t calc_cursor_pos(int x,int y,int yi);
	uint8_t extract_text_attribute(uint32_t address,int x, uint8_t width, uint8_t &non_special);
	void draw_char(bitmap_ind16 &bitmap,int x,int y,int pal,uint8_t gfx_mode,uint8_t reverse,uint8_t secret,
							uint8_t blink,uint8_t upper,uint8_t lower,int y_size,int width, uint8_t non_special);
	void draw_text(bitmap_ind16 &bitmap,int y_size, uint8_t width);

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void palette_reset();

	DECLARE_MACHINE_RESET(pc8801_dic);
	DECLARE_MACHINE_RESET(pc8801_cdrom);
	INTERRUPT_GEN_MEMBER(vrtc_irq);
	TIMER_DEVICE_CALLBACK_MEMBER(timer_irq);
	IRQ_CALLBACK_MEMBER(int_ack_cb);
	DECLARE_WRITE_LINE_MEMBER(irq_w);
	bool m_vrtc_irq_mask;
	bool m_timer_irq_mask;
	bool m_sound_irq_mask;
//	bool m_sound_irq_pending;

	uint8_t cpu_8255_c_r();
	void cpu_8255_c_w(uint8_t data);
	uint8_t fdc_8255_c_r();
	void fdc_8255_c_w(uint8_t data);
};

class pc8801mk2sr_state : public pc8801_state
{
public:
	pc8801mk2sr_state(const machine_config &mconfig, device_type type, const char *tag)
		: pc8801_state(mconfig, type, tag)
		, m_opn(*this, "opn")
	{ }

	void pc8801mk2sr(machine_config &config);
	void pc8801mk2mr(machine_config &config);

protected:
	virtual void main_io(address_map &map) override;

	uint8_t opn_porta_r();

private:
	optional_device<ym2203_device> m_opn;
};

// both FH and MH family bases sports selectable 8/4 MHz CPU clock switch
class pc8801fh_state : public pc8801mk2sr_state
{
public:
	pc8801fh_state(const machine_config &mconfig, device_type type, const char *tag)
		: pc8801mk2sr_state(mconfig, type, tag)
		, m_opna(*this, "opna")
	{ }

	void pc8801fh(machine_config &config);

protected:
	virtual void machine_reset() override;
	virtual void main_io(address_map &map) override;

	virtual attotime mouse_limit_hz() override;

private:
	required_device<ym2608_device> m_opna;
	void opna_map(address_map &map);

	uint8_t cpuclock_r();
	uint8_t baudrate_r();
	void baudrate_w(uint8_t data);

	uint8_t m_clock_setting;
	uint8_t m_baudrate_val;
};

class pc8801ma_state : public pc8801fh_state
{
public:
	pc8801ma_state(const machine_config &mconfig, device_type type, const char *tag)
		: pc8801fh_state(mconfig, type, tag)
		, m_dictionary_rom(*this, "dictionary")
	{ }

	void pc8801ma(machine_config &config);

protected:
	virtual void machine_reset() override;

	virtual void main_io(address_map &map) override;

	virtual uint8_t dictionary_rom_r(offs_t offset) override;
	virtual bool dictionary_rom_enable() override;

private:
	void dic_bank_w(uint8_t data);
	void dic_ctrl_w(uint8_t data);
	required_region_ptr<u8> m_dictionary_rom;

	uint8_t m_dic_ctrl;
	uint8_t m_dic_bank;
};

class pc8801mc_state : public pc8801ma_state
{
public:
	pc8801mc_state(const machine_config &mconfig, device_type type, const char *tag)
		: pc8801ma_state(mconfig, type, tag)
		, m_cdrom_if(*this, "cdrom_if")
		, m_cdrom_bios(*this, "cdrom_bios")
	{ }

	void pc8801mc(machine_config &config);

protected:
	virtual void machine_reset() override;

	virtual void main_io(address_map &map) override;

private:
	virtual uint8_t cdbios_rom_r(offs_t offset) override;
	virtual bool cdbios_rom_enable() override;

	required_device<pc8801_31_device> m_cdrom_if;
	required_region_ptr<u8> m_cdrom_bios;

	bool m_cdrom_bank;
};

#endif // MAME_INCLUDES_PC8801_H
