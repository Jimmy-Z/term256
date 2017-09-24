#include <nds.h>
#include <assert.h>
#include "term256.h"

#define BG_WIDTH 256
#define BG_HEIGHT 256

#define TERM_WIDTH (FONT_WIDTH * TERM_COLS)
#define TERM_HEIGHT (FONT_HEIGHT * TERM_ROWS)

// default scroll x, center, border both side
#define SCROLL_X0 ((TERM_WIDTH - SCREEN_WIDTH) / 2)
// default scroll y, touch bottom to hide text below on fb, border on top
#define SCROLL_Y0 ((TERM_HEIGHT - SCREEN_HEIGHT))

// use extra bg space to do native scroll, if possible
#define FB_ROWS (BG_HEIGHT / FONT_HEIGHT)
#define HW_SCROLL_STEPS (FB_ROWS - TERM_ROWS)

// make VC happy
#ifdef _MSC_VER
#define ITCM_CODE
#define UNROLL
#else
#define UNROLL __attribute__((optimize("unroll-loops")))
#endif

#define RGB(r, g, b) RGB15(r, g, b)
#define RBITS 5
#define RMAX (1 << RBITS)
#define GBITS 5
#define GMAX (1 << GBITS)
#define BBITS 5
#define BMAX (1 << BBITS)

// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
void generate_ansi256_palette(u16 *p) {
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
static void term_gen_clut(term_t *t) {
	u16 *p = t->clut;
	for (unsigned i = 0; i < (1 << FONT_WIDTH); ++i) {
		*p++ = (i & 0x20 ? t->cur_fg : t->cur_bg) | ((i & 0x10 ? t->cur_fg : t->cur_bg) << 8);
		*p++ = (i & 0x08 ? t->cur_fg : t->cur_bg) | ((i & 0x04 ? t->cur_fg : t->cur_bg) << 8);
		*p++ = (i & 0x02 ? t->cur_fg : t->cur_bg) | ((i & 0x01 ? t->cur_fg : t->cur_bg) << 8);
	}
}

#ifdef ARM9
ITCM_CODE
#endif
UNROLL
static inline void write_char(term_t *t, unsigned x, unsigned y, unsigned char c, unsigned char color, unsigned char bg_color) {
	const unsigned char *g = font + c * FONT_HEIGHT;
	u16 *p = t->fb + (y * BG_WIDTH + x) / 2;
	for (unsigned fy = 0; fy < FONT_HEIGHT; ++fy) {
		u16 *c = t->clut + (*g++) * FONT_WIDTH / sizeof(u16);
		*p++ = *c++;
		*p++ = *c++;
		*p = *c;
		p += BG_WIDTH / 2 - 2;
	}
}

void clr_screen(void *fb, u8 color) {
	u32 *p = (u32*)fb;
	u32 *p_end = p + (BG_WIDTH * TERM_HEIGHT / sizeof(u32));
	u32 c4 = color | color << 8 | color << 16 | color << 24;
	while (p < p_end) {
		*p++ = c4; *p++ = c4; *p++ = c4; *p++ = c4;
		*p++ = c4; *p++ = c4; *p++ = c4; *p++ = c4;
	}
}

void term_rst(term_t *t, u8 fg, u8 bg) {
	t->cur = 0;
	if (fg != t->cur_fg || bg != t->cur_bg) {
		t->cur_fg = fg;
		t->cur_bg = bg;
		term_gen_clut(t);
	}
	clr_screen(t->fb, bg);
}

void term_init(term_t *t, u16 *fb, set_scroll_cb_t cb, void *cb_param) {
	t->fb = fb;
	t->set_scroll_cb = cb;
	t->set_scroll_cb_param = cb_param;
	if (cb != 0) {
		cb(SCROLL_X0, SCROLL_Y0, cb_param);
	}
	term_rst(t, 15, 0);
}

static_assert((FONT_HEIGHT * BG_WIDTH) % sizeof(u32) == 0, "this thing can't handle this");
static_assert((FONT_HEIGHT * BG_WIDTH / sizeof(u32)) % 8 == 0, "you can't unroll that much");
#ifdef ARM9
ITCM_CODE
#endif
void scroll(term_t *t) {
	u32 *p = (u32*)t->fb;
	u32 *p_src = p + (FONT_HEIGHT * BG_WIDTH / sizeof(u32));
	u32 *p_end = p + (FONT_HEIGHT * BG_WIDTH / sizeof(u32) * (TERM_ROWS - 1));
	while (p < p_end) {
		*p++ = *p_src++; *p++ = *p_src++; *p++ = *p_src++; *p++ = *p_src++;
		*p++ = *p_src++; *p++ = *p_src++; *p++ = *p_src++; *p++ = *p_src++;
	}
	while (p < p_src) {
		*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
		*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
	}
	t->cur -= TERM_COLS;
}

void term_ctl(term_t *t, int ctl_code, int ctl_param) {
	switch (ctl_code) {
	case TERM_COLOR:
		if (t->cur_fg != ctl_param) {
			t->cur_fg = ctl_param;
			term_gen_clut(t);
		}
		break;
	case TERM_BG_COLOR:
		if (t->cur_bg != ctl_param) {
			t->cur_bg = ctl_param;
			term_gen_clut(t);
		}
		break;
	case TERM_MOVE_X: {
		unsigned x_pos = t->cur % TERM_COLS;
		unsigned min_cur = t->cur - x_pos;
		unsigned max_cur = t->cur - x_pos + TERM_COLS - 1;
		if (max_cur > TERM_MAX_CHARS) {
			max_cur = TERM_MAX_CHARS;
		}
		t->cur += ctl_param;
		if (t->cur < min_cur) {
			t->cur = min_cur;
		} else if(t->cur > max_cur){
			t->cur = max_cur;
		}
		break;
	} case TERM_MOVE_Y: {
		unsigned min_cur = t->cur % TERM_COLS;
		unsigned max_cur = TERM_COLS * (TERM_ROWS - 1) + min_cur;
		t->cur += ctl_param * TERM_ROWS;
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

void term_prt(term_t *t, const char *s) {
	// TODO: parse ANSI escape code
	// currently you can use term_ctl instead
	unsigned char c;
	while ((c = *(const unsigned char*)s++) != 0) {
		if (c == '\n') {
			// tx is short for terminal x, not transmission
			unsigned tx = t->cur % TERM_COLS;
			if (tx != 0) {
				// do not allow empty line
				// also, new line never causes scroll
				t->cur += TERM_COLS - tx;
			}
		} else if (c == '\r') {
			t->cur -= t->cur % TERM_COLS;
		} else if (c == '\t') {
			unsigned tx = t->cur % TERM_COLS;
			unsigned new_tx = tx + TAB_WIDTH;
			new_tx -= new_tx % TAB_WIDTH;
			if (new_tx > TERM_COLS) {
				new_tx = TERM_WIDTH;
			}
			new_tx -= tx; // becomes d_tx now
			if (new_tx > 0) {
				if (t->cur == TERM_MAX_CHARS) {
					scroll(t);
				}
				t->cur += new_tx;
			}
		} else {
#ifdef TERM256_DEBUG
			if (t->cur == TERM_MAX_CHARS) {
#else
			if (t->cur >= TERM_MAX_CHARS) {
#endif
				scroll(t);
			}
			write_char(t,
				(t->cur % TERM_COLS) * FONT_WIDTH,
				(t->cur / TERM_COLS) * FONT_HEIGHT,
				c, t->cur_fg, t->cur_bg);
			++t->cur;
		}
	}
}

// print a raw character, even \x00 or \xff
void term_raw(term_t *t, char c) {
	if (t->cur == TERM_MAX_CHARS) {
		scroll(t);
	}
	write_char(t,
		(t->cur % TERM_COLS) * FONT_WIDTH,
		(t->cur / TERM_COLS) * FONT_HEIGHT,
		c, t->cur_fg, t->cur_bg);
	++t->cur;
}
