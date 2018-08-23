/*
 * rave-eeprom-dump.c
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
#include <malloc.h>
#include <fs.h>
#include <errno.h>
#include <linux/ctype.h>
#include "rave-eeprom.h"
#include "rave-eeprom-dds.h"
#include "rave-eeprom-main.h"

#define MIN(x,y) (x < y ? x : y)

static int run_dump_dds(void)
{
	uint8_t page[RAVE_EEPROM_DDS_PAGE_SIZE];
	int ret = 1;
	int fd;

	fd = rave_eeprom_dds_open(O_RDONLY);
	if (fd < 0) {
		pr_err("unable to open dds eeprom: %s\n", strerror(errno));
		return -errno;
	}

	printf("DDS EEPROM INFO:\n");

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_ASSEMBLY_PARTNUM, page)) != 0)
		goto out;
	printf ("  part number: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_ASSEMBLY_SERIALNUM, page)) != 0)
		goto out;
	printf ("  serial number: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_ASSEMBLY_DOM, page)) != 0)
		goto out;
	printf ("  date of manufacture: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_PARTNUM, page)) != 0)
		goto out;
	printf ("  board part number: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_BOARD_REV, page)) != 0)
		goto out;
	printf ("  board part revision: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_SERIAL, page)) != 0)
		goto out;
	printf ("  board serial number: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_FORMAT, page)) != 0)
		goto out;
	printf ("  board date of manufacture: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[RAVE_EEPROM_DDS_PAGE_FORMAT_MANUFACTURE_OFFSET]), (char *)&page[RAVE_EEPROM_DDS_PAGE_FORMAT_MANUFACTURE_OFFSET + 1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_BOARD_REV_UPDATE, page)) != 0)
		goto out;
	printf ("  board updated revision: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_BOARD_DOM_UPDATE, page)) != 0)
		goto out;
	printf ("  board updated date of manufacture: %.*s\n", MIN(RAVE_EEPROM_DDS_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS1, page)) != 0)
		goto out;

	if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET] < RAVE_EEPROM_DDS_SEAT_CLASS_MAX)
		printf("  seat class: %s\n",
			   rave_eeprom_dds_seat_class_to_string(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET]));
	else
		printf("  seat class: unknown (0x%02x)\n", page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_CLASS_OFFSET]);

	if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET] < RAVE_EEPROM_DDS_DISPLAY_TYPE_MAX)
		printf("  display type: %s\n",
			   rave_eeprom_dds_display_type_to_string(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET]));
	else
		printf("  display type: unknown (0x%02x)\n", page[RAVE_EEPROM_DDS_PAGE_FLAGS1_DISPLAY_TYPE_OFFSET]);

	if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET] < RAVE_EEPROM_DDS_LOCATION_TYPE_MAX)
		printf("  location type: %s\n",
			   rave_eeprom_dds_location_type_to_string(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET]));
	else
		printf("  location type: unknown (0x%02x)\n", page[RAVE_EEPROM_DDS_PAGE_FLAGS1_LOCATION_TYPE_OFFSET]);

	if (page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET] != 0 && isalpha(page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET]))
		printf("  seat location: %u%c\n",
			   page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET],
			   page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET]);
	else
		printf("  seat location: unknown (0x%02x 0x%02x)\n",
			   page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_NUMBER_OFFSET],
			   page[RAVE_EEPROM_DDS_PAGE_FLAGS1_SEAT_LETTER_OFFSET]);

	if ((ret = rave_eeprom_dds_read_page(fd, RAVE_EEPROM_DDS_PAGE_FLAGS3, page)) != 0)
		goto out;

	printf ("  RJU USB power: %s\n", (page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] & RAVE_EEPROM_DDS_USB_POWER_RJU)  ? "yes" : "no");
	printf ("  User USB power: %s\n", (page[RAVE_EEPROM_DDS_PAGE_FLAGS3_PWR_CFG_BITS_OFFSET] & RAVE_EEPROM_DDS_USB_POWER_USER) ? "yes" : "no");

	ret = 0;

out:
	close(fd);
	return ret;
}

static int run_dump_main(void)
{
	uint8_t page[RAVE_EEPROM_MAIN_PAGE_SIZE];
	int ret = 1;
	int fd;

	fd = rave_eeprom_main_open(O_RDONLY);
	if (fd < 0) {
		pr_err("unable to open main eeprom: %s\n", strerror(errno));
		return -errno;
	}

	printf("MAIN EEPROM INFO:\n");

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_PARTNUM, page)) != 0)
		goto out;
	printf ("  part number: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_SERIAL, page)) != 0)
		goto out;
	printf ("  serial number: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_FORMAT, page)) != 0)
		goto out;
	printf ("  date of manufacture: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[RAVE_EEPROM_MAIN_PAGE_FORMAT_MANUFACTURE_OFFSET]), (char *)&page[RAVE_EEPROM_MAIN_PAGE_FORMAT_MANUFACTURE_OFFSET + 1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_BOARD_PARTNUM, page)) != 0)
		goto out;
	printf ("  board part number: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_BOARD_REV, page)) != 0)
		goto out;
	printf ("  board part revision: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_BOARD_SERIAL, page)) != 0)
		goto out;
	printf ("  board serial number: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_BOARD_DOM, page)) != 0)
		goto out;
	printf ("  board date of manufacture: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_BOARD_REV_UPDATE, page)) != 0)
		goto out;
	printf ("  board updated revision: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	if ((ret = rave_eeprom_main_read_page(fd, RAVE_EEPROM_MAIN_PAGE_BOARD_DOM_UPDATE, page)) != 0)
		goto out;
	printf ("  board updated date of manufacture: %.*s\n", MIN(RAVE_EEPROM_MAIN_PAGE_SIZE - 2, page[0]), (char *)&page[1]);

	ret = 0;

out:
	close(fd);
	return ret;
}

int rave_eeprom_dump(int dump_main, int dump_dds)
{
	int ret;

	if (!dump_dds && !dump_main) {
		dump_dds = 1;
		dump_main = 1;
	}

	if (dump_main && ((ret = run_dump_main()) != 0)) {
		printf("failed dumping main EEPROM contents\n");
		return ret;
	}

	if (dump_dds && ((ret = run_dump_dds()) != 0)) {
		printf("failed dumping dds EEPROM contents\n");
		return ret;
	}

	return 0;
}
