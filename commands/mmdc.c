#include <common.h>
#include <command.h>
#include <getopt.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <rtc.h>
#include <linux/rtc.h>
#include <string.h>
#include <environment.h>

#include <mach/imx6-mmdc.h>

static int do_mmdc(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "t:p")) > 0) {
		switch (opt) {
		case 'f':
			if (!strcmp(optarg, "write-level")) {
				return mmdc_do_write_level_calibration();
			} else if (!strcmp(optarg, "dqs")) {
				return mmdc_do_dqs_calibration();
			} else {
				return 1;
			}
			break;
		case 'p':
			mmdc_print_calibration_results();
			break;
		}
	}

	return 0;
}

BAREBOX_CMD_HELP_START(mmdc)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-t NAME\t\t\t", "RTC device name (default rtc0)")
BAREBOX_CMD_HELP_OPT ("-p\t\t\t", "print the results of MMDC calibration")

BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mmdc)
	.cmd		= do_mmdc,
	BAREBOX_CMD_DESC("query or set the hardware clock (RTC)")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_mmdc_help)
BAREBOX_CMD_END
