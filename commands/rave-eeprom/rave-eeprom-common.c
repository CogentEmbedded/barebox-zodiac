/*
 * rave-eeprom-common.h
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <malloc.h>
#include <fs.h>
#include <errno.h>

#include "rave-eeprom-common.h"

uint16_t rave_eeprom_common_page_crc_read(const uint8_t *page)
{
	return (uint16_t)((page[RAVE_EEPROM_COMMON_PAGE_SIZE - 2] << 8) | page[RAVE_EEPROM_COMMON_PAGE_SIZE - 1]);
}

uint16_t rave_eeprom_common_page_crc_compute(const uint8_t *page)
{
	uint8_t x;
	uint16_t crc = 0xFFFF;
	int length = RAVE_EEPROM_COMMON_PAGE_SIZE - 2;

	while (length--) {
		x = crc >> 8 ^ *page++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
	}

	return crc;
}

bool rave_eeprom_common_page_crc_validate(const uint8_t *page)
{
	return (rave_eeprom_common_page_crc_compute(page) == rave_eeprom_common_page_crc_read(page));
}

void rave_eeprom_common_page_crc_update(uint8_t *page)
{
	uint16_t crc;

	crc = rave_eeprom_common_page_crc_compute(page);
	page[RAVE_EEPROM_COMMON_PAGE_SIZE - 1] = crc & 0xff;
	page[RAVE_EEPROM_COMMON_PAGE_SIZE - 2] = (crc >> 8) & 0xff;
}

bool rave_eeprom_common_page_check_initialized(const uint8_t *page)
{
	int i;

	for (i = 0; i < RAVE_EEPROM_COMMON_PAGE_SIZE; i++) {
		if (page[i] != 0xff)
			return true;
	}
	return false;
}

int rave_eeprom_common_open(const char *path, int mode)
{
	int fd;

	fd = open(path, O_RWSIZE_1 | mode);
	if (fd < 0) {
		pr_err("unable to open eeprom at %s: %s\n", path, strerror(errno));
		return -errno;
	}

	return fd;
}

int rave_eeprom_common_read_page(int fd, unsigned int page_number, uint8_t *page)
{
	int r;
	loff_t seek, offset;

	offset = page_number * RAVE_EEPROM_COMMON_PAGE_SIZE;
	seek = lseek(fd, offset, SEEK_SET);
	if (seek != offset) {
		pr_err("unable to seek to page %u: %s\n", page_number, strerror(errno));
		return -errno;
	}

	r = read(fd, page, RAVE_EEPROM_COMMON_PAGE_SIZE);
	if (r < 0) {
		pr_err("unable to read page %u: %s\n", page_number, strerror(errno));
		return -errno;
	}

	if (r < RAVE_EEPROM_COMMON_PAGE_SIZE) {
		pr_err("unable to read full page %u: %u/%u bytes read\n", page_number, r, RAVE_EEPROM_COMMON_PAGE_SIZE);
		return -EFAULT;
	}

	return 0;
}

int rave_eeprom_common_write_page(int fd, unsigned int page_number, uint8_t *page)
{
	int w;
	loff_t seek, offset;

	offset = page_number * RAVE_EEPROM_COMMON_PAGE_SIZE;
	seek = lseek(fd, offset, SEEK_SET);
	if (seek != offset) {
		pr_err("unable to seek to page %u: %s\n", page_number, strerror(errno));
		return -errno;
	}

	w = write(fd, page, RAVE_EEPROM_COMMON_PAGE_SIZE);
	if (w < 0) {
		pr_err("unable to write common page %u: %s\n", page_number, strerror(errno));
		return -errno;
	}

	if (w < RAVE_EEPROM_COMMON_PAGE_SIZE) {
		pr_err("unable to write full common page %u: %u/%u bytes written\n", page_number, w, RAVE_EEPROM_COMMON_PAGE_SIZE);
		return -EFAULT;
	}

	return 0;
}
