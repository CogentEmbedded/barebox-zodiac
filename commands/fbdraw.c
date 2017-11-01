#include <common.h>
#include <command.h>
#include <errno.h>
#include <getopt.h>
#include <gui/graphic_utils.h>
#include <gui/2d-primitives.h>

#define COLORS(c)	(c >> 16) & 0xff, \
			(c >> 8) & 0xff, \
			(c >> 0) & 0xff, \
			0xff

static int do_fb_draw_box(int argc, char *argv[])
{
	char *fbdev = "/dev/fb0";
	struct screen *sc;
	int x1, y1, x2, y2, radius;
	u32 color;

	if (argc != 6 && argc != 7)
		return COMMAND_ERROR_USAGE;

	color = simple_strtol(argv[1], NULL, 16);
	x1 = simple_strtol(argv[2], NULL, 10);
	y1 = simple_strtol(argv[3], NULL, 10);
	x2 = simple_strtol(argv[4], NULL, 10);
	y2 = simple_strtol(argv[5], NULL, 10);
	radius = argc == 7 ? simple_strtol(argv[6], NULL, 10) : 0;

	sc = fb_open(fbdev);
	if (IS_ERR(sc)) {
		perror("fd_open");
		return PTR_ERR(sc);
	}

	gu_fill_rounded_rectangle(sc, x1, x2, y1, y2, radius, COLORS(color));

	gu_screen_blit(sc);
	fb_close(sc);
	return 0;
}

BAREBOX_CMD_HELP_START(fb_draw_box)
BAREBOX_CMD_HELP_TEXT("This command draws rectangle, optionally rounded")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_OPT ("color", "color, hex rrggbb")
BAREBOX_CMD_HELP_OPT ("x1 y1", "left top corner position")
BAREBOX_CMD_HELP_OPT ("x2 y2", "right bottom corner position")
BAREBOX_CMD_HELP_OPT ("radius", "radius for rounded corners")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(fb_draw_box)
	.cmd		= do_fb_draw_box,
	BAREBOX_CMD_DESC("draw colored rectangle")
	BAREBOX_CMD_OPTS("color x1 y2 x1 y2 [radius]")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_fb_draw_box_help)
BAREBOX_CMD_END
