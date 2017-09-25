#include <nds.h>
#include <stdarg.h>
#include <stdio.h>

#include "term256.h"
#include "term256ext.h"

// show ANSI color palette, 8 rows
void show_ansi256_color_table(u16 *fb, unsigned width, unsigned height) {
	// don't care about speed, 1 pixel at a time
	unsigned row_height = height / 8;
	// first row, 16 standard and intense colors
	unsigned block_width = width / 16;
	for (unsigned i = 0; i < 16; ++i) {
		unsigned start_x = i * block_width;
		for (unsigned y = 0; y < row_height; ++y) {
			for (unsigned x = start_x; x < start_x + block_width; ++x){
				// fb[y * width + x] = i;
				unsigned offset = (y * width + x) >> 1;
				if (x & 1) {
					fb[offset] = (fb[offset] & 0xff) | (i << 8);
				} else {
					fb[offset] = (fb[offset] & 0xff00) | i;
				}
			}
		}
	}
	// six rows, 6x6x6 RGB colors
	block_width = width / 36;
	for (unsigned i = 0; i < 6; ++i) {
		for (unsigned j = 0; j < 36; ++j) {
			unsigned c = 16 + i * 36 + j;
			unsigned start_x = j * block_width;
			unsigned start_y = (i + 1) * row_height;
			for (unsigned y = start_y; y < start_y + row_height; ++y) {
				for (unsigned x = start_x; x < start_x + block_width; ++x){
					// fb[y * width + x] = c;
					unsigned offset = (y * width + x) >> 1;
					if (x & 1) {
						fb[offset] = (fb[offset] & 0xff) | (c << 8);
					} else {
						fb[offset] = (fb[offset] & 0xff00) | c;
					}
				}
			}
		}
	}
	// final row, 24 grayscales
	block_width = width / 24;
	for (unsigned i = 0; i < 24; ++i) {
		unsigned c = 232 + i;
		unsigned start_x = i * block_width;
		for (unsigned y = row_height * 7; y < height; ++y) {
			for (unsigned x = start_x; x < start_x + block_width; ++x){
				unsigned offset = (y * width + x) >> 1;
				if (x & 1) {
					fb[offset] = (fb[offset] & 0xff) | (c << 8);
				} else {
					fb[offset] = (fb[offset] & 0xff00) | c;
				}
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

void set_scroll(int x, int y, void *param) {
	bgSetScroll(*(int*)param, x, y);
	bgUpdate();
}

char dbg_iprtf_str_buf[0x200];
void dbg_iprtf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vsniprintf(dbg_iprtf_str_buf, 0x200, fmt, args);
	va_end(args);
	term_prt(&t0, dbg_iprtf_str_buf);
}

int main(void)
{
	videoSetModeSub(MODE_3_2D);
	videoSetMode(MODE_3_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	vramSetBankA(VRAM_A_MAIN_BG);
	int bg0 = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
	int bg1 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
	u16 *fb0 = bgGetGfxPtr(bg0);
	u16 *fb1 = bgGetGfxPtr(bg1);
	generate_ansi256_palette(BG_PALETTE_SUB);
	dmaCopy(BG_PALETTE_SUB, BG_PALETTE, 256 * 2);

	term_init(&t0, fb0, set_scroll, &bg0);
	term_init(&t1, fb1, set_scroll, &bg1);
	// term_init(&t1, fb1, 0, 0);

	select_term(&t0);
	int is_DSi = isDSiMode();
	if (is_DSi) {
		prt("DSi Mode, previous CPU clock was ");
		setCpuClock(true) ? prt("high\n") : prt("low\n");
	}

	iprtf("This is an experimental 256 color terminal emulator\n"
		"\tColumns:\t%u\n\tRows:\t\t%u\n\tChars:\t\t%u\n"
		" \n"
		"press A to continue, B to quit\n",
		TERM_COLS, TERM_ROWS, TERM_MAX_CHARS);

	select_term(&t1);

	int wait = 1;
	while (true) {
		if (!wait) {
			cpuStartTiming(0);
		}
		// char map
		prt("\x1b[0m  ");
		// wait && wait_key(KEY_A);
		for (unsigned i = 0; i < 0x10; ++i) {
			iprtf(" %x", i);
		}
		prt("\n");
		for (unsigned i = 0; i < 0x100; i += 0x10) {
			iprtf("%02x", i);
			for (unsigned j = 0; j < 0x10; ++j) {
				unsigned c = i | j;
				term_raw(&t1, ' ');
				term_raw(&t1, c);
			}
			iprtf(" %02x\n", i);
		}
		prt("  ");
		for (unsigned i = 0; i < 0x10; ++i) {
			iprtf(" %x", i);
		}
		prt("\n");
		if (wait && wait_key(KEY_A | KEY_B) == KEY_B) {
			break;
		}
		// scroll test
		for (unsigned i = 1; i < 0x10; ++i) {
			if (wait) {
				swiWaitForVBlank();
				// wait_key(KEY_A);
			}
			iprtf("\t%02x\n", i);
		}
		prt("...\rcarriage return test\n");
		// color test
		for (unsigned i = 0; i < 256; ++i) {
			if (wait && !(i & 0xf)) {
				swiWaitForVBlank();
			}
			iprtf("\x1b[38;5;%dm%02x", i, i);
		}
		prt("\n");
		// font face test
		prt("\x1b[37;1m");
		prt("a quick brown fox jumps over the lazy dog,\n");
		prt("\x1b[30;48;5;15m");
		prt("a quick brown fox jumps over the lazy dog,\n");
		prt("\x1b[37;1;40m");
		prt("A QUICK BROWN FOX JUMPS OVER THE LAZY DOG.\n");
		prt("\x1b[30;48;5;15m");
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
	bgSetScroll(bg1, 0, 0);
	bgUpdate();
	show_ansi256_color_table(fb1, SCREEN_WIDTH, SCREEN_HEIGHT);
	wait_key(KEY_A | KEY_B);
	return 0;
}

