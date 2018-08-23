/*
 * rave-eeprom-dds.c
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
#include "rave-eeprom-dds.h"

static const char *dds_eeprom_path = "/dev/dds-eeprom";

static const char *dds_seat_class_str[RAVE_EEPROM_DDS_SEAT_CLASS_MAX] = {
	[RAVE_EEPROM_DDS_SEAT_CLASS_UNSET]             = "unset",
	[RAVE_EEPROM_DDS_SEAT_CLASS_ECONOMY]           = "economy",
	[RAVE_EEPROM_DDS_SEAT_CLASS_ECONOMY_PREMIERE]  = "economy-premiere",
	[RAVE_EEPROM_DDS_SEAT_CLASS_BUSINESS]          = "business",
	[RAVE_EEPROM_DDS_SEAT_CLASS_BUSINESS_PREMIERE] = "business-premiere",
	[RAVE_EEPROM_DDS_SEAT_CLASS_FIRST]             = "first",
	[RAVE_EEPROM_DDS_SEAT_CLASS_FLIGHT_CREW_REST]  = "flight-crew-rest",
	[RAVE_EEPROM_DDS_SEAT_CLASS_CABIN_CREW_REST]   = "cabin-crew-rest",
};

const char *rave_eeprom_dds_seat_class_to_string(rave_eeprom_dds_seat_class_t type)
{
	return ((type > RAVE_EEPROM_DDS_SEAT_CLASS_UNKNOWN) && (type < RAVE_EEPROM_DDS_SEAT_CLASS_MAX)) ? dds_seat_class_str[type] : NULL;
}

rave_eeprom_dds_seat_class_t
rave_eeprom_dds_seat_class_from_string(const char *str)
{
	int i;

	for (i = RAVE_EEPROM_DDS_SEAT_CLASS_UNSET; i < RAVE_EEPROM_DDS_SEAT_CLASS_MAX; i++) {
		if (strcmp(str, dds_seat_class_str[i]) == 0)
			return (rave_eeprom_dds_seat_class_t)i;
	}
	return RAVE_EEPROM_DDS_SEAT_CLASS_UNKNOWN;
}

static const char *dds_display_type_str[RAVE_EEPROM_DDS_DISPLAY_TYPE_MAX] = {
	[RAVE_EEPROM_DDS_DISPLAY_TYPE_UNSET]      = "unset",
	[RAVE_EEPROM_DDS_DISPLAY_TYPE_SEAT]       = "seat",
	[RAVE_EEPROM_DDS_DISPLAY_TYPE_CREW_PANEL] = "crew-panel",
	[RAVE_EEPROM_DDS_DISPLAY_TYPE_BCO]        = "broadcast",
	[RAVE_EEPROM_DDS_DISPLAY_TYPE_HOTSPARE]   = "hot-spare",
};

const char *rave_eeprom_dds_display_type_to_string(rave_eeprom_dds_display_type_t type)
{
	return ((type > RAVE_EEPROM_DDS_DISPLAY_TYPE_UNKNOWN) && (type < RAVE_EEPROM_DDS_DISPLAY_TYPE_MAX)) ? dds_display_type_str[type] : NULL;
}

rave_eeprom_dds_display_type_t
rave_eeprom_dds_display_type_from_string(const char *str)
{
	int i;

	for (i = RAVE_EEPROM_DDS_DISPLAY_TYPE_UNSET; i < RAVE_EEPROM_DDS_DISPLAY_TYPE_MAX; i++) {
		if (strcmp(str, dds_display_type_str[i]) == 0)
			return (rave_eeprom_dds_display_type_t)i;
	}
	return RAVE_EEPROM_DDS_DISPLAY_TYPE_UNKNOWN;
}

static const char *dds_location_type_str[RAVE_EEPROM_DDS_LOCATION_TYPE_MAX] = {
	[RAVE_EEPROM_DDS_LOCATION_TYPE_UNSET]            = "unset",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_STANDARD_SEAT]    = "standard-seat",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_CREW_REST]        = "crew-rest",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_BULKHEAD]         = "bulkhead",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_OVERHEAD]         = "overhead",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_LASTROW]          = "last-row",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_MISC]             = "misc",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_SCU]              = "scu",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_DISABLED]         = "disabled",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_NIU]              = "niu",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_LIU]              = "liu",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_ODB]              = "odb",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_MAINTENANCE_PORT] = "maintenance-port",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_CFU]              = "cfu",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_SPU]              = "spu",
	[RAVE_EEPROM_DDS_LOCATION_TYPE_SPB]              = "spb",
};

const char *rave_eeprom_dds_location_type_to_string(rave_eeprom_dds_location_type_t type)
{
	return ((type > RAVE_EEPROM_DDS_LOCATION_TYPE_UNKNOWN) && (type < RAVE_EEPROM_DDS_LOCATION_TYPE_MAX)) ? dds_location_type_str[type] : NULL;
}

rave_eeprom_dds_location_type_t
rave_eeprom_dds_location_type_from_string(const char *str)
{
	int i;

	for (i = RAVE_EEPROM_DDS_LOCATION_TYPE_UNSET; i < RAVE_EEPROM_DDS_LOCATION_TYPE_MAX; i++) {
		if (strcmp(str, dds_location_type_str[i]) == 0)
			return (rave_eeprom_dds_location_type_t)i;
	}
	return RAVE_EEPROM_DDS_LOCATION_TYPE_UNKNOWN;
}

static uint16_t dds_page_crc(const uint8_t *page)
{
	uint8_t x;
	uint16_t crc = 0xFFFF;
	int length = RAVE_EEPROM_DDS_PAGE_SIZE - 2;

	while (length--) {
		x = crc >> 8 ^ *page++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
	}

	return crc;
}

static bool dds_page_validate_crc(const uint8_t *page)
{
	return (dds_page_crc (page) == ((page[0x1e] << 8) | page[0x1f]));
}

static void dds_page_update_crc(uint8_t *page)
{
	uint16_t crc;

	crc = dds_page_crc(page);
	page[0x1f] = crc & 0xff;
	page[0x1e] = (crc >> 8) & 0xff;
}

int rave_eeprom_dds_read_page(int fd, rave_eeprom_dds_page_t page_number, uint8_t *page)
{
	int r;
	loff_t seek, offset;

	offset = page_number * RAVE_EEPROM_DDS_PAGE_SIZE;
	seek = lseek(fd, offset, SEEK_SET);
	if (seek != offset) {
		pr_err("unable to seek to dds page %d: %s\n", page_number, strerror(errno));
		return -errno;
	}

	r = read(fd, page, RAVE_EEPROM_DDS_PAGE_SIZE);
	if (r < 0) {
		pr_err("unable to read dds page %d: %s\n", page_number, strerror(errno));
		return -errno;
	}

	if (r < RAVE_EEPROM_DDS_PAGE_SIZE) {
		pr_err("unable to read full dds page %d: %u/%u bytes read\n", page_number, r, RAVE_EEPROM_DDS_PAGE_SIZE);
		return -EFAULT;
	}

	if (!dds_page_validate_crc(page)) {
		pr_err("invalid crc detected in page %u\n", page_number);
		return -EFAULT;
	}

	return 0;
}

int rave_eeprom_dds_write_page(int fd, rave_eeprom_dds_page_t page_number, uint8_t *page)
{
	int w;
	loff_t seek, offset;

	dds_page_update_crc(page);

	offset = page_number * RAVE_EEPROM_DDS_PAGE_SIZE;
	seek = lseek(fd, offset, SEEK_SET);
	if (seek != offset) {
		pr_err("unable to seek to dds page %d: %s\n", page_number, strerror(errno));
		return -errno;
	}

	w = write(fd, page, RAVE_EEPROM_DDS_PAGE_SIZE);
	if (w < 0) {
		pr_err("unable to write dds page %d: %s\n", page_number, strerror(errno));
		return -errno;
	}

	if (w < RAVE_EEPROM_DDS_PAGE_SIZE) {
		pr_err("unable to write full dds page %d: %u/%u bytes written\n", page_number, w, RAVE_EEPROM_DDS_PAGE_SIZE);
		return -EFAULT;
	}

	return 0;
}

int rave_eeprom_dds_open(int mode)
{
	int fd;

	fd = open(dds_eeprom_path, O_RWSIZE_1 | mode);
	if (fd < 0) {
		pr_err("unable to open dds eeprom: %s\n", strerror(errno));
		return -errno;
	}

	return fd;
}
