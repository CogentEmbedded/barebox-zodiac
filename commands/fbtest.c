#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fb.h>
#include <gui/graphic_utils.h>

#define TP_HOR_LINES	12
#define TP_VERT_LINES	17
#define DEF_COLOR	0x00FFFFFF	/* white */

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static const struct {
	unsigned char 	*name;
	u32		code;
} color_names[] = {
	{"white",	0x00FFFFFF},
	{"yellow",	0x00FFFF00},
	{"cyan",	0x00000FFF},
	{"green",	0x00008000},
	{"magenta",	0x00FF00FF},
	{"red",		0x00FF0000},
	{"blue",	0x000000FF},
	{"black",	0x00000000},
};

u32 get_color(unsigned char *color)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(color_names); i++)
		if (strcmp(color, color_names[i].name) == 0)
			return color_names[i].code;

	return DEF_COLOR;
}

/* move to ... */
unsigned long int_sqrt(unsigned long x);

static void draw_fill(struct screen *sc, void *buf, u32 color)
{
	gu_memset_pixel(sc->info, buf, gu_hex_to_pixel(sc->info, color),
			sc->s.width * sc->s.height);
}

static void draw_color_bars(struct screen *sc, void *buf, u32 __color)
{
	int i;
	u32 color;

	for (i = 0; i < ARRAY_SIZE(color_names); i++) {
		int x1, x2;
		u8 r, g, b;

		x1 = sc->info->xres * i / ARRAY_SIZE(color_names);
		x2 = sc->info->xres * (i + 1) / ARRAY_SIZE(color_names) - 1;
		color = color_names[i].code;
		r = (color >> 16) & 0xff;
		g = (color >> 8) & 0xff;
		b = (color >> 0) & 0xff;

		gu_fill_frame(sc->info, buf,
				x1, 0, x2, sc->info->yres - 1,
				r, g, b, 0xff);
	}
}

static void draw_test_pattern(struct screen *sc, void *buf, u32 color)
{
	int i;
	int R;
	long long tmp;
	u8 r, g, b;

	r = (color >> 16) & 0xff;
	g = (color >> 8) & 0xff;
	b = (color >> 0) & 0xff;

	/* clear screen with inverted color*/
	gu_memset_pixel(sc->info, buf, ~color,
			sc->s.width * sc->s.height);

	/* these two whould be dotted */
	gu_draw_line_dotted(sc->info, buf,
			0, 0,
			sc->info->xres - 1, 0,
			r, g, b, 0xff, 20);
	gu_draw_line_dotted(sc->info, buf,
			0, sc->info->yres - 1,
			sc->info->xres - 1, sc->info->yres - 1,
			r, g, b, 0xff, 20);
	/* horisontal solid lines */
	for (i = 1; i < TP_HOR_LINES; i++) {
		int y;
		y = (sc->info->yres - 1) * i / TP_HOR_LINES;
		gu_draw_line(sc->info, buf, 0, y, sc->info->xres - 1, y,
				r, g, b, 0xff);
	}

	/* these two whould be dotted */
	gu_draw_line_dotted(sc->info, buf,
			0, 0,
			0, sc->info->yres - 1,
			r, g, b, 0xff, 20);
	gu_draw_line_dotted(sc->info, buf,
			sc->info->xres - 1, 0,
			sc->info->xres - 1, sc->info->yres - 1,
			r, g, b, 0xff, 20);
	/* vertical lines */
	for (i = 1; i < TP_VERT_LINES; i++) {
		int x;
		x = (sc->info->xres - 1) * i / TP_VERT_LINES;
		gu_draw_line(sc->info, buf, x, 0, x, sc->info->yres - 1,
				r, g, b, 0xff);
	}

	/* big circle */
	R = MIN(sc->info->xres, sc->info->yres) / 2;
	gu_draw_circle(sc->info, buf,
			sc->info->xres / 2, sc->info->yres / 2, R - 1,
			r, g, b, 0xff);
	/* small circels */
	tmp = MAX(sc->info->xres, sc->info->yres) / 2;
	tmp = tmp * tmp;
	tmp = tmp + R * R;
	tmp = (int_sqrt(tmp) - R);
	/* div (1 + sqrt(2)) ~ div 2.41 ~ * 5 / 12 */
	R = (int)tmp * 5 / 12;
	/* 1 */
	gu_draw_circle(sc->info, buf,
			R, R, R - 1,
			r, g, b, 0xff);
	/* 2 */
	gu_draw_circle(sc->info, buf,
			sc->info->xres  - R, R, R - 1,
			r, g, b, 0xff);
	/* 3 */
	gu_draw_circle(sc->info, buf,
			sc->info->xres - R, sc->info->yres - R, R - 1,
			r, g, b, 0xff);
	/* 4 */
	gu_draw_circle(sc->info, buf,
			R, sc->info->yres - R, R - 1,
			r, g, b, 0xff);
}



