#include "Flash_Storage.h"

void my_flash_init() 
{
	uint32_t retCode;

	/* Initialize flash: 6 wait states for flash writing. */
	retCode = flash_init(FLASH_ACCESS_MODE_128, 6);
	if (retCode != FLASH_RC_OK) {
		_FLASH_DEBUG("Flash init failed\n");
	}
}

uint8_t flash_read_8(uint32_t address) {
	return FLASH_START[address];
}
uint8_t* flash_readAddress(uint32_t address) {
	return FLASH_START+address;
}

bool flash_write_8(uint32_t address, uint8_t value) {
	uint32_t retCode;
	uint32_t byteLength = 1;
	//  uint8_t *data;

	retCode = flash_unlock((uint32_t)FLASH_START+address, (uint32_t)FLASH_START+address + byteLength - 1, 0, 0);
	if (retCode != FLASH_RC_OK) {
		_FLASH_DEBUG("Failed to unlock flash for write\n");
		return false;
	}

	// write data
	retCode = flash_write((uint32_t)FLASH_START+address, &value, byteLength, 1);
	//retCode = flash_write((uint32_t)FLASH_START, data, byteLength, 1);

	if (retCode != FLASH_RC_OK) {
		_FLASH_DEBUG("Flash write failed\n");
		return false;
	}

	// Lock page
	retCode = flash_lock((uint32_t)FLASH_START+address, (uint32_t)FLASH_START+address + byteLength - 1, 0, 0);
	if (retCode != FLASH_RC_OK) {
		_FLASH_DEBUG("Failed to lock flash page\n");
		return false;
	}
	return true;
}

bool flash_write_buffer(uint32_t address, uint8_t *data, uint32_t dataLength) {
	uint32_t retCode;

	if ((uint32_t)FLASH_START+address < IFLASH1_ADDR) {
		_FLASH_DEBUG("Flash write address too low\n");
		return false;
	}

	if ((uint32_t)FLASH_START+address >= (IFLASH1_ADDR + IFLASH1_SIZE)) {
		_FLASH_DEBUG("Flash write address too high\n");
		return false;
	}

	if ((((uint32_t)FLASH_START+address) & 3) != 0) {
		_FLASH_DEBUG("Flash start address must be on four uint8_t boundary\n");
		return false;
	}

	// Unlock page
	retCode = flash_unlock((uint32_t)FLASH_START+address, (uint32_t)FLASH_START+address + dataLength - 1, 0, 0);
	if (retCode != FLASH_RC_OK) {
		_FLASH_DEBUG("Failed to unlock flash for write\n");
		return false;
	}

	// write data
	retCode = flash_write((uint32_t)FLASH_START+address, data, dataLength, 1);

	if (retCode != FLASH_RC_OK) {
		_FLASH_DEBUG("Flash write failed\n");
		return false;
	}

	// Lock page
	retCode = flash_lock((uint32_t)FLASH_START+address, (uint32_t)FLASH_START+address + dataLength - 1, 0, 0);
	if (retCode != FLASH_RC_OK) {
		_FLASH_DEBUG("Failed to lock flash page\n");
		return false;
	}
	return true;
}

