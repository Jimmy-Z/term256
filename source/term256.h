#pragma once

#include <nds.h>
#include "font.h"

#define TERM_WIDTH (SCREEN_WIDTH / FONT_WIDTH)
#define TERM_HEIGHT (SCREEN_HEIGHT / FONT_HEIGHT)
#define TERM_BUF_LEN (TERM_WIDTH * TERM_HEIGHT)

typedef u16 color_t;

void generate_ansi256_palette(color_t *p);

typedef struct {
	u16 *fb;
	u8 c[TERM_BUF_LEN];
	u8 fg[TERM_BUF_LEN];
	u8 bg[TERM_BUF_LEN];
	unsigned cur;
	u8 cur_fg;
	u8 cur_bg;
	unsigned update_region_a;
	unsigned update_region_b;
	int needs_full_update;
}term_t;

enum {
	TERM_COLOR,
	TERM_BG_COLOR,
	TERM_MOVE_X,
	TERM_MOVE_Y,
};

void term_init(term_t *t, u16 *fb);

void term_prt(term_t * t, const char *string);

void term_ctl(term_t *t, int ctl_code, int ctl_param);
