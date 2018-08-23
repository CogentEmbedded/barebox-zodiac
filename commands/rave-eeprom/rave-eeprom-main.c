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

typedef struct {
	rave_eeprom_main_page_t page_number;
	const char *page_name;
} page_info_t;

static const page_info_t page_infos[] = {
	{ RAVE_EEPROM_MAIN_PAGE_FORMAT, "format" },
	{ RAVE_EEPROM_MAIN_PAGE_PARTNUM, "partnum" },
	{ RAVE_EEPROM_MAIN_PAGE_CONFIG, "config" },
	{ RAVE_EEPROM_MAIN_PAGE_SERIAL, "serial" },
	{ RAVE_EEPROM_MAIN_PAGE_FLAGS1, "flags1" },
	{ RAVE_EEPROM_MAIN_PAGE_FLAGS2, "flags2" },
	{ RAVE_EEPROM_MAIN_PAGE_BOARD_DOM, "board-dom" },
	{ RAVE_EEPROM_MAIN_PAGE_BOARD_PARTNUM, "board-partnum" },
	{ RAVE_EEPROM_MAIN_PAGE_BOARD_SERIAL, "board-serial" },
	{ RAVE_EEPROM_MAIN_PAGE_BOARD_REV, "board-rev" },
	{ RAVE_EEPROM_MAIN_PAGE_BOARD_REV_UPDATE, "board-rev-update" },
	{ RAVE_EEPROM_MAIN_PAGE_BOARD_DOM_UPDATE, "board-dom-update" },
	{ RAVE_EEPROM_MAIN_PAGE_SDUMAINT, "sdumaint" },
	{ RAVE_EEPROM_MAIN_PAGE_PRIMARY_ENC_TEST_MSG, "primary-enc-test-msg" },
	{ RAVE_EEPROM_MAIN_PAGE_PRIMARY_PLAYER_KEY_INFO, "primary-player-key-info" },
	{ RAVE_EEPROM_MAIN_PAGE_PRIMARY_PLAYER_KEY, "primary-player-key" },
	{ RAVE_EEPROM_MAIN_PAGE_PLAYER_KEYS, "player-keys" },
	{ RAVE_EEPROM_MAIN_PAGE_ENCRYPTED_TEST_MSG_SIZE, "encrypted-test-msg-size" },
	{ RAVE_EEPROM_MAIN_PAGE_SECONDARY_ENC_TEST_MSG, "secondary-enc-test-msg" },
	{ RAVE_EEPROM_MAIN_PAGE_SECONDARY_PLAYER_KEY_INFO, "secondary-player-key-info" },
	{ RAVE_EEPROM_MAIN_PAGE_SECONDARY_PLAYER_KEY, "secondary-player-key" },
};

const char *rave_eeprom_main_page_to_string(rave_eeprom_main_page_t page_number)
{
	int i;

	for (i = 0; i < (sizeof(page_infos)/sizeof(page_infos[0])); i++) {
		if (page_number == page_infos[i].page_number)
			return page_infos[i].page_name;
	}

	return NULL;
}

void rave_eeprom_main_page_foreach(rave_eeprom_main_page_foreach_func callback, void *user_data)
{
	int i;

	for (i = 0; i < (sizeof(page_infos)/sizeof(page_infos[0])); i++)
		callback(page_infos[i].page_number, user_data);
}
