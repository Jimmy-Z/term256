#pragma once

#include <nds.h>
#include "font.h"

// written for NDS, should not be hard to port back to GBA

#define TERM_COLS (SCREEN_WIDTH / FONT_WIDTH)
#define TERM_ROWS (SCREEN_HEIGHT / FONT_HEIGHT)
#define TERM_MAX_CHARS (TERM_COLS * TERM_ROWS)

void generate_ansi256_palette(u16 *p);

typedef void(*set_scroll_cb_t)(int x, int y, void *cb_param);

typedef struct {
	u16 *fb;
	unsigned cur;
	u8 cur_fg;
	u8 cur_bg;
	u16 clut[FONT_WIDTH / sizeof(u16) * (1 << FONT_WIDTH)];
	set_scroll_cb_t set_scroll_cb;
	void *set_scroll_cb_param;
}term_t;

enum {
	TERM_COLOR,
	TERM_BG_COLOR,
	TERM_MOVE_X,
	TERM_MOVE_Y,
};

void clr_screen(void *fb, u8 color);

void term_init(term_t *t, u16 *fb, set_scroll_cb_t cb, void *cb_param);

void term_rst(term_t *t, u8 fg, u8 bg);

void term_prt(term_t *t, const char *string);

void term_ctl(term_t *t, int ctl_code, int ctl_param);

void term_raw(term_t *t, char c);
