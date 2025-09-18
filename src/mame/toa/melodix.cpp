// license:BSD-3-Clause
// copyright-holders: natarii

/***************************************************************************

    TOA MELODIX Hardware

	Driver by Natalie Null (natarii)

	ML-301:
	Clock:	4MHz resonator
	MCU:	Hitachi HD63B01V1 @ 4MHz
	Sound:	YM2203C @ 4MHz
	I/O:	2x start switches (front panel)
            2x playback indicator LEDs (front panel)
			1x 10-position rotary tempo control (front panel, recessed)
			2x volume trimmers (front panel, recessed)
			2x switch inputs (back panel)
			1x remote relay output (back panel)
			1x balanced audio output

	ML-304:
	Clock:	4MHz resonator
	MCU:	Hitachi HD63B01V1 @ 4MHz
	Sound:	YM2203C @ 4MHz
	Media:	4x Fujisoku B-series memory cards
			Up to 32KByte max addressable
			Hardware does not support writing to cards
	I/O:	4x card start switches (front panel)
            4x playback indicator LEDs (front panel)
			4x 10-position rotary tempo controls (front panel, recessed)
			4x volume trimmers (front panel, recessed)
			4x switch inputs (back panel)
			1x remote relay output (back panel)
			1x balanced audio output

	Notes:
	- MCU ROM is shared between ML-301 and ML-304.
	- Port 4 bit 7 selects the hardware type. L = 301, H = 304
	- ML-301 does not have card slots, and uses music data in MCU ROM.
	- ML-301's tempo control only affects the Westminster chime.
	- ML-301B and ML-304B models also exist, with no known changes to the
      circuit or program.
    - I do not have access to ML-301, so there are some hardware mysteries.
	- Later ML-301 units have revised hardware using MSM9810 instead of
	  YM2203, not emulated here.
	- YM2203 SSG outputs are not connected on hardware.

    Not Implemented:
    - Card volume controls. Bits 0 and 2 on port2 control a mux which
      selects the corresponding trimmer for the currently playing card.

	Other TODOs:
    - UI lol
	- does this need to go in mess.flt - nevermind, mess.flt is gone
    - better handling of hardware select bit?
	- card insertion shouldn't force reset - can't use generic_slot_device
    - maybe some cleaner way to handle ml304's strange ym2203 memory map
    - avoid duplicate rom def?
    - better handling of tempo DIPs? IPT_POSITIONAL seems for analog...
    - populate software list

****************************************************************************/

#include "emu.h"

#include "bus/generic/carts.h"
#include "bus/generic/slot.h"
#include "cpu/m6800/m6801.h"
#include "sound/ymopn.h"

#include "softlist_dev.h"
#include "speaker.h"

namespace {

class melodix_state : public driver_device
{
public:
	melodix_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_ym2203(*this, "ym2203")
		, m_cards(*this, "card%u", 0)
	{ }

	void melodix(machine_config &config);
	void ml304(machine_config &config);
	void ml301(machine_config &config);
	
	void init_ml304();
    void init_ml301();

private:
    required_device<hd6301v1_cpu_device> m_maincpu;
    required_device<ym2203_device> m_ym2203;
    optional_device_array<generic_slot_device, 4> m_cards;

    void p2_w(uint8_t data);
    uint8_t p4_r();
    void ml304_ym2203_porta_w(uint8_t data);
    void ml301_ym2203_porta_w(uint8_t data);
    uint8_t ml304_ym2203_portb_r();
    uint8_t ml301_ym2203_portb_r();
    uint8_t ml304_card_data_r(offs_t offset);
    void ml304_card_addr_w(offs_t offset, u8 data);
    void ml304_ym2203_data_w(offs_t offset, u8 data);
    uint8_t ml304_ym2203_data_r(offs_t offset);

    uint8_t m_hardware_sel;

    // only used on ml304
    uint16_t m_card_addr_latch;
    uint8_t m_slot_select;

