#include <common.h>
#include <fb.h>
#include <gui/graphic_utils.h>
#include <linux/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <fs.h>
#include <malloc.h>
#include <linux/font.h>
#include <linux/ctype.h>

static void __illuminate(struct fb_info *info,
			 int x, int y,
			 u8 r, u8 g, u8 b, u8 a)
{
	void *pixel;

	if (x < 0 || y < 0 || x >= info->xres || y >= info->yres)
		return;

	pixel  = fb_get_screen_base(info);
	pixel += y * info->line_length + x * (info->bits_per_pixel >> 3);

	gu_set_rgba_pixel(info, pixel, r, g, b, a);
}

static void illuminate(struct fb_info *info,
		       bool invert,
		       int x, int y,
		       u8 r, u8 g, u8 b, u8 a)
{
	if (invert)
		__illuminate(info, y, x,
			     r, g, b, a);
	else
		__illuminate(info, x, y,
			     r, g, b, a);

}


static void draw_simple_line(struct screen *sc,
			     int x1, int y1,
			     int x2, int y2,
			     u8 r, u8 g, u8 b, u8 a,
			     unsigned int dash)
{
	int x;
	bool invert = false;
	unsigned int pixel = 0;

	BUG_ON(x1 != x2 &&
	       y1 != y2);

	if (x1 == x2) {
		swap(x1, y1);
		swap(x2, y2);
		invert = true;
	}

	if (x1 > x2) {
		swap(x1, x2);
		swap(y1, y2);
	}

	for (x = x1; x <= x2; x++) {
		if (!dash ||
		    (++pixel % (2 * dash)) < dash)
			illuminate(sc->info,
				   invert,
				   x, y1,
				   r, g, b, a);
	}
}

/**
 * gu_draw_line - draw a 2D dashed line between (x1, y1) and (x2,y2)
 *
 * @sc: screen to draw on
 * @x1, @y1: first point defining the line
 * @x2, @y2: second point defining the line
 * @r, @g, @b, @a: line's color
 * @dash: dash length (0 denotes solid line)
 *
 * gu_draw_line() implements integer version of Bresenham's algoritm
 * as can be found here:
 *
 * http://www.idav.ucdavis.edu/education/GraphicsNotes/Bresenhams-Algorithm.pdf
 */
void gu_draw_line(struct screen *sc,
		  int x1, int y1,
		  int x2, int y2,
		  u8 r, u8 g, u8 b, u8 a,
		  unsigned int dash)
{
	int dx;
	int dy;
	int i, j, eps;
	bool invert = false;
	unsigned int pixel = 0;

	BUG_ON(x1 < 0 || y1 < 0 ||
	       x2 < 0 || y2 < 0);

	if (x1 == x2 || y1 == y2) {
		draw_simple_line(sc,
				 x1, y1,
				 x2, y2,
				 r, g, b, a, dash);
		return;
	}

	dx = abs(x2 - x1);
	dy = abs(y2 - y1);

	/*
	 * First thing we need to determine "Driving Axis", as can be
	 * seen below if Y-axis projection of the line is bigger than
	 * X-axis projection we swap axes and pretend the X is Y and
	 * vice versa
	 */
	if (dy > dx) {
		swap(x1, y1);
		swap(x2, y2);
		swap(dx, dy);
		invert = true;
	}

	/*
	 * Second, we need to make sure that we will be traversing
	 * driving axis in the direction of increment so we swap point
	 * 1 with point 2 if x1 is greater than x2
	 */
	if (x1 > x2) {
		swap(x1, x2);
		swap(y1, y2);
	}

	j   = y1;
	eps = dy - dx;

	for (i = x1; i <= x2; i++) {
		if (!dash ||
		    (++pixel % (2 * dash)) > dash) {
			illuminate(sc->info,
				   invert,
				   j, i,
				   r, g, b, a);
		} else {
			printf("NOT illuminating pixel: %d\n", pixel);
		}

		if (eps >= 0) {
			j += 1;
			eps -= dx;
		}

		eps += dy;
	}
}

