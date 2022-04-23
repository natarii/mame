//32620.2 has no dbe eprom populated and memory map is all different

//TODO
//proper speaker/output handling
//uart + document commands based on disasm?
//daisy chain
//proper display timing
//real dac
//dac gain
//leds
//sta/stp interrupt thing, look into INPUT_CHANGED_MEMBER ?
//explain problems encountered with display - just OBOE thing on 4028 that selects row
//clock is wrong, board pics show 9832.0
//cpu eprom label Bu01 8/87
//32620 8251 uart, 32620.2 UB8560 = Z80 SIO
//update rom checksums to include SP, also now that german has been redumped (and confirmed bad dump on the first batch)
#include "includes/sprachmg.h"


#include "screen.h"
#include "speaker.h"

static const z80_daisy_config z80_daisy_chain[] =
{
    { "pio_d14" },
    { "pio_d15" },
    { nullptr }
};

void sprachmg_state::machine_start() {
	m_display.resolve();
    m_led_morse.resolve();
    m_led_speech.resolve();
    m_led_standard.resolve();
}

void sprachmg_state::machine_reset() {

}

void sprachmg_state::dac_w(uint8_t data) {
    m_dac->write(data);
}

void sprachmg_state::pio_d6_pa_w(uint8_t data) {
    printf("D6 A %02x\n", data);
}

WRITE_LINE_MEMBER(sprachmg_state::test) {
    printf("INT!!!\n");
}
void sprachmg_state::init_sprachmg2() {
    
    
    
    
    m_disp_buf[8] = 0; //hack
    m_cart_sel = 0xC0; //ub hack
    m_dac->write(0x7f); //hack
}

uint8_t sprachmg_state::pio_d15_pb_r() {
    printf("D15 B READ\n"); //never used probably
    return 0;
}

void sprachmg_state::pio_d14_pa_w(uint8_t data) {
    m_keyboard_scan = ~data & 0x1F;
    //printf("KEY SCAN %02x\n", m_keyboard_scan);
}

uint8_t sprachmg_state::pio_d14_pb_r() {
    //is 0x80 needed? yes, but why? code seems to OR on 0xe0 often, maybe not everywhere? - correct, not everywhere
    uint8_t val = 0x80 | (ioport("SPECIAL")->read() & 0x60);
    for (int bit = 0; bit < 5; bit++) {
        if (BIT(m_keyboard_scan, bit)) {
            val |= m_keyboard[bit]->read();
            break;
        }
    }
    return val;
}

void sprachmg_state::pio_d15_pa_w(uint8_t data) {
    //printf("D15 A %02x %c\n", data, data);
    m_disp_char = data;
    //hack lole
    m_pio_d15->strobe_b(0);
    m_pio_d15->strobe_b(1);
}

void sprachmg_state::pio_d15_pb_w(uint8_t data) { //write char num
    //printf("D15 B %02x\n", data);
    m_disp_buf[data&7] = m_disp_char;
    //popmessage("%s", m_disp_buf);
    uint8_t *charrom = memregion("charrom")->base();
    uint8_t charidx = data&7;
    for (uint8_t row=0;row<7;row++) {
        m_display[charidx][row] = charrom[((row+1)*256)+m_disp_char];
    }
}