    void ml304_mem(address_map &map);
    void ml301_mem(address_map &map);
};

void melodix_state::init_ml304()
{
#if 0
	// fix original firmware FM patch upload bug
	uint8_t *rom = memregion("maincpu")->base();
	rom[0xf5ad] = 0x44; // LDAB #$43 -> LDAB #$44
#endif

    m_hardware_sel = 1;
}

void melodix_state::init_ml301()
{
    m_hardware_sel = 0;
}

void melodix_state::p2_w(uint8_t data)
{
    // TODO UI?
    popmessage("Relay: %d", BIT(data, 3));
}

uint8_t melodix_state::p4_r()
{
    // the only valid read here is the hardware select bit
    return 0x7f | (m_hardware_sel << 7);
}

void melodix_state::ml304_ym2203_porta_w(uint8_t data)
{
    // TODO UI
	popmessage("LEDs: %d %d %d %d", BIT(~data, 3, 1), BIT(~data, 2, 1), BIT(~data, 1, 1), BIT(~data, 0, 1));
	m_slot_select = data;
}

void melodix_state::ml301_ym2203_porta_w(uint8_t data)
{
    // TODO UI
	popmessage("LEDs: %d %d", BIT(~data, 0, 1), BIT(~data, 1, 1));
}

uint8_t melodix_state::ml304_ym2203_portb_r()
{
	uint8_t tempo = 0;
    // TODO better way
	switch (BIT(~m_slot_select, 0, 4))
	{
		case 8:
			tempo = ioport("TEMPOA")->read();
			break;
		case 4:
			tempo = ioport("TEMPOB")->read();
			break;
		case 2:
			tempo = ioport("TEMPOC")->read();
			break;
		case 1:
			tempo = ioport("TEMPOD")->read();
			break;
		default:
			// impossible
			break;
	};

	return ioport("STARTDELAY")->read() | (tempo << 1);
}

uint8_t melodix_state::ml301_ym2203_portb_r()
{
	return ioport("STARTDELAY")->read() | (BIT(~ioport("TEMPO")->read(), 0, 4) << 1);
}

uint8_t melodix_state::ml304_card_data_r(offs_t offset)
{
    // program detects card presence by checking if offset 0 bit 0 is not set
    // read_rom() already returns ff if nothing loaded, so no need to handle it here
    return m_cards[3 - offset]->read_rom(m_card_addr_latch);
}

void melodix_state::ml304_card_addr_w(offs_t offset, u8 data)
{
    // TODO better way
    if (offset == 0)
    {
        m_card_addr_latch = (m_card_addr_latch & 0x00ff) | (data << 8);
    }
    else
    {
        m_card_addr_latch = (m_card_addr_latch & 0xff00) | data;
    }
}

void melodix_state::ml304_ym2203_data_w(offs_t offset, u8 data)
{
	m_ym2203->write(1, data);
}

uint8_t melodix_state::ml304_ym2203_data_r(offs_t offset)
{
	return m_ym2203->read(1);
}

void melodix_state::ml304_mem(address_map &map)
{
	map(0x0100, 0x0100).rw(m_ym2203, FUNC(ym2203_device::read), FUNC(ym2203_device::write));
	map(0x0101, 0x0104).r(FUNC(melodix_state::ml304_card_data_r));
	map(0x0105, 0x0106).w(FUNC(melodix_state::ml304_card_addr_w));
	map(0x0140, 0x0140).rw(FUNC(melodix_state::ml304_ym2203_data_r), FUNC(melodix_state::ml304_ym2203_data_w)); // bus A6 is wired directly to ym2203 A0
}

void melodix_state::ml301_mem(address_map &map)
{
	map(0x0100, 0x0101).rw(m_ym2203, FUNC(ym2203_device::read), FUNC(ym2203_device::write));
}

static INPUT_PORTS_START( ml304 )
	PORT_START("STARTDELAY")
	PORT_DIPNAME(0xe0, 0x60, "Start Delay")
	PORT_DIPSETTING(0x60, "0 seconds")
	PORT_DIPSETTING(0xa0, "2 seconds")
	PORT_DIPSETTING(0xc0, "4 seconds")

	PORT_START("TEMPOA")
	PORT_DIPNAME(0x0f, 5, "Card A Tempo")
	PORT_DIPSETTING(0, "0 (slowest)")
	PORT_DIPSETTING(1, "1")
	PORT_DIPSETTING(2, "2")
	PORT_DIPSETTING(3, "3")
	PORT_DIPSETTING(4, "4")
	PORT_DIPSETTING(5, "5 (normal)")
	PORT_DIPSETTING(6, "6")
	PORT_DIPSETTING(7, "7")
	PORT_DIPSETTING(8, "8")
	PORT_DIPSETTING(9, "9 (fastest)")

	PORT_START("TEMPOB")
	PORT_DIPNAME(0x0f, 5, "Card B Tempo")
	PORT_DIPSETTING(0, "0 (slowest)")
	PORT_DIPSETTING(1, "1")
	PORT_DIPSETTING(2, "2")
	PORT_DIPSETTING(3, "3")
	PORT_DIPSETTING(4, "4")
	PORT_DIPSETTING(5, "5 (normal)")
	PORT_DIPSETTING(6, "6")
	PORT_DIPSETTING(7, "7")
	PORT_DIPSETTING(8, "8")
	PORT_DIPSETTING(9, "9 (fastest)")

	PORT_START("TEMPOC")
	PORT_DIPNAME(0x0f, 5, "Card C Tempo")
	PORT_DIPSETTING(0, "0 (slowest)")
	PORT_DIPSETTING(1, "1")
	PORT_DIPSETTING(2, "2")
	PORT_DIPSETTING(3, "3")
	PORT_DIPSETTING(4, "4")
	PORT_DIPSETTING(5, "5 (normal)")
	PORT_DIPSETTING(6, "6")
	PORT_DIPSETTING(7, "7")
	PORT_DIPSETTING(8, "8")
	PORT_DIPSETTING(9, "9 (fastest)")

