/*
    This file is part of AutoQuad.

    AutoQuad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AutoQuad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with AutoQuad.  If not, see <http://www.gnu.org/licenses/>.

    Copyright � 2011, 2012  Bill Nesbitt
*/

#include "flash.h"

// Gets the sector of a given address
uint32_t GetSector(uint32_t Address) {
uint32_t sector = 0;

    if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0)) {
	sector = FLASH_Sector_0;
    }
    else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1)) {
	sector = FLASH_Sector_1;
    }
    else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2)) {
	sector = FLASH_Sector_2;
    }
    else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3)) {
	sector = FLASH_Sector_3;
    }
    else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4)) {
	sector = FLASH_Sector_4;
    }
    else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5)) {
	sector = FLASH_Sector_5;
    }
    else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6)) {
	sector = FLASH_Sector_6;
    }
    else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7)) {
	sector = FLASH_Sector_7;
    }
    else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8)) {
	sector = FLASH_Sector_8;
    }
    else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9)) {
	sector = FLASH_Sector_9;
    }
    else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10)) {
	sector = FLASH_Sector_10;
    }
    else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/ {
	sector = FLASH_Sector_11;
    }

    return sector;
}

int flashAddress(uint32_t startAddr, uint32_t *data, unsigned int len) {
    FLASH_Status status;
    unsigned int retries;
    int ret;
    unsigned int i;

    if (startAddr == 0)
	startAddr = FLASH_START_ADDR;

    FLASH_Unlock();

    // Clear pending flags (if any)
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

    ret = 1;

    for (i = GetSector(startAddr); i <= GetSector(startAddr+len); i += 8) {
	retries = 0;
	do {
	    // Device voltage range supposed to be [2.7V to 3.6V], the operation will be done by word
	    status = FLASH_EraseSector(i, VoltageRange_3);
	    retries++;
	} while (status != FLASH_COMPLETE && retries < FLASH_RETRIES);

	if (retries == FLASH_RETRIES) {
	    ret = 0;
	    break;
	}
    }

    if (ret) {
	for (i = 0; i < len; i++) {
	    retries = 0;
	    do {
		status = FLASH_ProgramWord(startAddr + i*4, *(data + i));
		retries++;
	    } while (status != FLASH_COMPLETE && retries < FLASH_RETRIES);

	    if (retries == FLASH_RETRIES) {
		ret = 0;
		break;
	    }
	}
    }

    FLASH_Lock();

    return ret;
}

uint32_t flashStartAddr(void) {
    return FLASH_START_ADDR;
}

uint32_t flashSerno(uint8_t n) {
    return *((uint32_t *)(0x1FFF7A10) + n);
}
