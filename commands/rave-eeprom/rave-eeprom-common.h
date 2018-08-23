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

#ifndef __RAVE_EEPROM_COMMON_H
#define __RAVE_EEPROM_COMMON_H

#define RAVE_EEPROM_COMMON_PAGE_SIZE 0x20

uint16_t rave_eeprom_common_page_crc_read(const uint8_t *page);
uint16_t rave_eeprom_common_page_crc_compute(const uint8_t *page);
bool rave_eeprom_common_page_crc_validate(const uint8_t *page);
void rave_eeprom_common_page_crc_update(uint8_t *page);

bool rave_eeprom_common_page_check_initialized(const uint8_t *page);

int rave_eeprom_common_open(const char *path, int mode);
int rave_eeprom_common_read_page(int fd, unsigned int page_number, uint8_t *page);
int rave_eeprom_common_write_page(int fd, unsigned int page_number, uint8_t *page);

#endif
