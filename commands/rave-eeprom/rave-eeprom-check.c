/*
 * rave-eeprom-check.c
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
#include "rave-eeprom-common.h"
#include "rave-eeprom-dds.h"
#include "rave-eeprom-main.h"

#define FIELD_NAME_LENGTH 52

static void read_and_check_page(int fd, unsigned int page_number)
{
	uint8_t page[RAVE_EEPROM_COMMON_PAGE_SIZE];
	uint16_t expected_crc;
	uint16_t read_crc;

	if (rave_eeprom_common_read_page(fd, page_number, page) != 0) {
		printf ("error reading page\n");
		return;
	}

	if (!rave_eeprom_common_page_check_initialized(page)) {
		printf ("page not initialized\n");
		return;
	}

	read_crc = rave_eeprom_common_page_crc_read(page);
	expected_crc = rave_eeprom_common_page_crc_compute(page);
	if (read_crc != expected_crc) {
		printf ("invalid crc: expected 0x%04x, read 0x%04x\n", expected_crc, read_crc);
		return;
	}

	printf ("passed\n");
}

static void dds_foreach_page(rave_eeprom_dds_page_t page_number, void *user_data)
{
	char buffer[FIELD_NAME_LENGTH + 1];
	int fd;

	fd = *((int *)user_data);

	snprintf(buffer, sizeof(buffer), "  dds eeprom page %d (%s):", page_number, rave_eeprom_dds_page_to_string(page_number));
	printf ("%-*.*s", FIELD_NAME_LENGTH, FIELD_NAME_LENGTH, buffer);

	read_and_check_page(fd, page_number);
}

static int run_check_dds(void)
{
	int fd;

	fd = rave_eeprom_common_open(RAVE_EEPROM_DDS_PATH, O_RDONLY);
	if (fd < 0) {
		pr_err("unable to open dds eeprom: %s\n", strerror(errno));
		return -errno;
	}

	rave_eeprom_dds_page_foreach(dds_foreach_page, (void *)&fd);

	close(fd);
	return 0;
}

static void main_foreach_page(rave_eeprom_main_page_t page_number, void *user_data)
{
	char buffer[FIELD_NAME_LENGTH + 1];
	int fd;

	fd = *((int *)user_data);

	snprintf(buffer, sizeof(buffer), "  main eeprom page %d (%s):", page_number, rave_eeprom_main_page_to_string(page_number));
	printf ("%-*.*s", FIELD_NAME_LENGTH, FIELD_NAME_LENGTH, buffer);

	read_and_check_page(fd, page_number);
}

static int run_check_main(void)
{
	int fd;

	fd = rave_eeprom_common_open(RAVE_EEPROM_MAIN_PATH, O_RDONLY);
	if (fd < 0) {
		pr_err("unable to open main eeprom: %s\n", strerror(errno));
		return -errno;
	}

	rave_eeprom_main_page_foreach(main_foreach_page, (void *)&fd);

	close(fd);
	return 0;
}

int rave_eeprom_check(int check_main, int check_dds)
{
	int ret;

	if (!check_dds && !check_main) {
		check_dds = 1;
		check_main = 1;
	}

	if (check_main && ((ret = run_check_main()) != 0)) {
		pr_err("failed checking main eeprom page crc values\n");
		return ret;
	}

	if (check_dds && ((ret = run_check_dds()) != 0)) {
		pr_err("failed checking dds eeprom page crc values\n");
		return ret;
	}

	return 0;
}
