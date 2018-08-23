/*
 * rave-eeprom-main.c
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

#include "rave-eeprom.h"
#include "rave-eeprom-main.h"

static const char *main_eeprom_path = "/dev/main-eeprom";

static uint16_t main_page_crc(const uint8_t *page)
{
	uint8_t x;
	uint16_t crc = 0xFFFF;
	int length = RAVE_EEPROM_MAIN_PAGE_SIZE - 2;

	while (length--) {
		x = crc >> 8 ^ *page++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
	}

	return crc;
}

static bool main_page_validate_crc(const uint8_t *page)
{
	return (main_page_crc (page) == ((page[0x1e] << 8) | page[0x1f]));
}

int rave_eeprom_main_read_page(int fd, rave_eeprom_main_page_t page_number, uint8_t *page)
{
	int r;
	loff_t seek, offset;

	offset = page_number * RAVE_EEPROM_MAIN_PAGE_SIZE;
	seek = lseek(fd, offset, SEEK_SET);
	if (seek != offset) {
		pr_err("unable to seek to main page %d: %s\n", page_number, strerror(errno));
		return -errno;
	}

	r = read(fd, page, RAVE_EEPROM_MAIN_PAGE_SIZE);
	if (r < 0) {
		pr_err("unable to read main page %d: %s\n", page_number, strerror(errno));
		return -errno;
	}

	if (r < RAVE_EEPROM_MAIN_PAGE_SIZE) {
		pr_err("unable to read full main page %d: %u/%u bytes read\n", page_number, r, RAVE_EEPROM_MAIN_PAGE_SIZE);
		return -EFAULT;
	}

	if (!main_page_validate_crc(page)) {
		pr_err("invalid crc detected in page %u\n", page_number);
		return -EFAULT;
	}

	return 0;
}

int rave_eeprom_main_open(int mode)
{
	int fd;

	fd = open(main_eeprom_path, O_RWSIZE_1 | mode);
	if (fd < 0) {
		pr_err("unable to open main eeprom: %s\n", strerror(errno));
		return -errno;
	}

	return fd;
}
