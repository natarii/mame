// license:BSD-3-Clause
// copyright-holders: Angelo Salese
// thanks-to: Alex Marshall
/**************************************************************************************************

    PC-8801 (c) 1981 NEC

    driver by Angelo Salese, original MESS PC-88SR driver by ???

    TODO:
    - implement proper i8214 routing, and add irq latch mechanism;
    - implement proper upd3301 / i8257 text support (currently hacked around);
    - Add limits for extend work RAM;
    - waitstates;
    - clean-ups:
      - better state machine isolation of features between various models.
        Vanilla PC-8801 doesn't have analog palette, PC80S31 device as default
        (uses external minidisk), other misc banking bits.
      - refactor memory banking to use address maps;
      - double check dipswitches;
      - Slotify PC80S31K, also needed by PC-6601SR, PC-88VA, (vanilla & optional) PC-9801. **partially done**
        Also notice that there are common points with SPC-1000 and TF-20 FDDs;
      - backport/merge what is portable to PC-8001;
      - Kanji LV1/LV2 ROM hookups needs to be moved at slot level.
        Needs identification effort about what's internal to machine models and what instead
        can be optionally installed;
    - implement proper joypad / mouse (PC-8872) port connector;
    - implement bus slot mechanism for NEC boards
      (does it have an actual codename or just "PC-8801 EXPansion bus"?);
      \- NEC PC-8801-10
            (MIDI interface);
      \- NEC PC-8801-11
            ("Sound Board", single YM2203C OPN, single joy port, mono out);
      \- NEC PC-8801-12
            (Modem board, full duplex 300bps);
      \- NEC PC-8801-13
            (Parallel I/F board);
      \- NEC PC-8801-17 / -18
            (VTR capture card "Video art board" / "Video digitizing unit", 16-bit color);
      \- NEC PC-8801-21
            (CMT i/f board);
      \- NEC PC-8801-22
            ("Multi board B", upgrades a FH to MH and FA to MA (?));
      \- NEC PC-8801-23 & -24 & -25
            ("Sound Board 2", single YM2608 OPNA, single joy port, stereo out.
            -24 is the internal FH / MH version, -25 is the internal FE / FE2 with YM2608B.
            Standard and on main board instead for FA / MA and onward);
      \- NEC PC-8801-30 & -31
            (CD-ROM SCSI i/f, subset of PC Engine and PCFX) **in progress**
      \- NEC PC-8864
            (Network board mapping at $a0-$a3)
      \- HAL PCG-8100
            (PCG and 3x DAC_1BIT at I/O $01, $02. PIT at $0c-$0f)
      \- HAL GSX-8800
            (2x PSG at I/O $a0-$a3, mono out. Has goofy extra connector on top and a couple jumpers,
             guess it may cascade with another board for 2x extra PSGs at $a4-$a7);
      \- HIBIKI-8800
            (YM2151 OPM + YM3802-X MIDI controller, stereo out, has own internal XTAL @ 4MHz.
             Has an undumped PAL/PROM labeled "HAL-881");
      \- HAL HMB-20
            (same as HIBIKI-8800 board?)
      \- JMB-X1
            ("Sound Board X", 2x OPM + 1x SSG. Used by NRTDRV, more info at GH #8709);

    list of games/apps that crashes due of floppy issues (* -> denotes games fixed with current floppy code, # -> regressed with current floppy code):
    - Chikyuu Boueigun (disk i/o error during "ESDF SYSTEM LOADING") (REGRESSED with current floppy code)
    * Chikyuu Senshi Rayieza (fdc CPU crashes)
    - Choplifter
    - Columns (code at 0x28c8, copy protection)
    - Corridor ("THIS SYSTEM NOT KOEI SYSTEM" printed on screen) (REGRESSED with current floppy code)
    # Craze (returns to basic after logo pops up, tries to self-modify program data via the window offset?)
    * Crimson
    * Crimson 3
    * Cuby Panic (copy protection routine at 0x911A)
    - Databox (app)
    - Day Dream ("Bad drive number at 570")
    - Demons Ring
    * Dennou Tsuushin
    - Door Door MK-2 (sets up TC in the middle of execution phase read then wants status bit 6 to be low PC=0x7050 of fdc cpu)
    * Dragon Slayer - The Legend of Heroes 2
    * El Dorado Denki
    - Emerald Densetsu (dies after few seconds of intro)
    - Emerald Dragon (it seems to miss a timer)
    - Explosion (fails to load ADPCM data?)
    * F15 Strike Eagle
    - F2 Grand Prix ("Boot dekimasen")
    - Fantasian
    - FSD Sample Ongaku Shuu Vol. 1-7
    - Gaia no Kiba (Disk I/O error at 150)
    - GC-clusterz Music Disk Vol. 1-7

    - Kaseijin (app) (code snippet is empty at some point)
    - Lamia: fails to create an user disk (after character creation) -> disk write error
    - Mr. Pro Yakyuu
    - P1 (app)
    - Pattern Editor 88 (app)
    - Super Shunbo II (app) (Load error)
    - Super TII (app)
    - Tobira wo Akete (random crashes in parent pc8801 only)

    list of games that doesn't like i8214_irq_level == 5 in sound irq
    - 100yen Disk 2 / Jumper 2: Sound BGM dies pretty soon;

    games that needs to NOT have write-protect floppies (BTANBs):
    - Balance of Power
    - Blue Moon Story: moans with a kanji msg;
    - Mahjong Clinic Zoukangou
    - Tobira wo Akete (hangs at title screen)

    games that needs to HAVE write-protect floppies (BTANBs):
    - 100 Yen Disk 7: (doesn't boot in V2 mode)

    Notes:
    - Later models have washed out palette with some SWs, with no red component.
      This is because you have to set up the V1 / V2 DIP-SW to V1 mode for those games
      (BIOS sets up analog palette and never changes back otherwise).
      cfr. SW list usage SW notes that specifically needs V1.

======================================================================================================================================

    PC-88xx Models (and similar machines like PC-80xx and PC-98DO)

    Model            | release |      CPU     |                      BIOS components                        |       |
                     |         |     clock    | N-BASIC | N88-BASIC | N88-BASIC Enh |  Sound  |  CD |  Dict |  Disk | Notes
    ==================================================================================================================================
    PC-8001          | 1979-03 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   |
    PC-8001A         |   ??    |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (U)
    PC-8801          | 1981-11 |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   |   -   | (KO)
    PC-8801A         |   ??    |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   |   -   | (U)
    PC-8001 mkII     | 1983-03 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (GE),(KO)
    PC-8001 mkIIA    |   ??    |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (U),(GE)
    PC-8801 mkII     | 1983-11 |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   | (FDM) | (K1)
    PC-8001 mkII SR  | 1985-01 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (GE),(NE),(KO)
    PC-8801 mkII SR  | 1985-03 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K1)
    PC-8801 mkII TR  | 1985-10 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K1)
    PC-8801 mkII FR  | 1985-11 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K1)
    PC-8801 mkII MR  | 1985-11 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (K2)
    PC-8801 FH       | 1986-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K2)
    PC-8801 MH       | 1986-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (K2)
    PC-88 VA         | 1987-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-8801 FA       | 1987-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MA       | 1987-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-88 VA2        | 1988-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-88 VA3        | 1988-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FD3) | (K2)
    PC-8801 FE       | 1988-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MA2      | 1988-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-98 DO         | 1989-06 |   z80H @ 8   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (KE)
    PC-8801 FE2      | 1989-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MC       | 1989-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  X  |   X   | (FDH) | (K2)
    PC-98 DO+        | 1990-10 |   z80H @ 8   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (KE)

    info for PC-98 DO & DO+ refers to their 88-mode

    Disk Drive options:
    (FDM): there exist three model of this computer: Model 10 (base model, only optional floppy drive), Model 20
        (1 floppy drive for 5.25" 2D disks) and Model 30 (2 floppy drive for 5.25" 2D disks)
    (FD2): 2 floppy drive for 5.25" 2D disks
    (FDH): 2 floppy drive for both 5.25" 2D disks and 5.25" HD disks
    (FD3): 2 floppy drive for both 5.25" 2D disks and 5.25" HD disks + 1 floppy drive for 3.5" 2TD disks

    Notes:
    (U): US version
    (GE): Graphic Expansion for PC-8001
    (NE): N-BASIC Expansion for PC-8001 (similar to N88-BASIC Expansion for PC-88xx)
    (KO): Optional Kanji ROM
    (K1): Kanji 1st Level ROM
    (K2): Kanji 2nd Level ROM
    (KE): Kanji Enhanced ROM

    Memory mounting locations:
     * N-BASIC 0x0000 - 0x5fff, N-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * N-BASIC 0x0000 - 0x5fff, N-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * N88-BASIC 0x0000 - 0x7fff, N88-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * Sound BIOS: 0x6000 - 0x7fff
     * CD-ROM BIOS: 0x0000 - 0x7fff
     * Dictionary: 0xc000 - 0xffff (32 Banks)

    info from https://retrocomputerpeople.web.fc2.com/machines/nec/8801/

*************************************************************************************************************************************/


#include "emu.h"
#include "includes/pc8801.h"

#include "softlist_dev.h"


#define PC8801FH_OSC1   XTAL(28'636'363)
#define PC8801FH_OSC2   XTAL(42'105'200)
#define PC8801FH_OSC3   XTAL(31'948'800)    // called OSC1 on PC-8801FE board

#define MASTER_CLOCK (PC8801FH_OSC3 / 8)
// TODO: exact clocks
#define PIXEL_CLOCK_15KHz (PC8801FH_OSC1 / 2)
#define PIXEL_CLOCK_24KHz XTAL(21'477'272)  // should be (PC8801FH_OSC2 / 2)?


/*
CRTC command params:
0. CRTC reset

[0] *--- ---- <unknown>
[0] -xxx xxxx screen columns (+2)

[1] xx-- ---- blink speed (in frame unit) (+1, << 3)
[1] --xx xxxx screen lines (+1)

[2] x--- ---- "skip line"
[2] -x-- ---- cursor style (reverse on / underscore off)
[2] --x- ---- cursor blink on/off
[2] ---x xxxx lines per character (+1)

[3] xxx- ---- Vertical Retrace (+1)
[3] ---x xxxx Horizontal Retrace (+2)

[4] x--- ---- attribute not separate flag
[4] -x-- ---- attribute color flag
[4] --x- ---- attribute not special flag (invalidates next register)
[4] ---x xxxx attribute size (+1)
*/

#define screen_width ((m_crtc.param[0][0] & 0x7f) + 2) * 8

#define blink_speed ((((m_crtc.param[0][1] & 0xc0) >> 6) + 1) << 3)
#define screen_height ((m_crtc.param[0][1] & 0x3f) + 1)

#define lines_per_char ((m_crtc.param[0][2] & 0x1f) + 1)

#define vretrace (((m_crtc.param[0][3] & 0xe0) >> 5) + 1)
#define hretrace ((m_crtc.param[0][3] & 0x1f) + 2) * 8

#define text_color_flag ((m_crtc.param[0][4] & 0xe0) == 0x40)
// TODO: not the right condition
//#define monitor_24KHz ((m_gfx_ctrl & 0x19) == 0x08)

void pc8801_state::video_start()
{
}

