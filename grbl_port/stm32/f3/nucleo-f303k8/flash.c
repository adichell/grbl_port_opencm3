/*
  flash.c - flash access functions
  Part of grbl_port_opencm3 project.

  Copyright (c) 2017 Angelo Di Chello

  Grbl_port_opencm3 is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl_port_opencm3 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl_port_opencm3.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libopencm3/stm32/flash.h>
#include <flash.h>
#include <grbl.h>

#define FLASH_WAIT_FOR_LAST_OP while((FLASH_SR & FLASH_SR_BSY) == FLASH_SR_BSY);
#define FLASH_CR_LOCK_OPERATION  FLASH_CR |= FLASH_CR_LOCK;
#define FLASH_CR_UNLOCK_OPERATION do{FLASH_KEYR = FLASH_KEYR_KEY1;FLASH_KEYR = FLASH_KEYR_KEY2;}while(0);

/*! \brief  Write an half-word at a given FLASH address.
 *
 *   This function writes one half-word at a given FLASH address.
 *
 *  \param  address FLASH address to write to.
 *
 *  \param  data The half-word written to the FLASH address.
 */
__attribute__( ( long_call, section(".ramcode") ) )
static void flash_program_half_word_private(uint32_t address, uint16_t data)
{
    FLASH_WAIT_FOR_LAST_OP

    FLASH_CR |= FLASH_CR_PG;

    MMIO16(address) = data;

    FLASH_WAIT_FOR_LAST_OP

    FLASH_CR &= ~FLASH_CR_PG;
}

/*! \brief  Write a word at a given FLASH address.
 *
 *   This function writes one 32-bit word at a given FLASH address.
 *
 *  \param  address FLASH address to write to.
 *
 *  \param  data The word written to the FLASH address.
 */
static void flash_program_word_private(uint32_t address, uint32_t data)
{
    flash_program_half_word_private(address, (uint16_t)data);
    flash_program_half_word_private(address+2, (uint16_t)(data>>16));
}

/*! \brief  Erase a given FLASH sector.
 *
 *   This function erases a given sector of FLASH.
 *
 *  \param  sector FLASH sector to be erased.
 *
 *  \param  program_size program/erase access size.
 */
__attribute__( ( long_call, section(".ramcode") ) )
static void flash_erase_page_private(uint32_t page_address)
{
    FLASH_WAIT_FOR_LAST_OP

    FLASH_CR |= FLASH_CR_PER;
    FLASH_AR = page_address;
    FLASH_CR |= FLASH_CR_STRT;

    FLASH_WAIT_FOR_LAST_OP

    FLASH_CR &= ~FLASH_CR_PER;
}

/*! \brief  Read byte from FLASH.
 *
 *  This function reads one byte from a given FLASH address.
 *
 *  \note  The CPU is halted for 4 clock cycles during FLASH read.
 *
 *  \param  addr  FLASH address to read from.
 *
 *  \return  The byte read from the FLASH address.
 */
unsigned char flash_get_char( unsigned int addr )
{
    unsigned char value = *((unsigned char*)addr);
    FLASH_WAIT_FOR_LAST_OP // Wait for completion of previous write.
    /* Temporarily do not manage read protection RDP */
    return value; // Return the byte read from EEPROM.
}


/*! \brief  Read half word from FLASH.
 *
 *  This function reads 2 bytes from a given FLASH address.
 *
 *  \note  The CPU is halted for 4 clock cycles during FLASH read.
 *
 *  \param  addr  FLASH address to read from.
 *
 *  \return  The bytes read from the FLASH address.
 */
uint16_t flash_get_half_word( unsigned int addr )
{
    uint16_t value = *((uint16_t*)addr);
    FLASH_WAIT_FOR_LAST_OP // Wait for completion of previous write.

    return value;
}

/*! \brief  Verifies if an erase is needed to write the given payload.
 *
 *   Check if the values to be written in FLASH needs to set to 1 a bit
 *   that is at currently at 0, since this needs an erase operation.
 *
 *  \param  destination pointer to current stored payload bytes.
 *
 *  \param  source pointer to payload bytes.
 *
 *  \param  size number of bytes to be checked.
 */
unsigned int flash_verify_erase_need(uint16_t * destination, uint16_t *source, unsigned int size)
{
    unsigned int i;
    uint16_t new_value; // New EFLASH value.
    uint16_t old_value; // Old EFLASH value.
    uint16_t diff_mask; // Difference mask, i.e. old value XOR new value.
    uint16_t old_checksum = 0;
    uint16_t new_checksum = 0;

    for(i = 0; i < (size/2); i++)
    {
        new_value = *(source+i); // new EFLASH value.
        old_value = *(destination+i); // Get old EFLASH value.
        diff_mask = old_value ^ new_value; // Get bit differences.

        // Check if any bits are changed to '1' in the new value.
        if( diff_mask & new_value )
        {
            return ((unsigned int)1);
        }

        /* Calculate checksums to be able to verify them at the loop end */
        new_checksum = (new_checksum << 1) | (new_checksum >> 7);
        new_checksum += *(source+i);

        old_checksum = (old_checksum << 1) | (old_checksum >> 7);
        old_checksum += *(destination+i);
    }

    diff_mask = old_checksum ^ new_checksum; // Get bit differences.
    // Check if any bits needs to be changed to '1' in the checksum.
    if( diff_mask & new_checksum )
    {
        return ((unsigned int)1);
    }

    return ((unsigned int)0);

}

/*! \brief  Write byte to FLASH.
 *
 *  This function writes one byte to a given FLASH address.
 *  The differences between the existing byte and the new value is used
 *  to select the most efficient FLASH programming mode.
 *
 *  \param  addr FLASH address to write to.
 *
 *  \param  new_value  New FLASH value to write.
 *
 */
