#ifndef FLASH_STORAGE_H_
#define FLASH_STORAGE_H_


#include "sam.h"
#include <stdint.h>
#include <stdbool.h>
#include "flash_efc.h"
#include "efc.h"

// 1Kb of data
#define DATA_LENGTH   ((IFLASH1_PAGE_SIZE/sizeof(uint8_t))*4)

// choose a start address that's offset to show that it doesn't have to be on a page boundary
#define  FLASH_START  ((uint8_t *)IFLASH1_ADDR)

//  FLASH_DEBUG can be enabled to get debugging information displayed.
//#define FLASH_DEBUG


#ifdef FLASH_DEBUG
//#include "myfile.h"
#define _FLASH_DEBUG(x) ErrorMessage((char*)x)
#else
#define _FLASH_DEBUG(x)
#endif

//  DueFlash is the main class for flash functions
extern	uint8_t flash_read_8(uint32_t address);
extern	uint8_t* flash_readAddress(uint32_t address);
extern	bool flash_write_8(uint32_t address, uint8_t value);
extern	bool flash_write_buffer(uint32_t address, uint8_t *data, uint32_t dataLength);
extern void my_flash_init() ;


#endif /* FLASH_STORAGE_H_ */