void pc8801_state::palette_reset()
{
	int i;

	for (i = 0; i < 8; i ++)
	{
		m_palram[i].b = i & 1 ? 7 : 0;
		m_palram[i].r = i & 2 ? 7 : 0;
		m_palram[i].g = i & 4 ? 7 : 0;
	}

	// text + bitmap
	for(i = 0; i < 0x10; i++)
		m_palette->set_pen_color(i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
}

void pc8801_state::draw_bitmap_3bpp(bitmap_ind16 &bitmap,const rectangle &cliprect)
{
	uint32_t count = 0;

	uint16_t y_double = pixel_clock();
	uint16_t y_size = (y_double+1) * 200;

	for(int y = 0; y < y_size; y+=(y_double + 1))
	{
		for(int x = 0; x < 640; x+=8)
		{
			for(int xi = 0; xi < 8; xi++)
			{
				int pen = 0;

				/* note: layer masking doesn't occur in 3bpp mode, Bug Attack relies on this */
				pen |= ((m_gvram[count+0x0000] >> (7-xi)) & 1) << 0;
				pen |= ((m_gvram[count+0x4000] >> (7-xi)) & 1) << 1;
				pen |= ((m_gvram[count+0x8000] >> (7-xi)) & 1) << 2;

				if(y_double)
				{
					if(cliprect.contains(x+xi, y+0))
						bitmap.pix(y+0, x+xi) = m_palette->pen(pen & 7);

					// TODO: real HW seems to actually just output to either even or odd line when in 3bpp mode
					// investigate which is right
					if(cliprect.contains(x+xi, y+1))
						bitmap.pix(y+1, x+xi) = m_palette->pen(pen & 7);
				}
				else
				{
					if(cliprect.contains(x+xi, y+0))
						bitmap.pix(y, x+xi) = m_palette->pen(pen & 7);
				}
			}

			count++;
		}
	}
}

void pc8801_state::draw_bitmap_1bpp(bitmap_ind16 &bitmap,const rectangle &cliprect)
{
	// TODO: jettermi really masks the color attribute from 3301
	// (we currently draw it in b&w, should be colorized)
	uint32_t count = 0;
	uint8_t color = (m_gfx_ctrl & 1) ? 7 & ((m_layer_mask ^ 0xe) >> 1) : 7;
	uint8_t is_cursor = 0;

	for(int y = 0; y < 200; y++)
	{
		for(int x = 0; x < 640; x+=8)
		{
			if(!(m_gfx_ctrl & 1))
				is_cursor = calc_cursor_pos(x / 8, y / lines_per_char, y & (lines_per_char-1));

			for(int xi = 0; xi < 8; xi++)
			{
				int pen = ((m_gvram[count+0x0000] >> (7-xi)) & 1);
				if(is_cursor)
					pen^=1;

				if((m_gfx_ctrl & 1))
				{
					if(cliprect.contains(x+xi, y*2+0))
						bitmap.pix(y*2+0, x+xi) = m_palette->pen(pen ? color : 0);

					if(cliprect.contains(x+xi, y*2+1))
						bitmap.pix(y*2+1, x+xi) = m_palette->pen(pen ? color : 0);
				}
				else
				{
					if(cliprect.contains(x+xi, y))
						bitmap.pix(y, x+xi) = m_palette->pen(pen ? color : 0);
				}
			}

			count++;
		}
	}

	if(!(m_gfx_ctrl & 1)) // 400 lines
	{
		count = 0;

		for(int y = 200; y < 400; y++)
		{
			for(int x = 0; x < 640; x+=8)
			{
				if(!(m_gfx_ctrl & 1))
					is_cursor = calc_cursor_pos(x/8,y/lines_per_char,y & (lines_per_char-1));

				for(int xi = 0; xi < 8; xi++)
				{
					int pen = ((m_gvram[count+0x4000] >> (7-xi)) & 1);
					if(is_cursor)
						pen^=1;

					if(cliprect.contains(x+xi, y))
						bitmap.pix(y, x+xi) = m_palette->pen(pen ? 7 : 0);
				}

				count++;
			}
		}
	}
}

uint8_t pc8801_state::calc_cursor_pos(int x,int y,int yi)
{
	if(!(m_crtc.cursor_on)) // don't bother if cursor is off
		return 0;

	if(x == m_crtc.param[4][0] && y == m_crtc.param[4][1]) /* check if position matches */
	{
		/* don't pass through if we are using underscore */
		if((!(m_crtc.param[0][2] & 0x40)) && yi != 7)
			return 0;

		/* finally check if blinking is currently active high */
		if(!(m_crtc.param[0][2] & 0x20))
			return 1;

		if(((m_screen->frame_number() / blink_speed) & 1) == 0)
			return 1;

		return 0;
	}

	return 0;
}



uint8_t pc8801_state::extract_text_attribute(uint32_t address,int x, uint8_t width, uint8_t &non_special)
{
	uint8_t *vram = m_work_ram.get();
	int i;
	int fifo_size;
	int offset;

	non_special = 0;
	if(m_crtc.param[0][4] & 0x80)
	{
		popmessage("Using non-separate mode for text tilemap, contact MAMEdev");
		return 0;
	}

	fifo_size = (m_crtc.param[0][4] & 0x20) ? 0 : ((m_crtc.param[0][4] & 0x1f) + 1);

	if(fifo_size == 0)
	{
		non_special = 1;
		return (text_color_flag) ? 0xe8 : 0;
	}

	// NB: We offset here as a side effect of how PC8801 really handles uPD3301 attribute fetch
	offset = (vram[address] == 0) ? 2 : 0;

	for(i = 0; i < fifo_size; i++)
	{
		if(x < vram[address+offset])
		{
			return vram[address+1];
		}
		else
			address+=2;
	}

	return vram[address-3+offset];
}

void pc8801_state::draw_char(bitmap_ind16 &bitmap,int x,int y,int pal,uint8_t gfx_mode,uint8_t reverse,uint8_t secret,uint8_t blink,uint8_t upper,uint8_t lower,int y_size,int width, uint8_t non_special)
{
	uint8_t *vram = m_work_ram.get();

	uint8_t y_height = lines_per_char;
	uint8_t y_double = pixel_clock();
	// elthlead uses latter
	uint8_t y_step = (non_special) ? 80 : 120;
	uint8_t is_cursor = 0;

	for(int yi = 0; yi < y_height; yi++)
	{
		if(m_gfx_ctrl & 1)
			is_cursor = calc_cursor_pos(x,y,yi);

		for(int xi = 0; xi < 8; xi++)
		{
			int tile = vram[x+(y*y_step)+m_dma_address[2]];

			int res_x = x*8+xi*(width+1);
			int res_y = y*y_height+yi;

			if(!m_screen->visible_area().contains(res_x, res_y))
				continue;

			int color;
			if(gfx_mode)
			{
				uint8_t mask;

				mask = (xi & 4) ? 0x10 : 0x01;
				mask <<= ((yi & (0x6 << y_double)) >> (1+y_double));
				color = (tile & mask) ? pal : -1;
			}
			else
			{
				uint8_t blink_mask = 0;
				if(blink && ((m_screen->frame_number() / blink_speed) & 3) == 1)
					blink_mask = 1;

				uint8_t char_data;
				if(yi >= (1 << (y_double+3)) || secret || blink_mask)
					char_data = 0;
				else
					char_data = (m_cg_rom[tile*8+(yi >> y_double)] >> (7-xi)) & 1;

				if(yi == 0 && upper)
					char_data = 1;

				if(yi == y_height && lower)
					char_data = 1;

				if(is_cursor)
					char_data^=1;

				if(reverse)
					char_data^=1;

				color = char_data ? pal : -1;
			}

			if(color != -1)
			{
				bitmap.pix(res_y, res_x) = m_palette->pen(color);
				if(width)
				{
					if(!m_screen->visible_area().contains(res_x+1, res_y))
						continue;

					bitmap.pix(res_y, res_x+1) = m_palette->pen(color);
				}
			}
		}
	}
}

void pc8801_state::draw_text(bitmap_ind16 &bitmap,int y_size, uint8_t width)
{
	int x,y;
	uint8_t attr;
	uint8_t reverse;
	uint8_t gfx_mode;
	uint8_t secret;
	uint8_t upper;
	uint8_t lower;
	uint8_t blink;
	int pal;
	uint8_t non_special;

	for(y = 0; y < y_size; y++)
	{
		for(x = 0; x < 80; x++)
		{
			if(x & 1 && !width)
				continue;

			attr = extract_text_attribute((((y*120)+80+m_dma_address[2]) & 0xffff),(x),width,non_special);

			if(text_color_flag && (attr & 8)) // color mode
			{
				pal =  ((attr & 0xe0) >> 5);
				gfx_mode = (attr & 0x10) >> 4;
				reverse = 0;
				secret = 0;
				upper = 0;
				lower = 0;
				blink = 0;
				pal |= 8; //text PAL bank
			}
			else // monochrome
			{
				// TODO: bishojbg Pasoket logo wants this to be black instead
				pal = 7;
				gfx_mode = (attr & 0x80) >> 7;
				reverse = (attr & 4) >> 2;
				secret = (attr & 1);
				upper = (attr & 0x10) >> 4;
				lower = (attr & 0x20) >> 5;
				blink = (attr & 2) >> 1;
				pal |= 8; //text PAL bank
				reverse ^= m_crtc.inverse;

				if(attr & 0x80)
					popmessage("Warning: mono gfx mode enabled, contact MAMEdev");
			}

			draw_char(bitmap,x,y,pal,gfx_mode,reverse,secret,blink,upper,lower,y_size,!width,non_special);
		}
	}
}

uint32_t pc8801_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->pen(0), cliprect);

//  popmessage("%04x %04x %02x",m_dma_address[2],m_dma_counter[2],m_dmac_mode);

	if(m_gfx_ctrl & 8)
	{
		if(m_gfx_ctrl & 0x10)
			draw_bitmap_3bpp(bitmap,cliprect);
		else
			draw_bitmap_1bpp(bitmap,cliprect);
	}

	//popmessage("%02x %02x %02x %02x %02x",m_layer_mask,m_dmac_mode,m_crtc.status,m_crtc.irq_mask,m_gfx_ctrl);

	if(!(m_layer_mask & 1) && m_dmac_mode & 4 && m_crtc.status & 0x10 && m_crtc.irq_mask == 3)
	{
		//popmessage("%02x %02x",m_crtc.param[0][0],m_crtc.param[0][4]);

		draw_text(bitmap,screen_height,m_txt_width);
	}

	return 0;
}

uint8_t pc8801_state::alu_r(offs_t offset)
{
	uint8_t b, r, g;

	/* store data to ALU regs */
	for(int i = 0; i < 3; i++)
		m_alu_reg[i] = m_gvram[i*0x4000 + offset];

	b = m_gvram[offset + 0x0000];
	r = m_gvram[offset + 0x4000];
	g = m_gvram[offset + 0x8000];
	if(!(m_alu_ctrl2 & 1)) { b^=0xff; }
	if(!(m_alu_ctrl2 & 2)) { r^=0xff; }
	if(!(m_alu_ctrl2 & 4)) { g^=0xff; }

	return b & r & g;
}

void pc8801_state::alu_w(offs_t offset, uint8_t data)
{
	int i;

	// ALU write mode
	switch(m_alu_ctrl2 & 0x30)
	{
		// logic operation
		case 0x00:
		{
			uint8_t logic_op;

			for(i = 0; i < 3; i++)
			{
				logic_op = (m_alu_ctrl1 & (0x11 << i)) >> i;

				switch(logic_op)
				{
					case 0x00: { m_gvram[i*0x4000 + offset] &= ~data; } break;
					case 0x01: { m_gvram[i*0x4000 + offset] |= data; } break;
					case 0x10: { m_gvram[i*0x4000 + offset] ^= data; } break;
					case 0x11: break; // NOP
				}
			}
		}
		break;

		// restore data from ALU regs
		case 0x10:
		{
			for(i = 0; i < 3; i++)
				m_gvram[i*0x4000 + offset] = m_alu_reg[i];
		}
		break;

		// swap ALU reg 1 into R GVRAM
		case 0x20:
			m_gvram[0x0000 + offset] = m_alu_reg[1];
			break;

		// swap ALU reg 0 into B GVRAM
		case 0x30:
			m_gvram[0x4000 + offset] = m_alu_reg[0];
			break;
	}
}


