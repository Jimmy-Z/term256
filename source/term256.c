#include <nds.h>
#include <assert.h>
#include "term256.h"

#ifdef _MSC_VER
#define ITCM_CODE
#define UNROLL
#else
#define UNROLL __attribute__((optimize("unroll-loops")))
#endif

#define RGB(r,g,b) RGB15(r, g, b)
#define RBITS 5
#define RMAX (1 << RBITS)
#define GBITS 5
#define GMAX (1 << GBITS)
#define BBITS 5
#define BMAX (1 << BBITS)

// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
void generate_ansi256_palette(color_t *p) {
	// maybe I should use round to improve precision
	// standard colors
	*p++ = RGB(0, 0, 0);
	*p++ = RGB(RMAX >> 1, 0, 0);
	*p++ = RGB(0, GMAX >> 1, 0);
	*p++ = RGB(RMAX >> 1, GMAX >> 1, 0);
	*p++ = RGB(0, 0, BMAX >> 1);
	*p++ = RGB(RMAX >> 1, 0, BMAX >> 1);
	*p++ = RGB(0, GMAX >> 1, BMAX >> 1);
	*p++ = RGB((RMAX * 3) >> 2, (GMAX * 3) >> 2, (BMAX * 3) >> 2);
	// high-intensity colors
	*p++ = RGB(RMAX >> 1, GMAX >> 1, BMAX >> 1);
	*p++ = RGB(RMAX - 1, 0, 0);
	*p++ = RGB(0, GMAX - 1, 0);
	*p++ = RGB(RMAX - 1, GMAX - 1, 0);
	*p++ = RGB(0, 0, BMAX - 1);
	*p++ = RGB(RMAX - 1, 0, BMAX - 1);
	*p++ = RGB(0, GMAX - 1, BMAX - 1);
	*p++ = RGB(RMAX - 1, GMAX - 1, BMAX - 1);
	// 216 colors, 6 * 6 * 6
	unsigned rsteps[6] = { 0, (RMAX - 1) / 5, (RMAX - 1) * 2 / 5,
		(RMAX - 1) * 3 / 5, (RMAX - 1) * 4 / 5, RMAX - 1 };
	unsigned gsteps[6] = { 0, (GMAX - 1) / 5, (GMAX - 1) * 2 / 5,
		(GMAX - 1) * 3 / 5, (GMAX - 1) * 4 / 5, GMAX - 1 };
	unsigned bsteps[6] = { 0, (BMAX - 1) / 5, (BMAX - 1) * 2 / 5,
		(BMAX - 1) * 3 / 5, (BMAX - 1) * 4 / 5, BMAX - 1 };
	// how do I tell gcc not to unroll this?
	for (unsigned r = 0; r < 6; ++r) {
		for (unsigned g = 0; g < 6; ++g) {
			for (unsigned b = 0; b < 6; ++b) {
				*p++ = RGB(rsteps[r], gsteps[g], bsteps[b]);
			}
		}
	}
	// 24 gray scales, not including full white and full black, so 25 steps
	for (unsigned j = 1; j < 25; ++j) {
		*p++ = RGB((RMAX - 1) * j / 25, (GMAX - 1) * j / 25, (BMAX - 1) * j / 25);
	}
}

static_assert(FONT_WIDTH == 6, "this thing can only handle FONT_WIDTH == 6");

#ifdef ARM9
ITCM_CODE
#endif
UNROLL
static inline void write_char(u16 *fb, unsigned x, unsigned y, unsigned char c, unsigned char color, unsigned char bg_color) {
	const unsigned char *g = &font[c * FONT_HEIGHT];
	for (unsigned fy = 0; fy < FONT_HEIGHT; ++fy) {
		unsigned char l = g[fy]; // line fy of the glyph
		u16 *p = &fb[((y + fy) * SCREEN_WIDTH + x) / 2];
		*p++ = (l & 0x80 ? color : bg_color) | ((l & 0x40 ? color : bg_color) << 8);
		*p++ = (l & 0x20 ? color : bg_color) | ((l & 0x10 ? color : bg_color) << 8);
		*p = (l & 0x08 ? color : bg_color) | ((l & 0x04 ? color : bg_color) << 8);
	}
}

void term_rst(term_t *t, u8 fg, u8 bg) {
	/* so memset just fail?
	memset(t->c, 0, sizeof(TERM_BUF_LEN));
	memset(t->fg, fg, sizeof(TERM_BUF_LEN));
	memset(t->bg, bg, sizeof(TERM_BUF_LEN));
	for (unsigned i = 0; i < TERM_BUF_LEN; ++i) {
		if (t->fg[i] != fg) {
			iprintf("fg[%u] == %u\n", i, t->fg[i]);
			break;
		}
	}
	*/
	for (unsigned i = 0; i < TERM_BUF_LEN; ++i) {
		t->c[i] = 0;
		t->fg[i] = fg;
		t->bg[i] = bg;
	}
	t->cur = 0;
	t->cur_fg = fg;
	t->cur_bg = bg;
	t->update_region_a = 0;
	t->update_region_b = 0;
	t->needs_full_update = 0;
}