/**
 * gu_draw_circle - draw a 2D circle with center at (x0, y0)
 *
 * @sc: screen to draw on
 * @x0, @y0: coordinates of circle's center
 * @radius: circle's radius
 * @r, @g, @b, @a: circle's color

 *
 * gu_draw_circle() implements midpoint circle algorithm as can be
 * found here:
 *
 * https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 */
void gu_draw_circle(struct screen *sc,
		    int x0, int y0, int radius,
		    u8 r, u8 g, u8 b, u8 a)
{
	int x = radius;
	int y = 0;
	int e = 0;

	BUG_ON(x0 < 0 || y0 < 0 || radius < 0);

	while (x >= y) {
		 __illuminate(sc->info, x0 + x, y0 + y, r, g, b, a);
		 __illuminate(sc->info, x0 + y, y0 + x, r, g, b, a);
		 __illuminate(sc->info, x0 - y, y0 + x, r, g, b, a);
		 __illuminate(sc->info, x0 - x, y0 + y, r, g, b, a);
		 __illuminate(sc->info, x0 - x, y0 - y, r, g, b, a);
		 __illuminate(sc->info, x0 - y, y0 - x, r, g, b, a);
		 __illuminate(sc->info, x0 + y, y0 - x, r, g, b, a);
		 __illuminate(sc->info, x0 + x, y0 - y, r, g, b, a);

		 y += 1;
		 e += 1 + 2 * y;

		 if (2 * (e - x) + 1 > 0) {
			 x -= 1;
			 e += 1 - 2 * x;
		 }
	}
}

#ifdef CONFIG_FONTS
static void gu_draw_char(struct screen *sc, const struct font_desc *font,
			 int ch, int x0, int y0,
			 u8 fr, u8 fg, u8 fb, u8 fa,
			 u8 br, u8 bg, u8 bb, u8 ba)
{
       const u8 *bitmap = font->data + find_font_index(font, ch);
       int x, y;

       for (y = 0; y < font->height; y++) {
               for (x = 0; x < font->width; x++) {
		       const unsigned index  = y * font->width + x;
		       const unsigned byte   = index / 8;
		       const unsigned bit    = 8 - (index % 8);
		       const bool foreground = bitmap[byte] & BIT(bit);

		       const u8 r = (foreground) ? fr : br;
		       const u8 g = (foreground) ? fg : bg;
		       const u8 b = (foreground) ? fb : bb;
		       const u8 a = (foreground) ? fa : ba;

		       __illuminate(sc->info, x0 + x, y0 + y, r, g, b, a);
               }
       }
}

/**
 * gu_draw_text - display text at (x0, y0)
 *
 * @sc: screen to draw on
 * @font: font used to render the text
 * @x0, @y0: coordinates of top left corner of the text "box"
 * @text: text to render
 * @fr, @fg, @fb, @fa: foreground color information
 * @fr, @fg, @fb, @fa: background color information
 *
 * gu_draw_text() implements basic algorithm capable of rendering
 * textual information to a frambuffer. Resulting text is produced to
 * be a single line and no attempt to do wrapping/text layout is made.
 */
void gu_draw_text(struct screen *sc, const struct font_desc *font,
		  int x0, int y0, const char *text,
		  u8 fr, u8 fg, u8 fb, u8 fa,
		  u8 br, u8 bg, u8 bb, u8 ba)
{
	char c;
	int x = x0;
	int y = y0;

	BUG_ON(x0 < 0 || y0 < 0);

	while ((c = *text++)) {
		/*
		 * Just skip all non-printable characters
		 */
		if (isprint(c)) {
			gu_draw_char(sc, font, c, x, y,
				     fr, fg, fb, fa, br, bg, bb, ba);
			x += font->width;
		}
	}
}
#endif