uint8_t pc8801_state::wram_r(offs_t offset)
{
	return m_work_ram[offset];
}

void pc8801_state::wram_w(offs_t offset, uint8_t data)
{
	m_work_ram[offset] = data;
}

uint8_t pc8801_state::ext_wram_r(offs_t offset)
{
	if(offset < m_extram_size)
		return m_ext_work_ram[offset];

	return 0xff;
}

void pc8801_state::ext_wram_w(offs_t offset, uint8_t data)
{
	if(offset < m_extram_size)
		m_ext_work_ram[offset] = data;
}

uint8_t pc8801_state::nbasic_rom_r(offs_t offset)
{
	return m_n80rom[offset];
}

uint8_t pc8801_state::n88basic_rom_r(offs_t offset)
{
	return m_n88rom[offset];
}

uint8_t pc8801_state::gvram_r(offs_t offset)
{
	return m_gvram[offset];
}

void pc8801_state::gvram_w(offs_t offset, uint8_t data)
{
	m_gvram[offset] = data;
}

uint8_t pc8801_state::high_wram_r(offs_t offset)
{
	return m_hi_work_ram[offset];
}

void pc8801_state::high_wram_w(offs_t offset, uint8_t data)
{
	m_hi_work_ram[offset] = data;
}

// TODO: remove these virtual trampolines once we modernize memory map
// Needs confirmation about really not being there tho, given the design
// may be that both dictionary and CD-ROM are generic slots instead.
inline uint8_t pc8801_state::dictionary_rom_r(offs_t offset)
{
	return 0xff;
}

inline bool pc8801_state::dictionary_rom_enable()
{
	return false;
}

inline uint8_t pc8801_state::cdbios_rom_r(offs_t offset)
{
	return 0xff;
}

inline bool pc8801_state::cdbios_rom_enable()
{
	return false;
}

uint8_t pc8801_state::mem_r(offs_t offset)
{
	if(offset <= 0x7fff)
	{
		if(m_extram_mode & 1)
			return ext_wram_r(offset | (m_extram_bank * 0x8000));

		if(m_gfx_ctrl & 2)
			return wram_r(offset);

		if(cdbios_rom_enable())
			return cdbios_rom_r(offset & 0x7fff);

		if(m_gfx_ctrl & 4)
			return nbasic_rom_r(offset);

		if(offset >= 0x6000 && offset <= 0x7fff && ((m_ext_rom_bank & 1) == 0))
			return n88basic_rom_r(0x8000 + (offset & 0x1fff) + (0x2000 * (m_misc_ctrl & 3)));

		return n88basic_rom_r(offset);
	}
	else if(offset >= 0x8000 && offset <= 0x83ff) // work RAM window
	{
		uint32_t window_offset;

		// work RAM read select or N-Basic select always banks this as normal work RAM
		if(m_gfx_ctrl & 6)
			return wram_r(offset);

		window_offset = (offset & 0x3ff) + (m_window_offset_bank << 8);

		// castlex and imenes accesses this
		// TODO: high TVRAM even
		if(((window_offset & 0xf000) == 0xf000) && (m_misc_ctrl & 0x10))
			return high_wram_r(window_offset & 0xfff);

		return wram_r(window_offset);
	}
	else if(offset >= 0x8400 && offset <= 0xbfff)
	{
		return wram_r(offset);
	}
	else if(offset >= 0xc000 && offset <= 0xffff)
	{
		if(dictionary_rom_enable())
			return dictionary_rom_r(offset & 0x3fff);

		if(m_misc_ctrl & 0x40)
		{
			if(!machine().side_effects_disabled())
				m_vram_sel = 3;

			if(m_alu_ctrl2 & 0x80)
				return alu_r(offset & 0x3fff);
		}

		if(m_vram_sel == 3)
		{
			if(offset >= 0xf000 && offset <= 0xffff && (m_misc_ctrl & 0x10))
				return high_wram_r(offset & 0xfff);

			return wram_r(offset);
		}

		return gvram_r((offset & 0x3fff) + (0x4000 * m_vram_sel));
	}

	return 0xff;
}

void pc8801_state::mem_w(offs_t offset, uint8_t data)
{
	if(offset <= 0x7fff)
	{
		if(m_extram_mode & 0x10)
			ext_wram_w(offset | (m_extram_bank * 0x8000),data);
		else
			wram_w(offset,data);

		return;
	}
	else if(offset >= 0x8000 && offset <= 0x83ff)
	{
		// work RAM read select or N-Basic select always banks this as normal work RAM
		if(m_gfx_ctrl & 6)
			wram_w(offset,data);
		else
		{
			uint32_t window_offset;

			window_offset = (offset & 0x3ff) + (m_window_offset_bank << 8);

			// castlex and imenes accesses this
			// TODO: high TVRAM even
			// μPD3301 DMAs from this instead of the regular work RAM in later models
			// to resolve a bus bottleneck.
			if(((window_offset & 0xf000) == 0xf000) && (m_misc_ctrl & 0x10))
				high_wram_w(window_offset & 0xfff,data);
			else
				wram_w(window_offset,data);
		}

		return;
	}
	else if(offset >= 0x8400 && offset <= 0xbfff)
	{
		wram_w(offset,data);
		return;
	}
	else if(offset >= 0xc000 && offset <= 0xffff)
	{
		if(m_misc_ctrl & 0x40)
		{
			if(!machine().side_effects_disabled())
				m_vram_sel = 3;

			if(m_alu_ctrl2 & 0x80)
			{
				alu_w(offset & 0x3fff,data);
				return;
			}
		}

		if(m_vram_sel == 3)
		{
			if(offset >= 0xf000 && offset <= 0xffff && (m_misc_ctrl & 0x10))
			{
				high_wram_w(offset & 0xfff,data);
				return;
			}

			wram_w(offset,data);
			return;
		}

		gvram_w((offset & 0x3fff) + (0x4000 * m_vram_sel),data);
		return;
	}
}

void pc8801_state::main_map(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(pc8801_state::mem_r), FUNC(pc8801_state::mem_w));
}

uint8_t pc8801_state::ext_rom_bank_r()
{
	return m_ext_rom_bank;
}

void pc8801_state::ext_rom_bank_w(uint8_t data)
{
	m_ext_rom_bank = data;
}

uint8_t pc8801_state::pixel_clock(void)
{
	// TODO: pinpoint exact condition
	int ysize = m_screen->height();

	return (ysize >= 400);
}

void pc8801_state::dynamic_res_change(void)
{
	rectangle visarea;
	int xsize,ysize,xvis,yvis;
	attoseconds_t refresh;;

	/* bail out if screen params aren't valid */
	if(!m_crtc.param[0][0] || !m_crtc.param[0][1] || !m_crtc.param[0][2] || !m_crtc.param[0][3])
		return;

	xvis = screen_width;
	yvis = screen_height * lines_per_char;
	xsize = screen_width + hretrace;
	ysize = screen_height * lines_per_char + vretrace * lines_per_char;

//  popmessage("H %d V %d (%d x %d) HR %d VR %d (%d %d)\n",xvis,yvis,screen_height,lines_per_char,hretrace,vretrace, xsize,ysize);

	visarea.set(0, xvis - 1, 0, yvis - 1);
	if(pixel_clock())
		refresh = HZ_TO_ATTOSECONDS(PIXEL_CLOCK_24KHz) * (xsize) * ysize;
	else
		refresh = HZ_TO_ATTOSECONDS(PIXEL_CLOCK_15KHz) * (xsize) * ysize;

	m_screen->configure(xsize, ysize, visarea, refresh);
}

/*
 * I/O Port $30 (w/o) "System Control Port (1)"
 * N88-BASIC buffer port $e6c0
 *
 * Virtually same as the correlated PC-8001 port
 *
 * --xx ---- BS2, BS1: USART channel control
 * --00 ----           CMT 600 bps
 * --01 ----           CMT 1200 bps
 * --10 ----           RS-232C async mode
 * --11 ----           RS-232C sync mode
 * ---- x--- MTON: CMT motor control (active high)
 * ---- -x-- CDS: CMT carrier control (1) mark (0) space
 * ---- --x- /COLOR: CRT display mode control (1) color mode (0) monochrome
 * ---- ---x /40: CRT display format control (1) 80 chars per line (0) 40 chars
 *
 */
