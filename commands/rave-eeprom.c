/*
 * meminfo.c - show information about memory usage
 *
 * Copyright (c) 2018 Zodiac Inflight Innovations.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <command.h>
#include <getopt.h>
#include <malloc.h>

#include "rave-eeprom/rave-eeprom.h"

static int do_rave_eeprom(int argc, char *argv[])
{
	int opt;
	int ret;
	int dump = 0;
	int check_crcs = 0;
	int select_main = 0;
	int select_dds = 0;
	int dump_ignore_crc_errors = 0;
	char *seat_class = NULL;
	char *display_type = NULL;
	char *location_type = NULL;
	char *seat_location = NULL;
	char *rju_usb_power = NULL;
	char *user_usb_power = NULL;
	int nactions = 0;

	while ((opt = getopt(argc, argv, "dCIs:c:t:p:l:r:u:")) > 0) {
		switch (opt) {
		case 'd':
			dump = 1;
			break;
		case 'C':
			check_crcs = 1;
			break;
		case 'I':
			dump_ignore_crc_errors = 1;
			break;
		case 's':
			if (strcmp(optarg,"dds") == 0)
				select_dds = 1;
			else if (strcmp(optarg,"main") == 0)
				select_main = 1;
			else {
				pr_err("invalid eeprom type specified: %s\n", optarg);
				return 1;
			}
			break;
		case 'c':
			if (seat_class) {
				pr_err("seat class given multiple times\n");
				return 1;
			}
			seat_class = strdup(optarg);
			break;
		case 't':
			if (display_type) {
				pr_err("seat class given multiple times\n");
				return 1;
			}
			display_type = strdup(optarg);
			break;
		case 'p':
			if (location_type) {
				pr_err("location type given multiple times\n");
				return 1;
			}
			location_type = strdup(optarg);
			break;
		case 'l':
			if (seat_location) {
				pr_err("seat location given multiple times\n");
				return 1;
			}
			seat_location = strdup(optarg);
			break;
		case 'r':
			if (rju_usb_power) {
				pr_err("RJU USB power given multiple times\n");
				return 1;
			}
			rju_usb_power = strdup(optarg);
			break;
		case 'u':
			if (user_usb_power) {
				pr_err("user USB power given multiple times\n");
				return 1;
			}
			user_usb_power = strdup(optarg);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!dump && !check_crcs && (select_dds || select_main)) {
		printf("no dump or check action given for eeprom type\n");
		return COMMAND_ERROR_USAGE;
	}

	if (!dump && dump_ignore_crc_errors) {
		printf("ignore CRC errors may only be given along with a dump action\n");
		return COMMAND_ERROR_USAGE;
	}

	nactions = (dump +
		    check_crcs +
	            !!seat_class +
	            !!display_type +
	            !!location_type +
	            !!seat_location +
	            !!rju_usb_power +
	            !!user_usb_power);

	if (nactions == 0) {
		printf("no action specified\n");
		return COMMAND_ERROR_USAGE;
	}

	if (seat_class || display_type || location_type || seat_location) {
		ret = rave_eeprom_update_dds_page_4(seat_class, display_type, location_type, seat_location);
		if (ret != 0)
			goto out;
	}

	if (rju_usb_power || user_usb_power) {
		ret = rave_eeprom_update_dds_page_6(rju_usb_power, user_usb_power);
		if (ret != 0)
			goto out;
	}

	if (check_crcs)
		ret = rave_eeprom_check (select_main, select_dds);

	if (dump)
		ret = rave_eeprom_dump (select_main, select_dds, dump_ignore_crc_errors);
out:

	free(seat_class);
	free(display_type);
	free(location_type);
	free(seat_location);
	free(rju_usb_power);
	free(user_usb_power);

	return ret;
}

BAREBOX_CMD_HELP_START(rave_eeprom)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-d\t\t", "dump contents in human readable format")
BAREBOX_CMD_HELP_OPT("-I\t\t", "ignore CRC errors when dumping contents")
BAREBOX_CMD_HELP_OPT("-C\t\t", "check PIC EEPROM page CRCs (doesn't fix errors)")
BAREBOX_CMD_HELP_OPT("-s EEPROM\t", "select specific PIC EEPROM (dds or main) when dumping contents or checking CRCs")
BAREBOX_CMD_HELP_OPT("-c SEAT-CLASS\t", "set seat class (unset, economy, economy-premiere, business, ...)")
BAREBOX_CMD_HELP_OPT("-t DISPLAY-TYPE\t", "set display type (unset, seat, crew-panel, broadcast, hot-spare)")
BAREBOX_CMD_HELP_OPT("-p LOCATION-TYPE", "set location type (unset, standard-seat, crew-rest, ...)")
BAREBOX_CMD_HELP_OPT("-l SEAT-LOCATION", "set seat location (e.g. 1A)")
BAREBOX_CMD_HELP_OPT("-r POWER\t", "set RJU USB power state (off or on)")
BAREBOX_CMD_HELP_OPT("-u POWER\t", "set User USB power state (off or on)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(rave_eeprom)
	.cmd = do_rave_eeprom,
	BAREBOX_CMD_DESC("access RAVE PIC EEPROM contents in human-readable format")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_rave_eeprom_help)
BAREBOX_CMD_END