static INPUT_PORTS_START( sprachmg )
    PORT_START("SPECIAL")
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_NAME("Start/Stop") PORT_WRITE_LINE_DEVICE_MEMBER("pio_d14", z80pio_device, pb5_w)
    //PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_NAME("Remote Control Start/Stop")

    PORT_START("X0")
    PORT_BIT(1<<0, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_BIT(1<<1, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_NAME("Execute")
    PORT_BIT(1<<2, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT) PORT_NAME("Left")
    PORT_BIT(1<<3, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT) PORT_NAME("Right")
    PORT_BIT(1<<4, IP_ACTIVE_LOW, IPT_UNUSED)
    PORT_START("X1")
    PORT_BIT(1<<0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_NAME("Space")
    PORT_BIT(1<<1, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD) PORT_NAME("+")
    PORT_BIT(1<<2, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD) PORT_NAME("/")
    PORT_BIT(1<<3, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_NAME(":")
    PORT_BIT(1<<4, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_NAME("Clear")
    PORT_START("X2")
    PORT_BIT(1<<0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_NAME("/")
    PORT_BIT(1<<1, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) PORT_NAME("3/Dash")
    PORT_BIT(1<<2, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) PORT_NAME("6")
    PORT_BIT(1<<3, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) PORT_NAME("9")
    PORT_BIT(1<<4, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_NAME("Mode")
    PORT_START("X3")
    PORT_BIT(1<<0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_NAME("=")
    PORT_BIT(1<<1, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) PORT_NAME("2/Dot")
    PORT_BIT(1<<2, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) PORT_NAME("5")
    PORT_BIT(1<<3, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) PORT_NAME("8")
    PORT_BIT(1<<4, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_NAME("Output")
    PORT_START("X4")
    PORT_BIT(1<<0, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD) PORT_NAME("0/No")
    PORT_BIT(1<<1, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) PORT_NAME("1/Yes")
    PORT_BIT(1<<2, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) PORT_NAME("4")
    PORT_BIT(1<<3, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) PORT_NAME("7")
    PORT_BIT(1<<4, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_NAME("Input")
INPUT_PORTS_END

void sprachmg_state::pio_d5_pb_w(uint8_t data) {
    m_cart_sel = data & 0xC0;
    m_led_speech = (data >> 3) & 1;
    m_led_morse = (data >> 4) & 1;
    m_led_standard = (data >> 5) & 1;
}

uint8_t sprachmg_state::cart_r(offs_t offset) {
    if ((m_cart_sel & 0x80) == 0) {
        uint8_t *cart = memregion("cart")->base();
        return cart[offset];
    } else if ((m_cart_sel & 0x40) == 0) {
        //todo dbs 2
        return 0xff;
    }
    return 0xff; //open bus? or is it pulled somewhere? needs to read ff in unused space
}

void sprachmg_state::sprachmg_mem(address_map &map) {
    map(0x0000, 0x0FFF).rom().region("mainrom", 0);
    map(0x0000, 0x17FF).ram();
    map(0x2000, 0x3FFF).rom().region("expansionrom", 0);
    //map(0x4000, 0xFFFF).r(m_cart);
}

void sprachmg_state::sprachmg2_mem(address_map &map) {
    map(0x0000, 0x1FFF).rom().region("mainrom", 0);
    map(0x2000, 0x2FFF).ram();
    //map(0x4000, 0xFFFF).rom().region("cart", 0); //tmp
    map(0x4000, 0xFFFF).r(FUNC(sprachmg_state::cart_r));
}

void sprachmg_state::sprachmg_io(address_map &map) {
	map.unmap_value_high();
	map.global_mask(0xff);
    //map(0xF4).rw()
	//map(0xEC, 0xEF).rw("pio_d15", FUNC(z80pio_device::read), FUNC(z80pio_device::write));
}

void sprachmg_state::uarttest(uint8_t data) {
    printf("UART DATA %02x\n", data);
}

void sprachmg_state::sprachmg2_io(address_map &map) {
	map.unmap_value_high();
	map.global_mask(0xff);
	map(0x7C, 0x7F).rw("pio_d5", FUNC(z80pio_device::read), FUNC(z80pio_device::write)); //dc on block diagram
	map(0xBC, 0xBF).rw("pio_d6", FUNC(z80pio_device::read), FUNC(z80pio_device::write)); //bc on block diagram
	map(0xC8, 0xCB).rw("pio_d14", FUNC(z80pio_device::read), FUNC(z80pio_device::write)); //f8 on block diagram
	map(0xD0, 0xD3).rw("pio_d15", FUNC(z80pio_device::read), FUNC(z80pio_device::write)); //ec on block diagram
    map(0xC0, 0xC0).w(FUNC(sprachmg_state::uarttest));
}

void sprachmg_state::sprachmg(machine_config &config) {
    Z80(config, m_maincpu, 9.8304_MHz_XTAL / 4); //UB880D
	m_maincpu->set_daisy_config(z80_daisy_chain);
    m_maincpu->set_addrmap(AS_PROGRAM, &sprachmg_state::sprachmg_mem);
    m_maincpu->set_addrmap(AS_IO, &sprachmg_state::sprachmg_io);

    I8251(config, m_usart, 9.8304_MHz_XTAL / 4);
    //handlers, rs232_port_device

    Z80PIO(config, m_pio_d5, 9.8304_MHz_XTAL / 4); //todo verify clock?
    Z80PIO(config, m_pio_d6, 9.8304_MHz_XTAL / 4); //todo verify clock?
    Z80PIO(config, m_pio_d14, 9.8304_MHz_XTAL / 4); //todo verify clock?
    Z80PIO(config, m_pio_d15, 9.8304_MHz_XTAL / 4); //todo verify clock?
    m_pio_d5->out_pa_callback().set(FUNC(sprachmg_state::dac_w));
    m_pio_d5->out_pb_callback().set(FUNC(sprachmg_state::pio_d5_pb_w));
    m_pio_d6->out_pa_callback().set(FUNC(sprachmg_state::pio_d6_pa_w));
    m_pio_d14->out_pa_callback().set(FUNC(sprachmg_state::pio_d14_pa_w));
    m_pio_d14->in_pb_callback().set(FUNC(sprachmg_state::pio_d14_pb_r));
    m_pio_d15->out_pa_callback().set(FUNC(sprachmg_state::pio_d15_pa_w));
    m_pio_d15->out_pb_callback().set(FUNC(sprachmg_state::pio_d15_pb_w));
    //m_pio_d15->in_pb_callback().set(FUNC(sprachmg_state::pio_d15_pb_r)); //never will be used probably
    //ints, still need d4 d5 maybe?
    m_pio_d14->out_int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0); //////////////////////
    //m_pio_d14->out_int_callback().set(FUNC(sprachmg_state::test));
    m_pio_d15->out_int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
    //m_pio_d15->out_int_callback().set(FUNC(sprachmg_state::test));


    SPEAKER(config, "speaker").front_center();
	DAC_8BIT_R2R(config, m_dac, 0).add_route(0, "speaker", 0.5); //hack

	config.set_default_layout(layout_sprachmg);
}

void sprachmg_state::sprachmg2(machine_config &config) {
    sprachmg(config);
    m_maincpu->set_addrmap(AS_PROGRAM, &sprachmg_state::sprachmg2_mem);
    m_maincpu->set_addrmap(AS_IO, &sprachmg_state::sprachmg2_io);
}

ROM_START( sprachmg )
	ROM_REGION( 0x1000, "mainrom", ROMREGION_ERASEFF )
	ROM_LOAD( "cpu.bin", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x2000, "expansionrom", ROMREGION_ERASEFF )
	ROM_LOAD( "dbe.bin", 0x0000, 0x2000, NO_DUMP )

    ROM_REGION( 0x0800, "charrom", ROMREGION_ERASEFF )
    ROM_LOAD( "32620_display.bin", 0x0000, 0x0800, CRC(9ffd1e15) SHA1(759660404dfe479d13a1bdd4beb19e6035a34e17) ) //not confirmed, sprachmg2 was what was dumped, *probably* the same on sprachmg
ROM_END


ROM_START( sprachmg2 )
	ROM_REGION( 0x2000, "mainrom", ROMREGION_ERASEFF )
	ROM_LOAD( "32620-2-cpu.bin", 0x0000, 0x2000, CRC(7d9a92a6) SHA1(c9ca4a0d118b2c30e2505de051671769ad08a1c5) )

	ROM_REGION( 0x0800, "charrom", ROMREGION_ERASEFF )
    ROM_LOAD( "32620_display.bin", 0x0000, 0x0800, CRC(9ffd1e15) SHA1(759660404dfe479d13a1bdd4beb19e6035a34e17) )

    //todo turn into SOFTWARE_LIST?
	ROM_REGION( 0xC000, "cart", ROMREGION_ERASEFF )
    ROM_LOAD("2620_8401-1.bin", 0x0000, 0x2000, CRC(2e475ce7) SHA1(efd3da128f515e479547c9e4f4bb107dde8ea52d) BAD_DUMP)
    ROM_LOAD("2620_8401-2.bin", 0x2000, 0x2000, CRC(675ea7c8) SHA1(a74cd99f8be98b85d356ae0c3c5c794d740a7445) BAD_DUMP)
    ROM_LOAD("2620_8401-3.bin", 0x4000, 0x2000, CRC(9d474786) SHA1(f05e232c04de34aceaa63fe26a2d5dfb4c5a163a) BAD_DUMP)
    ROM_LOAD("2620_8401-4.bin", 0x6000, 0x2000, CRC(7f02ff72) SHA1(e4d7db906a08d89602fd093b7a037a270e567536))
    ROM_LOAD("2620_8401-5.bin", 0x8000, 0x2000, CRC(f20c8633) SHA1(eebd61bab47d54e7688cdac287ceb1a110d46af7))
    ROM_LOAD("2620_8401-6.bin", 0xA000, 0x2000, CRC(69a2d785) SHA1(2c547b583dcc16208f0422e121cd482c3356528d))
ROM_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   CLASS         INIT        COMPANY                   FULLNAME                                      FLAGS */
COMP( 1983, sprachmg, 0,      0,      sprachmg, sprachmg, sprachmg_state, empty_init, "Ministerium für Staatssicherheit", "Sprach/Morse Generator 32620", MACHINE_NOT_WORKING )
COMP( 1987, sprachmg2, sprachmg,      0,      sprachmg2, sprachmg, sprachmg_state, init_sprachmg2, "Ministerium für Staatssicherheit", "Sprach/Morse Generator 32620.2", 0 )