void pc8801_state::port30_w(uint8_t data)
{
	m_txt_width = data & 1;
	m_txt_color = data & 2;

	m_cassette->change_state(BIT(data, 3) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/*
 * I/O Port $31 (w/o) "System Control Port (2)"
 * N88-BASIC buffer port $e6c2
 *
 * --x- ---- 25LINE: line control in high speed CRT mode (1) 25 lines (0) 20 lines
 * ---x ---- HCOLOR: color graphic display mode
 * ---1 ----         color mode
 * ---0 ----         monochrome mode
 * ---- x--- GRPH: Graphic display mode yes (1) / no (0)
 * ---- -x-- RMODE: ROM mode control N-BASIC (1, ROM 1 & 2) / N88-BASIC (0, ROM 3 & 4)
 * ---- --x- MMODE: RAM mode control yes (1, full RAM) / no (0, ROM/RAM mixed)
 * ---- ---x 200LINE: 200 lines (1) / 400 lines (0) in 1bpp mode
 *
 */
void pc8801_state::port31_w(uint8_t data)
{
	m_gfx_ctrl = data;

	dynamic_res_change();
}

/*
 * I/O Port $40 reads "Strobe Port"
 *
 * 1--- ---- UOP2: SW1-8
 * -1-- ---- UOP1:
 * --x- ---- VRTC: vblank signal (0) display (1) vblank
 * ---x ---- CDI: upd1990a data read
 * ---- x--- /EXTON: Minidisc unit connection signal (SW2-7)
 * ---- -x-- DCD: SIO Data Carrier Detect signal (0) no carrier (1) with
 * ---- --x- /SHG: monitor resolution mode (0) high res (1) normal res
 * ---- ---x BUSY: printer (0) READY (1) BUSY
 *
 */
uint8_t pc8801_state::port40_r()
{
	return ioport("CTRL")->read();
}

inline attotime pc8801_state::mouse_limit_hz()
{
	return attotime::from_hz(900);
}

inline attotime pc8801fh_state::mouse_limit_hz()
{
	return attotime::from_hz(m_clock_setting ? 900 : 1800);
}

/*
 * I/O Port $40 writes "Strobe Port"
 * N88-BASIC buffer port $e6c1
 *
 * x--- ---- UOP2: general purpose output 2 / sound port
 *                 SING (buzzer mask?)
 * -x-- ---- UOP1: general purpose output 1
 *                 generally used for mouse latch (JOP1, routes on OPN sound port A)
 * --x- ---- BEEP: beeper enable
 * ---x ---- FLASH: flash mode control (active high)
 * ---- x--- /CLDS: "CRT I/F sync control" (init CRT and controller sync pulses?)
 * ---- -x-- CCK: upd1990a clock bit
 * ---- --x- CSTB: upd1990a strobe bit
 * ---- ---x /PSTB: printer strobe (active low)
 */
void pc8801_state::port40_w(uint8_t data)
{
	m_rtc->stb_w((data & 2) >> 1);
	m_rtc->clk_w((data & 4) >> 2);

	if(((m_device_ctrl_data & 0x20) == 0x00) && ((data & 0x20) == 0x20))
		m_beeper->set_state(1);

	if(((m_device_ctrl_data & 0x20) == 0x20) && ((data & 0x20) == 0x00))
		m_beeper->set_state(0);

	// TODO: send to joyport implementation
	if((m_device_ctrl_data & 0x40) != (data & 0x40))
	{
		attotime new_time = machine().time();

		if(data & 0x40 && (new_time - m_mouse.time) > mouse_limit_hz())
		{
			m_mouse.phase = 0;
		}
		else
		{
			m_mouse.phase ++;
			m_mouse.phase &= 3;
		}

		if(m_mouse.phase == 0)
		{
			const u8 mouse_x = ioport("MOUSEX")->read();
			const u8 mouse_y = ioport("MOUSEY")->read();

			m_mouse.lx = (mouse_x - m_mouse.prev_dx) & 0xff;
			m_mouse.ly = (mouse_y - m_mouse.prev_dy) & 0xff;

			m_mouse.prev_dx = mouse_x;
			m_mouse.prev_dy = mouse_y;
		}

		m_mouse.time = machine().time();
	}

	// TODO: is SING a buzzer mask? bastard leaves beeper to ON state otherwise
	if(m_device_ctrl_data & 0x80)
		m_beeper->set_state(0);

	m_device_ctrl_data = data;
}

uint8_t pc8801_state::vram_select_r()
{
	return 0xf8 | ((m_vram_sel == 3) ? 0 : (1 << m_vram_sel));
}

void pc8801_state::vram_select_w(offs_t offset, uint8_t data)
{
	m_vram_sel = offset & 3;
}

void pc8801_state::irq_level_w(uint8_t data)
{
	m_pic->b_sgs_w(~data);
}

/*
 * ---- -x-- /RXMF RXRDY irq mask
 * ---- --x- /VRMF VRTC irq mask
 * ---- ---x /RTMF Real-time clock irq mask
 *
 */
void pc8801_state::irq_mask_w(uint8_t data)
{
	m_timer_irq_enable = bool(BIT(data, 0));
	m_vrtc_irq_enable = bool(BIT(data, 1));
	// Pulled high when cassette LOAD command is issued
	m_rxrdy_irq_enable = bool(BIT(data, 2));

	if (m_timer_irq_enable && m_timer_irq_pending)
	{
		m_timer_irq_pending = false;
		m_pic->r_w(7 ^ 2, 0);
	}
}


uint8_t pc8801_state::window_bank_r()
{
	return m_window_offset_bank;
}

void pc8801_state::window_bank_w(uint8_t data)
{
	m_window_offset_bank = data;
}

void pc8801_state::window_bank_inc_w(uint8_t data)
{
	m_window_offset_bank ++;
	m_window_offset_bank &= 0xff;
}

/*
 * I/O Port $32 (R/W)
 * Not on vanilla PC-8801 (mkII onward)
 *
 * x--- ---- sound irq mask (0) irq enabled (1) irq masked
 * -x-- ---- Graphic VRAM access mode (0) independent access mode (1) ALU mode
 * --x- ---- analog (1) / digital (0) palette select
 * ---x ---- high speed RAM select (for TVRAM) (1) main RAM bank (0) dedicated Text RAM
 * ---- xx-- Screen output mode
 * ---- 00-- TV / video mode
 * ---- 01-- None (as in disabling the screen entirely?)
 * ---- 10-- Analog RGB mode
 * ---- 11-- Optional mode
 * ---- --xx internal EROM selection
 */
uint8_t pc8801_state::misc_ctrl_r()
{
	return m_misc_ctrl;
}

void pc8801_state::misc_ctrl_w(uint8_t data)
{
	m_misc_ctrl = data;

	m_sound_irq_enable = ((data & 0x80) == 0);

	// refresh INT4 state if irq is enabled
	// Note: this will map to no irq anyway if there's no internal OPN/OPNA
	if (m_sound_irq_enable)
		int4_irq_w(m_sound_irq_pending);
}

/*
 * I/O Port $52 "Border and background color control"
 *
 * -RGB ---- BGx: Background color, index for pen #0
 * ---- -RGB Rx: Border color?
 *           (NB: according to some sources a V2 equipped machine hardwires this to black)
 *
 */
void pc8801_state::bgpal_w(uint8_t data)
{
	// TODO: sorcerml uses index on main playlist (0x10 setting, should have blue instead of black)

	if(data)
		logerror("BG Pal %02x\n",data);
}

void pc8801_state::palram_w(offs_t offset, uint8_t data)
{
	if(m_misc_ctrl & 0x20) //analog palette
	{
		if((data & 0x40) == 0)
		{
			m_palram[offset].b = data & 0x7;
			m_palram[offset].r = (data & 0x38) >> 3;
		}
		else
		{
			m_palram[offset].g = data & 0x7;
		}
	}
	else //digital palette
	{
		m_palram[offset].b = data & 1 ? 7 : 0;
		m_palram[offset].r = data & 2 ? 7 : 0;
		m_palram[offset].g = data & 4 ? 7 : 0;
	}

	// TODO: sorcerml writes white to pen #0 on initial bootup, causing the underlying message to not be visible
	// "Press space to load disk in drive 2\nSound Board installed (Y/N)"
	// TODO: What happens to the palette contents when the analog/digital palette mode changes?
	// Preserve content? Translation? Undefined?
	m_palette->set_pen_color(offset, pal3bit(m_palram[offset].r), pal3bit(m_palram[offset].g), pal3bit(m_palram[offset].b));
	// TODO: at least analog mode can do rasters
}


/*
 * ---- x--- green gvram masked flag
 * ---- -x-- red gvram masked flag
 * ---- --x- blue gvram masked flag
 * ---- ---x text vram masked
 */
void pc8801_state::layer_masking_w(uint8_t data)
{
	m_layer_mask = data;
}

uint8_t pc8801_state::crtc_param_r()
{
	logerror("CRTC param reading\n");
	return 0xff;
}

void pc8801_state::crtc_param_w(uint8_t data)
{
	if(m_crtc.param_count < 5)
	{
		m_crtc.param[m_crtc.cmd][m_crtc.param_count] = data;
		if(m_crtc.cmd == 0)
			dynamic_res_change();

		m_crtc.param_count++;
	}
}

uint8_t pc8801_state::crtc_status_r()
{
	/*
	---x ---- video enable
	---- x--- DMA is running
	---- -x-- special control character IRQ
	---- --x- indication end IRQ
	---- ---x light pen input
	*/

	return m_crtc.status;
}

#if 0
static const char *const crtc_command[] =
{
	"Reset / Stop Display",             // 0
	"Start Display",                    // 1
	"Set IRQ MASK",                     // 2
	"Read Light Pen",                   // 3
	"Load Cursor Position",             // 4
	"Reset IRQ",                        // 5
	"Reset Counters",                   // 6
	"Read Status"                       // 7
};
#endif

void pc8801_state::crtc_cmd_w(uint8_t data)
{
	m_crtc.cmd = (data & 0xe0) >> 5;
	m_crtc.param_count = 0;

	switch(m_crtc.cmd)
	{
		case 0:  // reset CRTC
			m_crtc.status &= (~0x16);
			// TODO: honor device IRQ masks
			// megamit and babylon in particular expects that the VRTC irq disables during boot
			// otherwise they hangs up.
			// i.e. former will try to execute a spurious irq and jump to PC=0
			// Notice that no SW actually writes a "0" to the IRQ mask register,
			// which supposedly enable irqs from 3301 according to docs.
			// Other noteworthy checks that needs to be done on actual device hookup:
			// - BIOS BASIC itself (definitely needs VRTC irqs otherwise locks up on "How many files" prompt);
			// - xzr2 (locks up after map -> main gameplay screen if this isn't handled right);
			// - ashurano (expecting a VRTC after title screen, never reading inputs without, **regressed**);
			m_vrtc_irq_enable = false;
			break;
		case 1:  // start display
			m_crtc.status |= 0x10;
			m_crtc.status &= (~0x08);
			// cfr. pc8001 games for 3301 RVV bit
			m_crtc.inverse = data & 1;
			break;
		case 2:  // set irq mask
			m_crtc.irq_mask = data & 3;
			break;
		case 3:  // read light pen
			m_crtc.status &= (~0x01);
			break;
		case 4:  // load cursor position ON/OFF
			m_crtc.cursor_on = data & 1;
			break;
		case 5:  // reset IRQ
		case 6:  // reset counters
			m_crtc.status &= (~0x06);
			break;
	}

//  if((data >> 5) != 4)
//      printf("CRTC cmd %s polled %02x\n",crtc_command[data >> 5],data & 0x1f);
}

uint8_t pc8801_state::dmac_r(offs_t offset)
{
	logerror("DMAC R %08x\n",offset);
	return 0xff;
}

// CH0: 5-inch floppy DMA
// CH1: 8-inch floppy DMA
// CH2: CRTC
// CH3: CD-ROM and probably HxC etc.
void pc8801_state::dmac_w(offs_t offset, uint8_t data)
{
	if(offset & 1)
		m_dma_counter[offset >> 1] = (m_dmac_ff) ? (m_dma_counter[offset >> 1]&0xff)|(data<<8) : (m_dma_counter[offset >> 1]&0xff00)|(data&0xff);
	else
		m_dma_address[offset >> 1] = (m_dmac_ff) ? (m_dma_address[offset >> 1]&0xff)|(data<<8) : (m_dma_address[offset >> 1]&0xff00)|(data&0xff);

	m_dmac_ff ^= 1;
}

uint8_t pc8801_state::dmac_status_r()
{
	//logerror("DMAC R STATUS\n");
	return 0xff;
}

void pc8801_state::dmac_mode_w(uint8_t data)
{
	m_dmac_mode = data;
	m_dmac_ff = 0;

	// Valis II sets 0x20
	//if(data != 0xe4 && data != 0xa0 && data != 0xc4 && data != 0x80 && data != 0x00)
	//  logerror("%02x DMAC mode\n",data);
}

uint8_t pc8801_state::extram_mode_r()
{
	return (m_extram_mode ^ 0x11) | 0xee;
}

void pc8801_state::extram_mode_w(uint8_t data)
{
	/*
	---x ---- Write EXT RAM access at 0x0000 - 0x7fff
	---- ---x Read EXT RAM access at 0x0000 - 0x7fff
	*/

	m_extram_mode = data & 0x11;
}

uint8_t pc8801_state::extram_bank_r()
{
	return m_extram_bank;
}

void pc8801_state::extram_bank_w(uint8_t data)
{
	m_extram_bank = data;
}

void pc8801_state::alu_ctrl1_w(uint8_t data)
{
	m_alu_ctrl1 = data;
}

void pc8801_state::alu_ctrl2_w(uint8_t data)
{
	m_alu_ctrl2 = data;
}

// TODO: Implement PCG-8100 as a bus option
// It's an HAL Laboratory custom board with PCG (maps to chars $80-$ff),
// dual AY-3-891x & PIT, I/O $b0-$b2 is the I/O ID for it?
// Find a supported SW (only HAL seems to support it) & investigate
void pc8801_state::pcg8100_w(offs_t offset, uint8_t data)
{
	logerror("%s: Possible write to PCG-8100 %02x %02x\n", machine().describe_context(), offset, data);
}

/*
 * $e8-$eb kanji LV1
 * $ec-$ef kanji LV2
 *
 */
template <unsigned kanji_level> uint8_t pc8801_state::kanji_r(offs_t offset)
{
	if((offset & 2) == 0)
	{
		const u8 *kanji_rom = kanji_level ? m_kanji_lv2_rom : m_kanji_rom;
		const u32 kanji_address = (m_knj_addr[kanji_level] * 2) + ((offset & 1) ^ 1);
		return kanji_rom[kanji_address];
	}

	return 0xff;
}

template <unsigned kanji_level> void pc8801_state::kanji_w(offs_t offset, uint8_t data)
{
	if((offset & 2) == 0)
	{
		m_knj_addr[kanji_level] = (
			((offset & 1) == 0) ?
			((m_knj_addr[kanji_level] & 0xff00) | (data & 0xff)) :
			((m_knj_addr[kanji_level] & 0x00ff) | (data << 8))
		);
	}
	// TODO: document and implement what the upper two regs does
	// read latches on write? "read start/end sign" according to
	// https://retrocomputerpeople.web.fc2.com/machines/nec/8801/io_map88.html
}

void pc8801_state::rtc_w(uint8_t data)
{
	m_rtc->c0_w((data & 1) >> 0);
	m_rtc->c1_w((data & 2) >> 1);
	m_rtc->c2_w((data & 4) >> 2);
	m_rtc->data_in_w((data & 8) >> 3);

	// TODO: remaining bits
}

#if 0
uint8_t pc8801_state::sound_board_r(offs_t offset)
{
	if(m_has_opna)
		return m_opna->read(offset);

	return (offset & 2) ? 0xff : m_opn->read(offset);
}

void pc8801_state::sound_board_w(offs_t offset, uint8_t data)
{
	if(m_has_opna)
		m_opna->write(offset, data);
	else if((offset & 2) == 0)
		m_opn->write(offset, data);
}

uint8_t pc8801_state::opna_r(offs_t offset)
{
	if(m_has_opna && (offset & 2) == 0)
		return m_opna->read((offset & 1) | ((offset & 4) >> 1));

	return 0xff;
}

void pc8801_state::opna_w(offs_t offset, uint8_t data)
{
	if(m_has_opna && (offset & 2) == 0)
		m_opna->write((offset & 1) | ((offset & 4) >> 1),data);
	else if(m_has_opna && offset == 2)
	{
		// TODO: tied to second sound chip (noticeable in late doujinshi entries)
		//m_sound_irq_mask = ((data & 0x80) == 0);

		m_sound_irq_mask = ((data & 0x80) == 0);

		if(m_sound_irq_mask == 0)
			m_sound_irq_latch = 0;

		if(m_timer_irq_latch == 0 && m_vrtc_irq_latch == 0 && m_sound_irq_latch == 0)
			m_maincpu->set_input_line(0,CLEAR_LINE);

		if(m_sound_irq_mask && m_sound_irq_pending)
		{
			m_maincpu->set_input_line(0,HOLD_LINE);
			m_sound_irq_latch = 1;
			m_sound_irq_pending = 0;
		}
	}
}
#endif

/*
 * PC8801FH overrides (CPU clock switch)
 */

uint8_t pc8801fh_state::cpuclock_r()
{
	return 0x10 | m_clock_setting;
}

uint8_t pc8801fh_state::baudrate_r()
{
	return 0xf0 | m_baudrate_val;
}

void pc8801fh_state::baudrate_w(uint8_t data)
{
	m_baudrate_val = data & 0xf;
}

/*
 * PC8801MA overrides (dictionary)
 */

inline uint8_t pc8801ma_state::dictionary_rom_r(offs_t offset)
{
	return m_dictionary_rom[offset + ((m_dic_bank & 0x1f) * 0x4000)];
}

inline bool pc8801ma_state::dictionary_rom_enable()
{
	return m_dic_ctrl;
}

void pc8801ma_state::dic_bank_w(uint8_t data)
{
	m_dic_bank = data & 0x1f;
}

void pc8801ma_state::dic_ctrl_w(uint8_t data)
{
	m_dic_ctrl = (data ^ 1) & 1;
}

/*
 * PC8801MC overrides (CD-ROM)
 */

inline uint8_t pc8801mc_state::cdbios_rom_r(offs_t offset)
{
	return m_cdrom_bios[offset | ((m_gfx_ctrl & 4) ? 0x8000 : 0x0000)];
}

inline bool pc8801mc_state::cdbios_rom_enable()
{
	return m_cdrom_bank;
}

void pc8801_state::main_io(address_map &map)
{
	map.global_mask(0xff);
	map.unmap_value_high();
	map(0x00, 0x00).portr("KEY0");
	map(0x01, 0x01).portr("KEY1");
	map(0x02, 0x02).portr("KEY2");
	map(0x03, 0x03).portr("KEY3");
	map(0x04, 0x04).portr("KEY4");
	map(0x05, 0x05).portr("KEY5");
	map(0x06, 0x06).portr("KEY6");
	map(0x07, 0x07).portr("KEY7");
	map(0x08, 0x08).portr("KEY8");
	map(0x09, 0x09).portr("KEY9");
	map(0x0a, 0x0a).portr("KEY10");
	map(0x0b, 0x0b).portr("KEY11");
	map(0x0c, 0x0c).portr("KEY12");
	map(0x0d, 0x0d).portr("KEY13");
	map(0x0e, 0x0e).portr("KEY14");
	map(0x0f, 0x0f).portr("KEY15");
	map(0x00, 0x02).w(FUNC(pc8801_state::pcg8100_w));
	map(0x10, 0x10).w(FUNC(pc8801_state::rtc_w));
	map(0x20, 0x21).mirror(0x0e).rw(m_usart, FUNC(i8251_device::read), FUNC(i8251_device::write)); // CMT / RS-232C ch. 0
	map(0x30, 0x30).portr("DSW1").w(FUNC(pc8801_state::port30_w));
	map(0x31, 0x31).portr("DSW2").w(FUNC(pc8801_state::port31_w));
	map(0x32, 0x32).rw(FUNC(pc8801_state::misc_ctrl_r), FUNC(pc8801_state::misc_ctrl_w));
//  map(0x33, 0x33) PC8001mkIISR port, mirror on PC8801?
	// TODO: ALU not installed on pre-mkIISR machines
	map(0x34, 0x34).w(FUNC(pc8801_state::alu_ctrl1_w));
	map(0x35, 0x35).w(FUNC(pc8801_state::alu_ctrl2_w));
	map(0x40, 0x40).rw(FUNC(pc8801_state::port40_r), FUNC(pc8801_state::port40_w));
//  map(0x44, 0x47).rw internal OPN/OPNA sound card for 8801mkIISR and beyond
//  uPD3301
	map(0x50, 0x50).rw(FUNC(pc8801_state::crtc_param_r), FUNC(pc8801_state::crtc_param_w));
	map(0x51, 0x51).rw(FUNC(pc8801_state::crtc_status_r), FUNC(pc8801_state::crtc_cmd_w));

	map(0x52, 0x52).w(FUNC(pc8801_state::bgpal_w));
	map(0x53, 0x53).w(FUNC(pc8801_state::layer_masking_w));
	map(0x54, 0x5b).w(FUNC(pc8801_state::palram_w));
	map(0x5c, 0x5c).r(FUNC(pc8801_state::vram_select_r));
	map(0x5c, 0x5f).w(FUNC(pc8801_state::vram_select_w));
//  i8257
	map(0x60, 0x67).rw(FUNC(pc8801_state::dmac_r), FUNC(pc8801_state::dmac_w));
	map(0x68, 0x68).rw(FUNC(pc8801_state::dmac_status_r), FUNC(pc8801_state::dmac_mode_w));

//  map(0x6e, 0x6f) clock settings (8801FH and later)
	map(0x70, 0x70).rw(FUNC(pc8801_state::window_bank_r), FUNC(pc8801_state::window_bank_w));
	map(0x71, 0x71).rw(FUNC(pc8801_state::ext_rom_bank_r), FUNC(pc8801_state::ext_rom_bank_w));
	map(0x78, 0x78).w(FUNC(pc8801_state::window_bank_inc_w));
//  map(0x8e, 0x8e).r accessed by scruiser on boot, unknown purpose (a board ID?)
//  map(0x90, 0x9f) PC-8801-31 CD-ROM i/f (8801MC)
//  map(0xa0, 0xa3) GSX-8800 or network board
//  map(0xa8, 0xad).rw expansion OPN (Sound Board) or OPNA (Sound Board II)
//  map(0xb0, 0xb3) General purpose parallel I/O (i8255?)
//  map(0xb4, 0xb4) PC-8801-17 Video art board
//  map(0xb5, 0xb5) PC-8801-18 Video digitizing unit
//  map(0xbc, 0xbf) External mini floppy disk I/F (i8255), PC-8801-13 / -20 / -22
//  map(0xc0, 0xc3) USART RS-232C ch. 1 / ch. 2
//  map(0xc4, 0xc7) PC-8801-10 Music interface board (MIDI), GSX-8800 PIT?
//  map(0xc8, 0xc8) RS-232C ch. 1 "prohibited gate" (?)
//  map(0xca, 0xca) RS-232C ch. 2 "prohibited gate" (?)
//  map(0xd0, 0xdf) GP-IB
//  map(0xd3, 0xd4) PC-8801-10 Music interface board (MIDI)
//  map(0xdc, 0xdf) PC-8801-12 MODEM (built-in for mkIITR)
	// $e2-$e3 are standard for mkIIMR, MH / MA / MA2 / MC
	// also used by expansion boards -02 / -02N, -22,
	// and -17 video art board (transfers from RAM?)
	map(0xe2, 0xe2).rw(FUNC(pc8801_state::extram_mode_r), FUNC(pc8801_state::extram_mode_w));
	map(0xe3, 0xe3).rw(FUNC(pc8801_state::extram_bank_r), FUNC(pc8801_state::extram_bank_w));
	map(0xe4, 0xe4).w(FUNC(pc8801_state::irq_level_w));
	map(0xe6, 0xe6).w(FUNC(pc8801_state::irq_mask_w));
//  map(0xe7, 0xe7).noprw(); /* arcus writes here, mirror of above? */
	map(0xe8, 0xeb).rw(FUNC(pc8801_state::kanji_r<0>), FUNC(pc8801_state::kanji_w<0>));
	map(0xec, 0xef).rw(FUNC(pc8801_state::kanji_r<1>), FUNC(pc8801_state::kanji_w<1>));
//  map(0xf0, 0xf1) dictionary bank (8801MA and later)
//  map(0xf3, 0xf3) DMA floppy (unknown)
//  map(0xf4, 0xf7) DMA 5'25-inch floppy (?)
//  map(0xf8, 0xfb) DMA 8-inch floppy (?)
	map(0xfc, 0xff).m(m_pc80s31, FUNC(pc80s31_device::host_map));
}

void pc8801mk2sr_state::main_io(address_map &map)
{
	pc8801_state::main_io(map);
	map(0x44, 0x45).rw(m_opn, FUNC(ym2203_device::read), FUNC(ym2203_device::write));
}

void pc8801fh_state::main_io(address_map &map)
{
	pc8801_state::main_io(map);
	map(0x44, 0x47).rw(m_opna, FUNC(ym2608_device::read), FUNC(ym2608_device::write));

	map(0x6e, 0x6e).r(FUNC(pc8801fh_state::cpuclock_r));
	map(0x6f, 0x6f).rw(FUNC(pc8801fh_state::baudrate_r), FUNC(pc8801fh_state::baudrate_w));
}

void pc8801ma_state::main_io(address_map &map)
{
	pc8801fh_state::main_io(map);
	map(0xf0, 0xf0).w(FUNC(pc8801ma_state::dic_bank_w));
	map(0xf1, 0xf1).w(FUNC(pc8801ma_state::dic_ctrl_w));
}

void pc8801mc_state::main_io(address_map &map)
{
	pc8801ma_state::main_io(map);
	map(0x90, 0x9f).m(m_cdrom_if, FUNC(pc8801_31_device::amap));
}

void pc8801fh_state::opna_map(address_map &map)
{
	// TODO: confirm it really is ROMless
	// TODO: confirm size
	map(0x000000, 0x1fffff).ram();
}

/* Input Ports */

/* 2008-05 FP:
Small note about the strange default mapping of function keys:
the top line of keys in PC8801 keyboard is as follows
[STOP][COPY]      [F1][F2][F3][F4][F5]      [ROLL UP][ROLL DOWN]
Therefore, in Full Emulation mode, "F1" goes to 'F3' and so on

Also, the Keypad has 16 keys, making impossible to map it in a satisfactory
way to a PC keypad. Therefore, default settings for these keys in Full
Emulation are currently based on the effect of the key rather than on
their real position

About natural keyboards: currently,
- "Stop" is mapped to 'Pause'
- "Copy" is mapped to 'Print Screen'
- "Kana" is mapped to 'F6'
- "Grph" is mapped to 'F7'
- "Roll Up" and "Roll Down" are mapped to 'Page Up' and 'Page Down'
- "Help" is mapped to 'F8'
 */

static INPUT_PORTS_START( pc8801 )
	PORT_START("KEY0")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)       PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)       PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)       PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)       PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)       PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)       PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)       PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)       PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("KEY1")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)       PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)       PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)    PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)    PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PGUP)        PORT_CHAR(UCHAR_MAMEKEY(EQUALS_PAD))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PGDN)        PORT_CHAR(UCHAR_MAMEKEY(COMMA_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)     PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)

	PORT_START("KEY2")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)   PORT_CHAR('@')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)           PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)           PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)           PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)           PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)           PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)           PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)           PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("KEY3")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)           PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)           PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)           PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)           PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)           PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)           PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)           PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)           PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY4")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)           PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)           PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)           PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)           PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)           PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)           PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)           PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)           PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("KEY5")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)           PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)           PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)           PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)  PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)  PORT_CHAR(0xA5) PORT_CHAR('|')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)   PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)      PORT_CHAR('^')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)       PORT_CHAR('-') PORT_CHAR('=')

	PORT_START("KEY6")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)           PORT_CHAR('0')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)           PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)           PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)           PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)           PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)           PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)           PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)           PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("KEY7")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)           PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)           PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)       PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)       PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)       PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)        PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)       PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("  _") PORT_CODE(KEYCODE_DEL)            PORT_CHAR(0) PORT_CHAR('_')

	PORT_START("KEY8")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clr Home") PORT_CODE(KEYCODE_HOME)      PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP)   PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_RIGHT) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del Ins") PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(UCHAR_MAMEKEY(DEL)) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Grph") PORT_CODE(KEYCODE_LALT)  PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Kana") PORT_CODE(KEYCODE_LCONTROL) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RCONTROL)                        PORT_CHAR(UCHAR_SHIFT_2)

	PORT_START("KEY9")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_F1)            PORT_CHAR(UCHAR_MAMEKEY(PAUSE))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3)                              PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)                              PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5)                              PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6)                              PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7)                              PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)                           PORT_CHAR(' ')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)                             PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("KEY10")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)                             PORT_CHAR('\t')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN)   PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_LEFT) PORT_CODE(KEYCODE_LEFT)   PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Help") PORT_CODE(KEYCODE_END)           PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Copy") PORT_CODE(KEYCODE_F2)            PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)                       PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)                       PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("KEY11")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Roll Up") PORT_CODE(KEYCODE_F8)         PORT_CHAR(UCHAR_MAMEKEY(PGUP))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Roll Down") PORT_CODE(KEYCODE_F9)       PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY12")     /* port 0x0c */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY13")     /* port 0x0d */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY14")     /* port 0x0e */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY15")     /* port 0x0f */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "BASIC" ) PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(    0x01, "N88-BASIC" )
	PORT_DIPSETTING(    0x00, "N-BASIC" )
	PORT_DIPNAME( 0x02, 0x02, "Terminal mode" ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Text width" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x04, "40 chars/line" )
	PORT_DIPSETTING(    0x00, "80 chars/line" )
	PORT_DIPNAME( 0x08, 0x00, "Text height" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x08, "20 lines/screen" )
	PORT_DIPSETTING(    0x00, "25 lines/screen" )
	PORT_DIPNAME( 0x10, 0x10, "Enable S parameter" ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Enable DEL code" ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	// TODO: these really maps to "general purpose inputs" UIP1 / UIP2
	PORT_DIPNAME( 0x40, 0x40, "Memory wait" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Disable CMD SING" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Parity generate" ) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Parity type" ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x00, "Even" )
	PORT_DIPSETTING(    0x02, "Odd" )
	PORT_DIPNAME( 0x04, 0x00, "Serial character length" ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, "7 bits/char" )
	PORT_DIPSETTING(    0x00, "8 bits/char" )
	PORT_DIPNAME( 0x08, 0x08, "Stop bit length" ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, "Enable X parameter" ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Duplex" ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, "Half" )
	PORT_DIPSETTING(    0x00, "Full" )
	// TODO: vanilla PC8801 and mkII doesn't have V2
	PORT_DIPNAME( 0x40, 0x40, "BASIC speed select" ) PORT_DIPLOCATION("SW3:1") // actually SW3:0!
	PORT_DIPSETTING(    0x40, "High Speed Mode (V1H, V2)" )
	PORT_DIPSETTING(    0x00, "Standard Mode (V1S)" )
	PORT_DIPNAME( 0x80, 0x00, "BASIC Version select" ) PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(    0x80, "V1 Mode" )
	PORT_DIPSETTING(    0x00, "V2 Mode" )

	PORT_START("CTRL")
	PORT_DIPNAME( 0x02, 0x02, "Monitor Type" )
	PORT_DIPSETTING(    0x02, "15 KHz" )
	PORT_DIPSETTING(    0x00, "24 KHz" )
//  PORT_BIT 0x04 USART DCD signal carrier
	PORT_DIPNAME( 0x08, 0x00, "Auto-boot floppy at start-up" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("upd1990a", upd1990a_device, data_out_r)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,IPT_CUSTOM ) PORT_VBLANK("screen")
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	// TODO: Coming from the old legacy driver as "EXSWITCH", where this maps?
	PORT_START("CFG")
	#if 0
	PORT_DIPNAME( 0x0f, 0x08, "Serial speed" )
	PORT_DIPSETTING(    0x01, "75bps" )
	PORT_DIPSETTING(    0x02, "150bps" )
	PORT_DIPSETTING(    0x03, "300bps" )
	PORT_DIPSETTING(    0x04, "600bps" )
	PORT_DIPSETTING(    0x05, "1200bps" )
	PORT_DIPSETTING(    0x06, "2400bps" )
	PORT_DIPSETTING(    0x07, "4800bps" )
	PORT_DIPSETTING(    0x08, "9600bps" )
	PORT_DIPSETTING(    0x09, "19200bps" )
	#endif
	// TODO: unemulated waitstate weight
	PORT_DIPNAME( 0x40, 0x40, "Speed mode" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x40, DEF_STR( High ) )

	PORT_START("OPN_PA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1) PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x00)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1) PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x00)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1) PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x00)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1) PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x00)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("OPN_PB")
	// TODO: yojukiko and grobda maps Joystick buttons in reverse than expected?
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Joystick Button 1") PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x00)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Joystick Button 2") PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x00)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Mouse Button 1") PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x02)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Mouse Button 2") PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x02)
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X ) PORT_REVERSE PORT_SENSITIVITY(20) PORT_KEYDELTA(20) PORT_PLAYER(1) PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x02)

	PORT_START("MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y ) PORT_REVERSE PORT_SENSITIVITY(20) PORT_KEYDELTA(20) PORT_PLAYER(1) PORT_CONDITION("BOARD_CONFIG", 0x02, EQUALS, 0x02)

	PORT_START("MEM")
	PORT_CONFNAME( 0x0f, 0x0a, "Extension memory" )
	PORT_CONFSETTING(    0x00, DEF_STR( None ) )
	PORT_CONFSETTING(    0x01, "32KB (PC-8012-02 x 1)" )
	PORT_CONFSETTING(    0x02, "64KB (PC-8012-02 x 2)" )
	PORT_CONFSETTING(    0x03, "128KB (PC-8012-02 x 4)" )
	PORT_CONFSETTING(    0x04, "128KB (PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x05, "256KB (PC-8801-02N x 2)" )
	PORT_CONFSETTING(    0x06, "512KB (PC-8801-02N x 4)" )
	PORT_CONFSETTING(    0x07, "1M (PIO-8234H-1M x 1)" )
	PORT_CONFSETTING(    0x08, "2M (PIO-8234H-2M x 1)" )
	PORT_CONFSETTING(    0x09, "4M (PIO-8234H-2M x 2)" )
	PORT_CONFSETTING(    0x0a, "8M (PIO-8234H-2M x 4)" )
	PORT_CONFSETTING(    0x0b, "1.1M (PIO-8234H-1M x 1 + PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x0c, "2.1M (PIO-8234H-2M x 1 + PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x0d, "4.1M (PIO-8234H-2M x 2 + PC-8801-02N x 1)" )

	PORT_START("BOARD_CONFIG")
	// TODO: extend both via slot options
//  PORT_CONFNAME( 0x01, 0x01, "Sound Board" )
//  PORT_CONFSETTING(    0x00, "OPN (YM2203)" )
//  PORT_CONFSETTING(    0x01, "OPNA (YM2608)" )
	PORT_CONFNAME( 0x02, 0x00, "Port 1 Connection" )
	PORT_CONFSETTING(    0x00, "Joystick" )
	PORT_CONFSETTING(    0x02, "Mouse" )
INPUT_PORTS_END

static INPUT_PORTS_START( pc8801fh )
	PORT_INCLUDE( pc8801 )

	// TODO: KEY12, KEY13 and KEY14 have extended meaning
	// "KEY12" F6 - F10, BS, INS, DEL
	// "KEY13" kanji control (lower 4 bits)
	// "KEY14" Normal & Numpad RETURN, Left Shift, Right Shift.
	//         bit 7 acts as extension identifier (0 for FH+ keyboards).

	PORT_MODIFY("CFG")
	PORT_DIPNAME( 0x80, 0x80, "Main CPU clock" )
	PORT_DIPSETTING(    0x80, "4MHz" )
	PORT_DIPSETTING(    0x00, "8MHz" )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout char_layout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout kanji_layout =
{
	16, 16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP16(0,1) },
	{ STEP16(0,16) },
	16*16
};

// debugging only
static GFXDECODE_START( gfx_pc8801 )
	GFXDECODE_ENTRY( "cgrom",     0, char_layout,  0, 8 )
	GFXDECODE_ENTRY( "kanji",     0, kanji_layout, 0, 8 )
	GFXDECODE_ENTRY( "kanji_lv2", 0, kanji_layout, 0, 8 )
GFXDECODE_END

void pc8801_state::machine_start()
{
	m_rtc->cs_w(1);
	m_rtc->oe_w(1);

	m_work_ram = make_unique_clear<uint8_t[]>(0x10000);
	m_hi_work_ram = make_unique_clear<uint8_t[]>(0x1000);
	m_ext_work_ram = make_unique_clear<uint8_t[]>(0x8000*0x100);
	m_gvram = make_unique_clear<uint8_t[]>(0xc000);

	save_pointer(NAME(m_work_ram), 0x10000);
	save_pointer(NAME(m_hi_work_ram), 0x1000);
	save_pointer(NAME(m_ext_work_ram), 0x8000*0x100);
	save_pointer(NAME(m_gvram), 0xc000);
}

void pc8801_state::machine_reset()
{
	#define kB 1024
	#define MB 1024*1024
	const uint32_t extram_type[] = { 0*kB, 32*kB,64*kB,128*kB,128*kB,256*kB,512*kB,1*MB,2*MB,4*MB,8*MB,1*MB+128*kB,2*MB+128*kB,4*MB+128*kB, 0*kB, 0*kB };
	#undef kB
	#undef MB

	m_ext_rom_bank = 0xff;
	m_gfx_ctrl = 0x31;
	m_window_offset_bank = 0x80;
	m_misc_ctrl = 0x80;
	m_layer_mask = 0x00;
	m_vram_sel = 3;

//  dynamic_res_change();

	m_mouse.phase = 0;

	{
		m_txt_color = 2;
	}

	{
		int i;

		for(i = 0; i < 3; i++)
			m_alu_reg[i] = 0x00;
	}

	{
		m_crtc.param_count = 0;
		m_crtc.cmd = 0;
		m_crtc.status = 0;
	}

	m_beeper->set_state(0);

	// initialize I8214 (no way to set these from SW side?)
	m_pic->etlg_w(1);
	m_pic->inte_w(1);

	// initialize irq section
	{
		m_timer_irq_enable = false;
		m_timer_irq_pending = false;
		m_vrtc_irq_enable = false;
		m_rxrdy_irq_enable = false;
		m_sound_irq_enable = false;
		m_sound_irq_pending = false;
	}

	{
		m_dma_address[2] = 0xf300;
	}

	{
		m_extram_bank = 0;
		m_extram_mode = 0;
	}

	palette_reset();

	m_extram_size = extram_type[ioport("MEM")->read() & 0x0f];
//  m_has_opna = ioport("BOARD_CONFIG")->read() & 1;
}

void pc8801fh_state::machine_reset()
{
	pc8801_state::machine_reset();

	m_clock_setting = ioport("CFG")->read() & 0x80;

	m_maincpu->set_unscaled_clock(m_clock_setting ? (PC8801FH_OSC3 / 8) : (PC8801FH_OSC3 / 4));
	// TODO: FDC board shouldn't be connected to the clock setting, verify
//  m_fdccpu->set_unscaled_clock(m_clock_setting ?  XTAL(4'000'000) : XTAL(8'000'000));
	m_baudrate_val = 0;
}

void pc8801ma_state::machine_reset()
{
	pc8801fh_state::machine_reset();

	m_dic_bank = 0;
	m_dic_ctrl = 0;
}

void pc8801mc_state::machine_reset()
{
	pc8801ma_state::machine_reset();

	// Hold STOP during boot to bypass CDROM BIOS at POST (PC=0x10)
	m_cdrom_bank = true;
}

// TODO: to joyport option slot
uint8_t pc8801mk2sr_state::opn_porta_r()
{
	if(ioport("BOARD_CONFIG")->read() & 2)
	{
		uint8_t shift, res;

		shift = (m_mouse.phase & 1) ? 0 : 4;
		res = (m_mouse.phase & 2) ? m_mouse.ly : m_mouse.lx;

//      logerror("%d\n",m_mouse.phase);

		return ((res >> shift) & 0x0f) | 0xf0;
	}

	return ioport("OPN_PA")->read();
}

/* Cassette Configuration */
WRITE_LINE_MEMBER( pc8801_state::txdata_callback )
{
	//m_cassette->output( (state) ? 1.0 : -1.0);
}

WRITE_LINE_MEMBER( pc8801_state::rxrdy_irq_w )
{
	// TODO: verify if mechanism is correct
	if (m_rxrdy_irq_enable)
		m_pic->r_w(7 ^ 0, !state);
}

/*
 * 0 RXRDY
 * 1 VRTC
 * 2 CLOCK
 * 3 INT3 (GSX-8800)
 * 4 INT4 (any OPN, external boards included with different mask at $aa)
 * 5 INT5
 * 6 FDCINT1
 * 7 FDCINT2
 *
 */
IRQ_CALLBACK_MEMBER(pc8801_state::int_ack_cb)
{
	// TODO: schematics sports a μPB8212 too, with DI2-DI4 connected to 8214 A0-A2
	// Seems just an intermediate bridge for translating raw levels to vectors
	// with no access from outside world?
	u8 level = m_pic->a_r();
//  printf("%d\n", level);
	m_pic->r_w(level, 1);
	m_maincpu->set_input_line(0, CLEAR_LINE);

	return (7 - level) * 2;
}

WRITE_LINE_MEMBER(pc8801_state::int4_irq_w)
{
	bool irq_state = m_sound_irq_enable & state;

	m_pic->r_w(7 ^ 4, !irq_state);
	// remember current setting so that an enable reg variation will pick up
	// particularly needed by Telenet games (xzr2, valis2)
	m_sound_irq_pending = state;
}

// FIXME: convert following two to pure WRITE_LINE_MEMBERs
TIMER_DEVICE_CALLBACK_MEMBER(pc8801_state::clock_irq_w)
{
	// TODO: castlex sound notes in BGM loop are pretty erratic
	// (uses clock irq instead of the dedicated INT4, started happening on last OPN rewrite)
	if (m_timer_irq_enable)
		m_pic->r_w(7 ^ 2, 0);
	else
		m_timer_irq_pending = true;
}

INTERRUPT_GEN_MEMBER(pc8801_state::vrtc_irq_w)
{
	if (m_vrtc_irq_enable)
		m_pic->r_w(7 ^ 1, 0);
}

WRITE_LINE_MEMBER(pc8801_state::irq_w)
{
	if (state)
		m_maincpu->set_input_line(0, ASSERT_LINE);
}


void pc8801_state::pc8801(machine_config &config)
{
	/* main CPU */
	Z80(config, m_maincpu, MASTER_CLOCK);        /* 4 MHz */
	m_maincpu->set_addrmap(AS_PROGRAM, &pc8801_state::main_map);
	m_maincpu->set_addrmap(AS_IO, &pc8801_state::main_io);
	m_maincpu->set_vblank_int("screen", FUNC(pc8801_state::vrtc_irq_w));
	m_maincpu->set_irq_acknowledge_callback(FUNC(pc8801_state::int_ack_cb));

	PC80S31(config, m_pc80s31, MASTER_CLOCK);
	config.set_perfect_quantum(m_maincpu);
	config.set_perfect_quantum("pc80s31:fdc_cpu");

//  config.set_maximum_quantum(attotime::from_hz(MASTER_CLOCK/1024));

	I8214(config, m_pic, MASTER_CLOCK);
	m_pic->int_wr_callback().set(FUNC(pc8801_state::irq_w));

	UPD1990A(config, m_rtc);
	//CENTRONICS(config, "centronics", centronics_devices, "printer");

	// TODO: needs T88 format support
	CASSETTE(config, m_cassette);
	m_cassette->set_default_state(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED);
	m_cassette->set_interface("pc8801_cass");

	SOFTWARE_LIST(config, "tape_list").set_original("pc8801_cass");

	// TODO: clock, receiver handler, DCD?
	I8251(config, m_usart, 0);
	m_usart->txd_handler().set(FUNC(pc8801_state::txdata_callback));
	m_usart->rxrdy_handler().set(FUNC(pc8801_state::rxrdy_irq_w));

	SOFTWARE_LIST(config, "disk_n88_list").set_original("pc8801_flop");
	SOFTWARE_LIST(config, "disk_n_list").set_original("pc8001_flop");

	/* video hardware */
	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_raw(PIXEL_CLOCK_24KHz,848,0,640,448,0,400);
	m_screen->set_screen_update(FUNC(pc8801_state::screen_update));
	m_screen->set_palette(m_palette);

	GFXDECODE(config, "gfxdecode", m_palette, gfx_pc8801);
	PALETTE(config, m_palette, palette_device::BLACK, 0x10);

	TIMER(config, "rtc_timer").configure_periodic(FUNC(pc8801_state::clock_irq_w), attotime::from_hz(600));

	// Note: original models up to OPNA variants really have an internal mono speaker,
	// but user eventually can have a stereo mixing audio card mounted so for simplicity we MCM here.
	SPEAKER(config, m_lspeaker).front_left();
	SPEAKER(config, m_rspeaker).front_right();

	// TODO: DAC_1BIT
	// 2400 Hz according to schematics, unaffected by clock speed setting (confirmed on real HW)
	BEEP(config, m_beeper, MASTER_CLOCK / 16 / 13 / 8);

	for (auto &speaker : { m_lspeaker, m_rspeaker })
	{
		m_cassette->add_route(ALL_OUTPUTS, speaker, 0.025);
		m_beeper->add_route(ALL_OUTPUTS, speaker, 0.10);
	}
}

void pc8801mk2sr_state::pc8801mk2sr(machine_config &config)
{
	pc8801(config);

	YM2203(config, m_opn, MASTER_CLOCK);
	m_opn->irq_handler().set(FUNC(pc8801mk2sr_state::int4_irq_w));
	m_opn->port_a_read_callback().set(FUNC(pc8801mk2sr_state::opn_porta_r));
	m_opn->port_b_read_callback().set_ioport("OPN_PB");

	for (auto &speaker : { m_lspeaker, m_rspeaker })
	{
		// TODO: per-channel mixing is unconfirmed
		m_opn->add_route(0, speaker, 0.125);
		m_opn->add_route(1, speaker, 0.125);
		m_opn->add_route(2, speaker, 0.125);
		m_opn->add_route(3, speaker, 0.125);
	}
}

void pc8801mk2sr_state::pc8801mk2mr(machine_config &config)
{
	pc8801mk2sr(config);
	PC80S31K(config.replace(), m_pc80s31, MASTER_CLOCK);
}

void pc8801fh_state::pc8801fh(machine_config &config)
{
	pc8801mk2mr(config);

	config.device_remove("opn");

	YM2608(config, m_opna, MASTER_CLOCK*2);
	m_opna->set_addrmap(0, &pc8801fh_state::opna_map);
	m_opna->irq_handler().set(FUNC(pc8801fh_state::int4_irq_w));
	m_opna->port_a_read_callback().set(FUNC(pc8801fh_state::opn_porta_r));
	m_opna->port_b_read_callback().set_ioport("OPN_PB");
	// TODO: per-channel mixing is unconfirmed
	m_opna->add_route(0, m_lspeaker, 0.25);
	m_opna->add_route(0, m_rspeaker, 0.25);
	m_opna->add_route(1, m_lspeaker, 0.75);
	m_opna->add_route(2, m_rspeaker, 0.75);

	// TODO: add possible configuration override for baudrate here
	// ...
}

void pc8801ma_state::pc8801ma(machine_config &config)
{
	pc8801fh(config);
	// TODO: option slot for CD-ROM bus
	// ...
}

void pc8801mc_state::pc8801mc(machine_config &config)
{
	pc8801ma(config);

	PC8801_31(config, m_cdrom_if, 0);
	m_cdrom_if->rom_bank_cb().set([this](bool state) { m_cdrom_bank = state; });
}

ROM_START( pc8801 )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.2
	ROM_LOAD( "n80.rom",   0x0000, 0x8000, CRC(5cb8b584) SHA1(063609dd518c124a4fc9ba35d1bae35771666a34) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 1.0 V1
	ROM_LOAD( "n88.rom",   0x0000, 0x8000, CRC(ffd68be0) SHA1(3518193b8207bdebf22c1380c2db8c554baff329) )
	ROM_LOAD( "n88_0.rom", 0x8000, 0x2000, CRC(61984bab) SHA1(d1ae642aed4f0584eeb81ff50180db694e5101d4) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD_OPTIONAL( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(56653188) SHA1(84b90f69671d4b72e8f219e1fe7cd667e976cf7f) )
ROM_END

/*
 * The dump only included "maincpu".
 * Other roms arbitrariely taken from PC-8801 & PC-8801 MkIISR
 * (there should be at least 1 Kanji ROM).
 */
ROM_START( pc8801mk2 )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.4
	ROM_LOAD( "m2_n80.rom",   0x0000, 0x8000, CRC(91d84b1a) SHA1(d8a1abb0df75936b3fc9d226ccdb664a9070ffb1) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 1.3 V1
	ROM_LOAD( "m2_n88.rom",   0x0000, 0x8000, CRC(f35169eb) SHA1(ef1f067f819781d9fb2713836d195866f0f81501) )
	ROM_LOAD( "m2_n88_0.rom", 0x8000, 0x2000, CRC(5eb7a8d0) SHA1(95a70af83b0637a5a0f05e31fb0452bb2cb68055) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD_OPTIONAL( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x800 )
ROM_END

ROM_START( pc8801mk2sr )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.5
	ROM_LOAD( "mk2sr_n80.rom",   0x0000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.0 V2 / 1.4 V1
	ROM_LOAD( "mk2sr_n88.rom",   0x0000, 0x8000, CRC(a0fc0473) SHA1(3b31fc68fa7f47b21c1a1cb027b86b9e87afbfff) )
	ROM_LOAD( "mk2sr_n88_0.rom", 0x8000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "n88_1.rom",       0xa000, 0x2000, CRC(c0bd2aa6) SHA1(8528eef7946edf6501a6ccb1f416b60c64efac7c) )
	ROM_LOAD( "n88_2.rom",       0xc000, 0x2000, CRC(af2b6efa) SHA1(b7c8bcea219b77d9cc3ee0efafe343cc307425d1) )
	ROM_LOAD( "n88_3.rom",       0xe000, 0x2000, CRC(7713c519) SHA1(efce0b51cab9f0da6cf68507757f1245a2867a72) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )
	// not on stock mkIISR
	ROM_LOAD_OPTIONAL( "kanji2.rom", 0x00000, 0x20000, CRC(154803cc) SHA1(7e6591cd465cbb35d6d3446c5a83b46d30fafe95) )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x800 )
ROM_END

ROM_START( pc8801mk2fr )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.5
	ROM_LOAD( "m2fr_n80.rom",   0x0000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.1 V2 / 1.5 V1
	ROM_LOAD( "m2fr_n88.rom",   0x0000, 0x8000, CRC(b9daf1aa) SHA1(696a480232bcf8c827c7aeea8329db5c44420d2a) )
	ROM_LOAD( "m2fr_n88_0.rom", 0x8000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "m2fr_n88_1.rom", 0xa000, 0x2000, CRC(e3e78a37) SHA1(85ecd287fe72b56e54c8b01ea7492ca4a69a7470) )
	ROM_LOAD( "m2fr_n88_2.rom", 0xc000, 0x2000, CRC(98c3a7b2) SHA1(fc4980762d3caa56964d0ae583424756f511d186) )
	ROM_LOAD( "m2fr_n88_3.rom", 0xe000, 0x2000, CRC(0ca08abd) SHA1(a5a42d0b7caa84c3bc6e337c9f37874d82f9c14b) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x800 )
ROM_END

ROM_START( pc8801mk2mr )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.8
	ROM_LOAD( "m2mr_n80.rom",   0x0000, 0x8000, CRC(f074b515) SHA1(ebe9cf4cf57f1602c887f609a728267f8d953dce) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.2 V2 / 1.7 V1
	ROM_LOAD( "m2mr_n88.rom",   0x0000, 0x8000, CRC(69caa38e) SHA1(3c64090237152ee77c76e04d6f36bad7297bea93) )
	ROM_LOAD( "m2mr_n88_0.rom", 0x8000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "m2mr_n88_1.rom", 0xa000, 0x2000, CRC(e3e78a37) SHA1(85ecd287fe72b56e54c8b01ea7492ca4a69a7470) )
	ROM_LOAD( "m2mr_n88_2.rom", 0xc000, 0x2000, CRC(11176e0b) SHA1(f13f14f3d62df61498a23f7eb624e1a646caea45) )
	ROM_LOAD( "m2mr_n88_3.rom", 0xe000, 0x2000, CRC(0ca08abd) SHA1(a5a42d0b7caa84c3bc6e337c9f37874d82f9c14b) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom",      0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )
	ROM_LOAD( "m2mr_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x800 )
ROM_END

ROM_START( pc8801mh )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.8, but different BIOS code?
	ROM_LOAD( "mh_n80.rom",   0x0000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.3 V2 / 1.8 V1
	ROM_LOAD( "mh_n88.rom",   0x0000, 0x8000, CRC(64c5d162) SHA1(3e0aac76fb5d7edc99df26fa9f365fd991742a5d) )
	ROM_LOAD( "mh_n88_0.rom", 0x8000, 0x2000, CRC(deb384fb) SHA1(5f38cafa8aab16338038c82267800446fd082e79) )
	ROM_LOAD( "mh_n88_1.rom", 0xa000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "mh_n88_2.rom", 0xc000, 0x2000, CRC(6aa6b6d8) SHA1(2a077ab444a4fd1470cafb06fd3a0f45420c39cc) )
	ROM_LOAD( "mh_n88_3.rom", 0xe000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom",    0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )
	ROM_LOAD( "mh_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x0800 )
ROM_END

ROM_START( pc8801fa )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.8
	ROM_LOAD( "fa_n80.rom",   0x0000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.3 V2 / 1.9 V1
	ROM_LOAD( "fa_n88.rom",   0x0000, 0x8000, CRC(73573432) SHA1(9b1346d44044eeea921c4cce69b5dc49dbc0b7e9) )
	ROM_LOAD( "fa_n88_0.rom", 0x8000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "fa_n88_1.rom", 0xa000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "fa_n88_2.rom", 0xc000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "fa_n88_3.rom", 0xe000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom",    0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )
	ROM_LOAD( "fa_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x0800 )
ROM_END

// newer floppy BIOS and Jisyo (dictionary) ROM, otherwise same as FA
ROM_START( pc8801ma )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.8
	ROM_LOAD( "ma_n80.rom",   0x0000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.3 V2 / 1.9 V1
	ROM_LOAD( "ma_n88.rom",   0x0000, 0x8000, CRC(73573432) SHA1(9b1346d44044eeea921c4cce69b5dc49dbc0b7e9) )
	ROM_LOAD( "ma_n88_0.rom", 0x8000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "ma_n88_1.rom", 0xa000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "ma_n88_2.rom", 0xc000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "ma_n88_3.rom", 0xe000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom",    0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )
	ROM_LOAD( "ma_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x0800 )

	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "ma_jisyo.rom", 0x00000, 0x80000, CRC(a6108f4d) SHA1(3665db538598abb45d9dfe636423e6728a812b12) )
ROM_END

ROM_START( pc8801ma2 )
	ROM_REGION( 0x8000,  "n80rom", ROMREGION_ERASEFF ) // 1.8
	ROM_LOAD( "ma2_n80.rom",   0x0000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.3 (2.31?) V2 / 1.91 V1
	ROM_LOAD( "ma2_n88.rom",   0x0000, 0x8000, CRC(ae1a6ebc) SHA1(e53d628638f663099234e07837ffb1b0f86d480d) )
	ROM_LOAD( "ma2_n88_0.rom", 0x8000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "ma2_n88_1.rom", 0xa000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "ma2_n88_2.rom", 0xc000, 0x2000, CRC(1d6277b6) SHA1(dd9c3e50169b75bb707ef648f20d352e6a8bcfe4) )
	ROM_LOAD( "ma2_n88_3.rom", 0xe000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom",     0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )
	ROM_LOAD( "ma2_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x0800 )

	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "ma2_jisyo.rom", 0x00000, 0x80000, CRC(856459af) SHA1(06241085fc1d62d4b2968ad9cdbdadc1e7d7990a) )
ROM_END

ROM_START( pc8801mc )
	ROM_REGION( 0x08000, "n80rom", ROMREGION_ERASEFF ) // 1.8
	ROM_LOAD( "mc_n80.rom",   0x0000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )

	ROM_REGION( 0x10000, "n88rom", ROMREGION_ERASEFF ) // 2.3 (2.33?) V2 / 1.93 V1
	ROM_LOAD( "mc_n88.rom",   0x0000, 0x8000, CRC(356d5719) SHA1(5d9ba80d593a5119f52aae1ccd61a1457b4a89a1) )
	ROM_LOAD( "mc_n88_0.rom", 0x8000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "mc_n88_1.rom", 0xa000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "mc_n88_2.rom", 0xc000, 0x2000, CRC(1d6277b6) SHA1(dd9c3e50169b75bb707ef648f20d352e6a8bcfe4) )
	ROM_LOAD( "mc_n88_3.rom", 0xe000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "cdrom_bios", 0 )
	ROM_LOAD( "cdbios.rom", 0x0000, 0x10000, CRC(5c230221) SHA1(6394a8a23f44ea35fcfc3e974cf940bc8f84d62a) )

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji1.rom",    0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji_lv2", ROMREGION_ERASEFF )
	ROM_LOAD( "mc_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	ROM_REGION( 0x800, "cgrom", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x0800 )

	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "mc_jisyo.rom", 0x00000, 0x80000, CRC(bd6eb062) SHA1(deef0cc2a9734ba891a6d6c022aa70ffc66f783e) )
ROM_END

/* System Drivers */

/*    YEAR  NAME         PARENT  COMPAT  MACHINE      INPUT   CLASS         INIT        COMPANY  FULLNAME */

COMP( 1981, pc8801,      0,      0,      pc8801,      pc8801, pc8801_state, empty_init,      "NEC",   "PC-8801",       MACHINE_NOT_WORKING )
COMP( 1983, pc8801mk2,   pc8801, 0,      pc8801,      pc8801, pc8801_state, empty_init,      "NEC",   "PC-8801mkII",   MACHINE_NOT_WORKING )

// internal OPN
COMP( 1985, pc8801mk2sr, pc8801, 0,      pc8801mk2sr, pc8801, pc8801mk2sr_state, empty_init, "NEC",   "PC-8801mkIISR", MACHINE_NOT_WORKING )
//COMP( 1985, pc8801mk2tr, pc8801, 0,      pc8801mk2sr, pc8801, pc8801mk2sr_state, empty_init, "NEC",   "PC-8801mkIITR", MACHINE_NOT_WORKING )
COMP( 1985, pc8801mk2fr, pc8801, 0,      pc8801mk2sr, pc8801, pc8801mk2sr_state, empty_init, "NEC",   "PC-8801mkIIFR", MACHINE_NOT_WORKING )
COMP( 1985, pc8801mk2mr, pc8801, 0,      pc8801mk2mr, pc8801, pc8801mk2sr_state, empty_init, "NEC",   "PC-8801mkIIMR", MACHINE_NOT_WORKING )

// internal OPNA
//COMP( 1986, pc8801fh,    0,      0,      pc8801mk2fr,      pc8801fh, pc8801fh_state, empty_init, "NEC",   "PC-8801FH",     MACHINE_NOT_WORKING )
COMP( 1986, pc8801mh,    pc8801, 0,      pc8801fh,    pc8801fh, pc8801fh_state, empty_init, "NEC",   "PC-8801MH",     MACHINE_NOT_WORKING )
COMP( 1987, pc8801fa,    pc8801, 0,      pc8801fh,    pc8801fh, pc8801fh_state, empty_init, "NEC",   "PC-8801FA",     MACHINE_NOT_WORKING )
COMP( 1987, pc8801ma,    pc8801, 0,      pc8801ma,    pc8801fh, pc8801ma_state, empty_init, "NEC",   "PC-8801MA",     MACHINE_NOT_WORKING )
//COMP( 1988, pc8801fe,    pc8801, 0,      pc8801fa,      pc8801fh, pc8801ma_state, empty_init, "NEC",   "PC-8801FE",     MACHINE_NOT_WORKING )
COMP( 1988, pc8801ma2,   pc8801, 0,      pc8801ma,    pc8801fh, pc8801ma_state, empty_init, "NEC",   "PC-8801MA2",    MACHINE_NOT_WORKING )
//COMP( 1989, pc8801fe2,   pc8801, 0,      pc8801fa,      pc8801fh, pc8801ma_state, empty_init, "NEC",   "PC-8801FE2",    MACHINE_NOT_WORKING )
COMP( 1989, pc8801mc,    pc8801, 0,      pc8801mc,    pc8801fh, pc8801mc_state, empty_init, "NEC",   "PC-8801MC",     MACHINE_NOT_WORKING )

// PC98DO (PC88+PC98, V33 + μPD70008AC)
//COMP( 1989, pc98do,      0,      0,      pc98do,      pc98do, pc8801_state, empty_init, "NEC",   "PC-98DO",       MACHINE_NOT_WORKING )
//COMP( 1990, pc98dop,     0,      0,      pc98do,      pc98do, pc8801_state, empty_init, "NEC",   "PC-98DO+",      MACHINE_NOT_WORKING )