static void draw_test_lvds(struct screen *sc, void *buf, u32 __color)
{
	u32 color = 0x00000001;
	u8 r, g, b;
	int x;
	int y = 0;

	/* need update */
	while (y + 32 < sc->info->yres) {
		x = 0;
		while (x + 32 < sc->info->xres) {
			r = (color >> 16) & 0xff;
			g = (color >> 8) & 0xff;
			b = (color >> 0) & 0xff;

			gu_fill_frame(sc->info, buf,
					x, y, x + 31, y + 31,
					r, g, b, 0xff);

			/* runing bit */
			color = color << 1 | color >> 22;
			color &= 0x00FFFFFF;

			x += 32;
		}
		y += 32;
	}
}

static int do_test_pattern(int argc, char *argv[])
{
	struct surface s;
	struct screen *sc;
	int ret = 0;
	int opt;
	char *fbdev = "/dev/fb0";
	u32 color = DEF_COLOR;
	bool do_fill = false;
	bool do_bars = false;
	bool do_test = false;
	bool do_lvds = false;
	void *buf;

	memset(&s, 0, sizeof(s));

	s.x = -1;
	s.y = -1;
	s.width = -1;
	s.height = -1;

	while((opt = getopt(argc, argv, "f:c:Fbgl")) > 0) {
		switch(opt) {
		case 'f':
			fbdev = optarg;
			break;
		case 'c':
			color = get_color(optarg);
			break;
		case 'F':
			do_fill = true;
			break;
		case 'b':
			do_bars = true;
			break;
		case 'g':
			do_test = true;
			break;
		case 'l':
			do_lvds = true;

		}
	}

	if (!do_fill && !do_bars && !do_test && !do_lvds)
		return -EINVAL;

	sc = fb_open(fbdev);
	if (IS_ERR(sc)) {
		perror("fd_open");
		return PTR_ERR(sc);
	}

	buf = gui_screen_render_buffer(sc);

	if (do_fill)
		draw_fill(sc, buf, color);
	else if (do_bars)
		draw_color_bars(sc, buf, color);
	else if (do_test)
		draw_test_pattern(sc, buf, color);
	else if (do_lvds)
		draw_test_lvds(sc, buf, color);

	gu_screen_blit(sc);

	fb_close(sc);

	return ret;
}

BAREBOX_CMD_HELP_START(test_pattern)
BAREBOX_CMD_HELP_TEXT("This command displays a test pattern on fb")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-f FB\t",    "framebuffer device (default /dev/fb0)")
BAREBOX_CMD_HELP_OPT ("-c color\t", "color")
BAREBOX_CMD_HELP_OPT ("-F", "fill full screen")
BAREBOX_CMD_HELP_OPT ("-b", "displays color bars")
BAREBOX_CMD_HELP_OPT ("-g", "display test picture")
BAREBOX_CMD_HELP_OPT ("-l", "LVDS test pattern")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(test_pattern)
	.cmd		= do_test_pattern,
	BAREBOX_CMD_DESC("display a test pattern, VLDS pattern or fill screen")
	BAREBOX_CMD_OPTS("[-fcFbgl] FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_test_pattern_help)
BAREBOX_CMD_END