	PORT_START("TEMPOD")
	PORT_DIPNAME(0x0f, 5, "Card D Tempo")
	PORT_DIPSETTING(0, "0 (slowest)")
	PORT_DIPSETTING(1, "1")
	PORT_DIPSETTING(2, "2")
	PORT_DIPSETTING(3, "3")
	PORT_DIPSETTING(4, "4")
	PORT_DIPSETTING(5, "5 (normal)")
	PORT_DIPSETTING(6, "6")
	PORT_DIPSETTING(7, "7")
	PORT_DIPSETTING(8, "8")
	PORT_DIPSETTING(9, "9 (fastest)")

	PORT_START("SWITCH")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_START1) PORT_NAME("Front Panel A Start")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_START2) PORT_NAME("Front Panel B Start")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START3) PORT_NAME("Front Panel C Start")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_START4) PORT_NAME("Front Panel D Start")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_START5) PORT_NAME("Rear Terminals A Start")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START6) PORT_NAME("Rear Terminals B Start")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_START7) PORT_NAME("Rear Terminals C Start")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_START8) PORT_NAME("Rear Terminals D Start")
INPUT_PORTS_END

static INPUT_PORTS_START( ml301 )
	PORT_START("STARTDELAY")
	PORT_DIPNAME(0xe0, 0x60, "Start Delay")
	PORT_DIPSETTING(0x60, "0 seconds")
	PORT_DIPSETTING(0xa0, "2 seconds")
	PORT_DIPSETTING(0xc0, "4 seconds")
    
	PORT_START("TEMPO")
	PORT_DIPNAME(0x0f, 5, "Westminster Tempo")
	PORT_DIPSETTING(0, "0 (slowest)")
	PORT_DIPSETTING(1, "1")
	PORT_DIPSETTING(2, "2")
	PORT_DIPSETTING(3, "3")
	PORT_DIPSETTING(4, "4")
	PORT_DIPSETTING(5, "5 (normal)")
	PORT_DIPSETTING(6, "6")
	PORT_DIPSETTING(7, "7")
	PORT_DIPSETTING(8, "8")
	PORT_DIPSETTING(9, "9 (fastest)")

	PORT_START("SWITCH")
	// not perfect, hardware is unknown
	PORT_BIT(0x74, 0x74, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START1) PORT_NAME("Westminster Start")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_START2) PORT_NAME("Chime Start")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN) // also starts westminster
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN) // also starts westminster
INPUT_PORTS_END

void melodix_state::melodix(machine_config &config)
{
	// basic machine hardware
	HD6301V1(config, m_maincpu, 4_MHz_XTAL);
	m_maincpu->in_p1_cb().set_ioport("SWITCH");
	m_maincpu->out_p2_cb().set(FUNC(melodix_state::p2_w));
	m_maincpu->in_p4_cb().set(FUNC(melodix_state::p4_r));


	// sound hardware
    SPEAKER(config, "mono").front_center();
	YM2203(config, m_ym2203, 4_MHz_XTAL);
    m_ym2203->add_route(ALL_OUTPUTS, "mono", 0.15);
	m_ym2203->irq_handler().set_inputline(m_maincpu, 0);
}

void melodix_state::ml304(machine_config &config)
{
    melodix(config);

    m_maincpu->set_addrmap(AS_PROGRAM, &melodix_state::ml304_mem);

    m_ym2203->port_a_write_callback().set(FUNC(melodix_state::ml304_ym2203_porta_w));
    m_ym2203->port_b_read_callback().set(FUNC(melodix_state::ml304_ym2203_portb_r));

    // TODO better way
    GENERIC_CARTSLOT(config, m_cards[0], generic_plain_slot, "melodix_card");
    GENERIC_CARTSLOT(config, m_cards[1], generic_plain_slot, "melodix_card");
    GENERIC_CARTSLOT(config, m_cards[2], generic_plain_slot, "melodix_card");
    GENERIC_CARTSLOT(config, m_cards[3], generic_plain_slot, "melodix_card");

    SOFTWARE_LIST(config, "melodix_card_list").set_original("melodix_card");
}

void melodix_state::ml301(machine_config &config)
{
    melodix(config);

    m_maincpu->set_addrmap(AS_PROGRAM, &melodix_state::ml301_mem);

    m_ym2203->port_a_write_callback().set(FUNC(melodix_state::ml301_ym2203_porta_w));
    m_ym2203->port_b_read_callback().set(FUNC(melodix_state::ml301_ym2203_portb_r));
}

}

ROM_START( ml304 )
	ROM_REGION( 0x1000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "melodix.bin", 0x0000, 0x1000, CRC(82197619) SHA1(baf1be6c60d5119283ac77b14eb87ed32418061c) )
ROM_END

ROM_START( ml301 )
	ROM_REGION( 0x1000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "melodix.bin", 0x0000, 0x1000, CRC(82197619) SHA1(baf1be6c60d5119283ac77b14eb87ed32418061c) )
ROM_END

//    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  CLASS          INIT        COMPANY            FULLNAME  FLAGS
SYST( 1987, ml304, 0,      0,      ml304,   ml304, melodix_state, init_ml304, "TOA Corporation", "ML-304", 0 )
SYST( 198?, ml301, ml304,  0,      ml301,   ml301, melodix_state, init_ml301, "TOA Corporation", "ML-301", 0 )
