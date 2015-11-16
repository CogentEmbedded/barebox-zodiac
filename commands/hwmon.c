#include <common.h>
#include <command.h>
#include <getopt.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <string.h>
#include <environment.h>
#include <hwmon.h>

static int do_hwmon(int argc, char *argv[])
{
	struct hwmon_sensor *s;

	for_each_hwmon_sensor(s) {
		s32 value;
		int ret = hwmon_sensor_read(s, &value);

		if (ret < 0) {
			printf("%s -- failed to read the sensor (%d)\n", s->name, ret);
			continue;
		}

		switch (s->type) {
		case SENSOR_TEMPERATURE:
			printf("name: %s, reading: %d.%03d C\n",
			       s->name, (int)(value / 1000), (int)abs(value % 1000));
			break;
		default:
			printf("%s -- unknow type of sensor\n", s->name);
			break;
		}
	}

	return 0;
}

#if 0
BAREBOX_CMD_HELP_START(hwmon)
BAREBOX_CMD_HELP_END
#endif

BAREBOX_CMD_START(hwmon)
	.cmd		= do_hwmon,
	BAREBOX_CMD_DESC("query hardware sensors (HWMON)")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	/* BAREBOX_CMD_HELP(cmd_hwmon_help) */
BAREBOX_CMD_END
