#ifndef __2D_PRIMITIVES__
#define __2D_PRIMITIVES__

#include <fb.h>
#include <linux/font.h>

void gu_draw_line(struct screen *sc,
		  int x1, int y1, int x2, int y2,
		  u8 r, u8 g, u8 b, u8 a,
		  unsigned int dash);

void gu_draw_circle(struct screen *sc,
		    int x0, int y0, int radius,
		    u8 r, u8 g, u8 b, u8 a);

#ifdef CONFIG_FONTS
void gu_draw_text(struct screen *sc, const struct font_desc *font,
                 int x, int y, const char *text,
                 u8 fr, u8 fg, u8 fb, u8 fa,
                 u8 br, u8 bg, u8 bb, u8 ba);
#else
static inline void gu_draw_text(struct screen *sc, const struct font_desc *font,
				int x, int y, const char *text,
				u8 fr, u8 fg, u8 fb, u8 fa,
				u8 br, u8 bg, u8 bb, u8 ba) {}
#endif
#endif
