#ifndef MAME_INCLUDES_SPRACHMG_H
#define MAME_INCLUDES_SPRACHMG_H

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80daisy.h"
#include "machine/ram.h"
#include "machine/i8251.h"
#include "machine/z80pio.h"
#include "sound/dac.h"
#include "sprachmg.lh"

#define SCREEN_TAG "screen"
#define Z80_TAG "z80"

class sprachmg_state : public driver_device
{
public:
	sprachmg_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, Z80_TAG)
		, m_usart(*this, "usart")
		, m_pio_d5(*this, "pio_d5")
		, m_pio_d6(*this, "pio_d6")
		, m_pio_d14(*this, "pio_d14")
		, m_pio_d15(*this, "pio_d15")
		, m_dac(*this, "dac")
		, m_keyboard(*this, "X%u", 0)
		, m_mainrom(*this, "mainrom")
		, m_charrom(*this, "charrom")
		, m_display(*this, "disp_char%u_row%u", 0U, 0U)
		, m_led_morse(*this, "led_morse")
		, m_led_speech(*this, "led_speech")
		, m_led_standard(*this, "led_standard")
		, m_expansionrom(*this, "expansionrom")
		, m_cart(*this, "cart")
	{
	}

	void sprachmg(machine_config &config);
	void sprachmg2(machine_config &config);

	void init_sprachmg2();

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;

private:
	required_device<z80_device> m_maincpu;
	required_device<i8251_device> m_usart;
	required_device<z80pio_device> m_pio_d5;
	required_device<z80pio_device> m_pio_d6;
	required_device<z80pio_device> m_pio_d14;
	required_device<z80pio_device> m_pio_d15;
	required_device<dac_8bit_r2r_device> m_dac;
	required_ioport_array<5> m_keyboard;
	required_region_ptr<uint8_t> m_mainrom;
	required_region_ptr<uint8_t> m_charrom;
	output_finder<8, 7> m_display;
	output_finder<> m_led_morse;
	output_finder<> m_led_speech;
	output_finder<> m_led_standard;
	optional_region_ptr<uint8_t> m_expansionrom;
	optional_region_ptr<uint8_t> m_cart;

	void sprachmg_io(address_map &map);
	void sprachmg_mem(address_map &map);
	void sprachmg2_io(address_map &map);
	void sprachmg2_mem(address_map &map);

	void pio_d5_pb_w(uint8_t data);
	void pio_d6_pa_w(uint8_t data);
	void pio_d14_pa_w(uint8_t data);
	void pio_d15_pa_w(uint8_t data);
	void pio_d15_pb_w(uint8_t data);
	uint8_t pio_d14_pb_r();
	uint8_t pio_d15_pb_r();
	void dac_w(uint8_t data);
	uint8_t cart_r(offs_t offset);

	uint8_t m_keyboard_scan;
	char m_disp_char;
	char m_disp_buf[9];
	uint8_t m_cart_sel;

	//void test();
	DECLARE_WRITE_LINE_MEMBER( test );
	void uarttest(uint8_t data);
};

#endif