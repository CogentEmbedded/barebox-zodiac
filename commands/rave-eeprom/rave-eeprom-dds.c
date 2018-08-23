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

typedef struct {
	rave_eeprom_dds_page_t page_number;
	const char *page_name;
} page_info_t;

static const page_info_t page_infos[] = {
	{ RAVE_EEPROM_DDS_PAGE_FORMAT, "format" },
	{ RAVE_EEPROM_DDS_PAGE_PARTNUM, "partnum" },
	{ RAVE_EEPROM_DDS_PAGE_SERIAL, "serial" },
	{ RAVE_EEPROM_DDS_PAGE_RDU_SERIAL, "rdu-serial" },
	{ RAVE_EEPROM_DDS_PAGE_FLAGS1, "flags1" },
	{ RAVE_EEPROM_DDS_PAGE_FLAGS2, "flags2" },
	{ RAVE_EEPROM_DDS_PAGE_FLAGS3, "flags3" },
	{ RAVE_EEPROM_DDS_PAGE_ENGINEERING_TESTS, "engineering-tests" },
	{ RAVE_EEPROM_DDS_PAGE_SDUMAINT, "sdumaint" },
	{ RAVE_EEPROM_DDS_PAGE_ASSEMBLY_PARTNUM, "assembly-partnum" },
	{ RAVE_EEPROM_DDS_PAGE_ASSEMBLY_SERIALNUM, "assembly-serialnum" },
	{ RAVE_EEPROM_DDS_PAGE_ASSEMBLY_DOM, "assembly-dom" },
	{ RAVE_EEPROM_DDS_PAGE_BOARD_REV, "board-rev" },
	{ RAVE_EEPROM_DDS_PAGE_BOARD_REV_UPDATE, "board-rev-update" },
	{ RAVE_EEPROM_DDS_PAGE_BOARD_DOM_UPDATE, "board-dom-update" },
};

const char *rave_eeprom_dds_page_to_string(rave_eeprom_dds_page_t page_number)
{
	int i;

	for (i = 0; i < (sizeof(page_infos)/sizeof(page_infos[0])); i++) {
		if (page_number == page_infos[i].page_number)
			return page_infos[i].page_name;
	}

	return NULL;
}

void rave_eeprom_dds_page_foreach(rave_eeprom_dds_page_foreach_func callback, void *user_data)
{
	int i;

	for (i = 0; i < (sizeof(page_infos)/sizeof(page_infos[0])); i++)
		callback(page_infos[i].page_number, user_data);
}

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

rave_eeprom_dds_seat_class_t rave_eeprom_dds_seat_class_from_string(const char *str)
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

rave_eeprom_dds_display_type_t rave_eeprom_dds_display_type_from_string(const char *str)
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

rave_eeprom_dds_location_type_t rave_eeprom_dds_location_type_from_string(const char *str)
{
	int i;

	for (i = RAVE_EEPROM_DDS_LOCATION_TYPE_UNSET; i < RAVE_EEPROM_DDS_LOCATION_TYPE_MAX; i++) {
		if (strcmp(str, dds_location_type_str[i]) == 0)
			return (rave_eeprom_dds_location_type_t)i;
	}
	return RAVE_EEPROM_DDS_LOCATION_TYPE_UNKNOWN;
}
