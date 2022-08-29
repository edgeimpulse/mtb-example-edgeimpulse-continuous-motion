/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EI_FLASH_MEMORY_H
#define EI_FLASH_MEMORY_H

#include "firmware-sdk/ei_device_memory.h"

extern "C" {
	#include "cy_pdl.h"
	#include "cyhal.h"
	#include "cybsp.h"
	#include <cycfg_qspi_memslot.h>
	#include <cy_serial_flash_qspi.h>
};

#define QSPI_MEM_SLOT_NUM       (0u)		 /* QSPI slot to use */
#define QSPI_BUS_FREQUENCY_HZ   (50000000lu) /* 50 Mhz */
/*
  Flash Related Parameter Define
*/
#define FLASH_ERASE_TIME    500 // Typical time is 450ms + 10% buffer
#define FLASH_SIZE          0x4000000   // 64 MB
#define FLASH_SECTOR_SIZE   0x40000     // 256K Sector size
#define FLASH_PAGE_SIZE     0x0200      // 512 Byte Page size
#define FLASH_BLOCK_NUM     (FLASH_SIZE / SECTOR_SIZE)

class EiFlashMemory : public EiDeviceMemory {
protected:
    uint32_t read_data(uint8_t *data, uint32_t address, uint32_t num_bytes);
    uint32_t write_data(const uint8_t *data, uint32_t address, uint32_t num_bytes);
    uint32_t erase_data(uint32_t address, uint32_t num_bytes);

public:
    EiFlashMemory(uint32_t config_size);
};

#endif /* EI_FLASH_MEMORY_H */