void term_init(term_t *t, u16 *fb) {
	t->fb = fb;
	term_rst(t, 15, 0);
}

// void wait_key(unsigned key);

#ifdef ARM9
ITCM_CODE
#endif
void term_update_fb(term_t *t) {
	if (t->needs_full_update) {
		unsigned offset = 0;
		unsigned y = 0;
		for (unsigned ty = 0; ty < TERM_HEIGHT; ++ty) {
			unsigned x = 0;
			for (unsigned tx = 0; tx < TERM_WIDTH; ++tx) {
				// wait_key(KEY_A);
				// iprintf("%u,%c,%u,%u,%u,%u,%u,%u\n", offset, t->c[offset], t->fg[offset], t->bg[offset], ty, tx, y, x);
				write_char(t->fb, x, y, t->c[offset], t->fg[offset], t->bg[offset]);
				x += FONT_WIDTH;
				++offset;
			}
			y += FONT_HEIGHT;
		}
		t->needs_full_update = 0;
		t->update_region_a = t->update_region_b = t->cur;
	} else if (t->update_region_a < t->update_region_b) {
		// TODO: partial updates
	}
}

void scroll(term_t *t) {
	unsigned i;
	u8 *p0 = t->c, *p0s = p0 + TERM_WIDTH;
	u8 *p1 = t->fg, *p1s = p1 + TERM_WIDTH;
	u8 *p2 = t->bg, *p2s = p2 + TERM_WIDTH;
	for (i = 0; i < TERM_BUF_LEN - TERM_WIDTH; ++i){
		*p0++ = *p0s++;
		*p1++ = *p1s++;
		*p2++ = *p2s++;
	}
	// fill the new line with cursor fg/bg
	for (i = 0; i < TERM_WIDTH; ++i) {
		*p0++ = 0;
		*p1++ = t->cur_fg;
		*p2++ = t->cur_bg;
	}
	t->cur -= TERM_WIDTH;
}

void term_ctl(term_t *t, int ctl_code, int ctl_param) {
	switch (ctl_code) {
	case TERM_COLOR:
		t->cur_fg = ctl_param;
		break;
	case TERM_BG_COLOR:
		t->cur_bg = ctl_param;
		break;
	case TERM_MOVE_X: {
		unsigned x_pos = t->cur % TERM_WIDTH;
		unsigned min_cur = t->cur - x_pos;
		unsigned max_cur = t->cur - x_pos + TERM_WIDTH - 1;
		t->cur += ctl_param;
		if (t->cur < min_cur) {
			t->cur = min_cur;
		} else if(t->cur > max_cur){
			t->cur = max_cur;
		}
		break;
	} case TERM_MOVE_Y: {
		unsigned min_cur = t->cur % TERM_WIDTH;
		unsigned max_cur = TERM_WIDTH * (TERM_HEIGHT - 1) + min_cur;
		t->cur += ctl_param * TERM_HEIGHT;
		if (t->cur < min_cur) {
			t->cur = min_cur;
		} else if(t->cur > max_cur){
			t->cur = max_cur;
		}
		break;
	} default:
		break;
	}
}

#define TAB_WIDTH 4

void term_prt(term_t * t, const char *s) {
	// I'm too lazy to parse ANSI escape code
	// use term_ctl instead
	unsigned char c;
	while ((c = *(const unsigned char*)s) != 0) {
		if (c == '\n') {
			unsigned tx = t->cur % TERM_WIDTH;
			if (tx != 0) {
				// do not allow empty line
				// also, new line never causes scroll
				t->cur += TERM_WIDTH - tx;
			}
		} else if (c == '\r') {
			t->cur -= t->cur % TERM_WIDTH;
		} else if (c == '\t') {
			unsigned tx = t->cur % TERM_WIDTH;
			unsigned new_tx = tx + TAB_WIDTH;
			new_tx -= new_tx % TAB_WIDTH;
			if (new_tx > TERM_WIDTH) {
				new_tx = TERM_WIDTH;
			}
			t->cur += new_tx - tx;
		} else {
			if (t->cur == TERM_BUF_LEN) {
				scroll(t);
			}
			t->c[t->cur] = c;
			t->fg[t->cur] = t->cur_fg;
			t->bg[t->cur] = t->cur_bg;
			++t->cur;
		}
		++s;
	}
	// TODO: partial updates
	t->needs_full_update = 1;
	term_update_fb(t);
}

