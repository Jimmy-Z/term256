#include <nds.h>

#include "term256.h"
#include "term256ext.h"

// show ANSI color palette, 8 rows
void show_ansi256_color_table(u16 *fb, unsigned width, unsigned height) {
	// 4 pixels at a time, there might be some overlaps, but we don't care
	width >>= 1;
	unsigned row_height = height / 8;
	// first row, 16 standard and intense colors
	unsigned block_width = width / 16;
	for (unsigned i = 0; i < 16; ++i) {
		u16 c2 = i | (i << 8);
		unsigned start_x = i * block_width;
		for (unsigned y = 0; y < row_height; ++y) {
			for (unsigned x = start_x; x < start_x + block_width; ++x){
				fb[y * width + x] = c2;
			}
		}
	}
	// six rows, 6x6x6 RGB colors
	block_width = width / 36;
	for (unsigned i = 0; i < 6; ++i) {
		for (unsigned j = 0; j < 36; ++j) {
			unsigned c = 16 + i * 36 + j;
			u16 c2 = c | (c << 8);
			unsigned start_x = j * block_width;
			unsigned start_y = (i + 1) * row_height;
			for (unsigned y = start_y; y < start_y + row_height; ++y) {
				for (unsigned x = start_x; x < start_x + block_width; ++x){
					fb[y * width + x] = c2;
				}
			}
		}
	}
	// final row, 24 grayscales
	block_width = width / 24;
	for (unsigned i = 0; i < 24; ++i) {
		unsigned c = 232 + i;
		u32 c2 = c | (c << 8);
		unsigned start_x = i * block_width;
		for (unsigned y = row_height * 7; y < height; ++y) {
			for (unsigned x = start_x; x < start_x + block_width; ++x){
				fb[y * width + x] = c2;
			}
		}
	}
}

unsigned wait_key(unsigned wanted) {
	while (1) {
		swiWaitForVBlank();
		scanKeys();
		unsigned keys = keysDown();
		if (keys & wanted) {
			return keys;
		}
	}
}

term_t t0, t1;

int main(void)
{
	videoSetModeSub(MODE_3_2D);
	videoSetMode(MODE_3_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	vramSetBankA(VRAM_A_MAIN_BG);
	u16 *fb0 = bgGetGfxPtr(bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0));
	u16 *fb1 = bgGetGfxPtr(bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0));
	generate_ansi256_palette(BG_PALETTE_SUB);
	dmaCopy(BG_PALETTE_SUB, BG_PALETTE, 256 * 2);

	term_init(&t0, fb0);
	term_init(&t1, fb1);

	select_term(&t0);
	int is_DSi = isDSiMode();
	if (is_DSi) {
		prt("DSi Mode, previous CPU clock was ");
		setCpuClock(true) ? prt("high\n") : prt("low\n");
	}

	iprtf("This is an experimental 256 color terminal emulator\n"
		"\tWidth:\t%u\n\tHeight:\t%u\n\tChars:\t%u\n"
		" \n"
		"press A to continue, B to quit\n",
		TERM_WIDTH, TERM_HEIGHT, TERM_MAX_CHARS);

	select_term(&t1);

	int wait = 1;
	while (true) {
		if (!wait) {
			cpuStartTiming(0);
		}
		term_rst(&t1, 15, 0);
		// char map
		prt("  ");
		for (unsigned i = 0; i < 0x10; ++i) {
			iprtf(" %x", i);
		}
		prt("\n");
		for (unsigned i = 0; i < 0x100; ++i) {
			if (!(i & 0xf)) {
				iprtf("%02x", i);
			}
			iprtf(" %c", i == 0 || i == '\n' || i == '\r' || i == '\t' ? ' ' : i);
			if ((i & 0xf) == 0xf) {
				prt("\n");
			}
		}
		if (wait) {
			wait_key(KEY_A);
		}
		// scroll test
		for (unsigned i = 1; i < 0x10; ++i) {
			if (wait) {
				swiWaitForVBlank();
			}
			iprtf("\t%02x\n", i);
		}
		prt("...\rcarriage return test\n");
		// color test
		for (unsigned i = 0; i < 256; ++i) {
			if (wait) {
				swiWaitForVBlank();
			}
			term_ctl(&t1, TERM_COLOR, i);
			iprtf("%02x", i);
		}
		prt("\n");
		// font face test
		term_ctl(&t1, TERM_COLOR, 15);
		prt("a quick brown fox jumps over the lazy dog,\n");
		term_ctl(&t1, TERM_COLOR, 0);
		term_ctl(&t1, TERM_BG_COLOR, 15);
		prt("a quick brown fox jumps over the lazy dog,\n");
		term_ctl(&t1, TERM_COLOR, 15);
		term_ctl(&t1, TERM_BG_COLOR, 0);
		prt("A QUICK BROWN FOX JUMPS OVER THE LAZY DOG.\n");
		term_ctl(&t1, TERM_COLOR, 0);
		term_ctl(&t1, TERM_BG_COLOR, 15);
		prt("A QUICK BROWN FOX JUMPS OVER THE LAZY DOG.\n");
		if (!wait) {
			select_term(&t0);
			iprtf("render time %lu \xe6s\n", timerTicks2usec(cpuEndTiming()));
			select_term(&t1);
		}

		if (wait_key(KEY_A | KEY_B) == KEY_B) {
			break;
		}
		wait = !wait;
	}
	clr_screen(fb1, 0);
	show_ansi256_color_table(fb1, SCREEN_WIDTH, SCREEN_HEIGHT);
	wait_key(KEY_A | KEY_B);
	return 0;
}