void flash_put_char( unsigned int addr, unsigned char new_value)
{
    /* Work around since this flash has no 8-bit access capability */
    uint16_t rebuilt_value;
    if (addr % 2 == 0)
    {
        rebuilt_value = (((*((uint16_t*)addr)) & 0xFF00) | new_value);
    }
    else
    {
        rebuilt_value = ((*((uint16_t*)(addr & 0xFFFFFFFE)) & 0x00FF) | (((uint16_t)new_value) << 8));
    }
    //__disable_irq(); // Ensure atomic operation for the write operation.
    FLASH_CR_UNLOCK_OPERATION

    flash_program_half_word_private((uint32_t) (addr & 0xFFFFFFFE), (uint16_t) rebuilt_value);
    
    FLASH_CR_LOCK_OPERATION
    //__enable_irq(); // Restore interrupt flag state.
}


/*! \brief Store payload bytes in flash.
 *
 *   Store an array of bytes in flash.
 *
 *  \param  destination pointer to location where to store payload.
 *
 *  \param  source pointer to payload bytes.
 *
 *  \param  size number of bytes to be stored.
 */
void memcpy_to_flash_with_checksum(unsigned int destination, char *source, unsigned int size)
{
    uint16_t checksum = 0;
    uint16_t* src = ((uint16_t*)source);

    FLASH_CR_UNLOCK_OPERATION

    for(; size > 0; size = size-2)
    {
        checksum = (checksum << 1) | (checksum >> 7);
        checksum += *src;
        flash_program_half_word_private(((uint32_t)destination), *src);
        destination += 2;
        src++;
    }
    flash_program_half_word_private(((uint32_t)destination), checksum);

    FLASH_CR_LOCK_OPERATION
}


/*! \brief Read payload bytes from flash.
 *
 *   Read an array of bytes from flash.
 *
 *  \param  destination pointer to location where the payload will be read.
 *
 *  \param  source pointer address to bytes to be read.
 *
 *  \param  size number of bytes to be stored.
 */
int memcpy_from_flash_with_checksum(char *destination, unsigned int source, unsigned int size)
{
  uint16_t data;
  uint16_t checksum = 0;
  uint16_t* dst = ((uint16_t*)destination);

  for(; size > 0; size = size-2)
  {
    data = flash_get_half_word(source);
    checksum = (checksum << 1) | (checksum >> 7);
    checksum += data;
    *(dst) = data;
    dst++;
    source += 2;
  }
  return(checksum == flash_get_half_word(source));
}


/*! \brief Update main sector status word.
 *
 *   Update main sector status word.
 *
 *  \param  updated_status status value to be updated in the reserved flash address.
 *
 */
void update_main_sector_status(uint32_t updated_status)
{
    FLASH_CR_UNLOCK_OPERATION
    flash_program_word_private(((uint32_t)EFLASH_MAIN_SECTOR_STATUS), updated_status);
    FLASH_CR_LOCK_OPERATION
}

/*! \brief Delete main sector.
 */
void delete_main_sector(void)
{
    FLASH_CR_UNLOCK_OPERATION
    flash_erase_page_private((uint32_t)EFLASH_MAIN_BASE_ADDRESS);
    FLASH_CR_LOCK_OPERATION
}

/*! \brief Delete copy sector.
 */
void delete_copy_sector(void)
{
    FLASH_CR_UNLOCK_OPERATION
    flash_erase_page_private((uint32_t)EFLASH_COPY_BASE_ADDRESS);
    FLASH_CR_LOCK_OPERATION
}

void copy_from_main_to_copy(uint32_t start_address_offset, uint32_t end_address_offset)
{
    uint32_t * address = (uint32_t*)(start_address_offset + EFLASH_MAIN_BASE_ADDRESS);
    uint32_t value;
    uint32_t i;

    FLASH_CR_UNLOCK_OPERATION

    for(i = 0; (start_address_offset+(i<<2)) < end_address_offset; i++)
    {
        value = *(address+i); // new EFLASH value.
        flash_program_word_private((start_address_offset+(i<<2)+EFLASH_COPY_BASE_ADDRESS), value);
    }

    FLASH_CR_LOCK_OPERATION
}

void restore_main_sector()
{
    uint32_t * address = ((uint32_t*)EFLASH_COPY_BASE_ADDRESS);
    uint32_t destination = ((uint32_t)EFLASH_MAIN_BASE_ADDRESS);
    uint32_t value;
    uint32_t i;

    FLASH_CR_UNLOCK_OPERATION
    for(i = 0; i < (EFLASH_ERASE_AND_RESTORE_OFFSET / 4); i++)
    {
        value = *(address+i); // new EFLASH value.
        flash_program_word_private(((i<<2)+destination), value);
    }
    FLASH_CR_LOCK_OPERATION
}


void restore_default_sector_status()
{
    uint32_t* main_sector_status_addr = ((uint32_t*)EFLASH_MAIN_SECTOR_STATUS);

    switch(*main_sector_status_addr)
    {
    case(MAIN_SECTOR_COPIED):
        //delete main-sector and update status is done implicitly
        delete_main_sector();
    case(MAIN_SECTOR_ERASED):
        //copy from copy-sector to main-sector to restore it and update status
        restore_main_sector();
    case(MAIN_SECTOR_RESTORED):
        //delete copy sector and update status
        delete_copy_sector();
        update_main_sector_status(COPY_SECTOR_CLEARED);
        break;
    case(COPY_SECTOR_CLEARED):
        break;
    default:
        //delete main and copy sector, setting restore function will do the rest
        delete_main_sector();
        delete_copy_sector();
        break;
    }
}
