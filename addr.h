#pragma once

/**
 * @file addr.h
 * @brief Type definitions for a virtual and physical addresses.
 *
 * @date 2018-19
 */

#include <stdint.h>

#define PAGE_OFFSET 12
#define PAGE_SIZE 4096 // = 2^12 B = 4 kiB pages

#define PTE_ENTRY 9
#define PMD_ENTRY 9
#define PUD_ENTRY 9
#define PGD_ENTRY 9

#define PTE_PREV_SIZE 12
#define PMD_PREV_SIZE 21
#define PUD_PREV_SIZE 30
#define PGD_PREV_SIZE 39
/* the number of entries in a page directory = 2^9
* each entry size is equal to the size of a physical address = 32b
*/
#define PD_ENTRIES 512

#define VIRT_PAGE_NUM 36 // = PTE_ENTRY + PUD_ENTRY + PMD_ENTRY + PGD_ENTRY
#define VIRT_ADDR_RES 16
#define VIRT_ADDR 64 // = VIRT_ADDR_RES + 4*9 + PAGE_OFFSET

#define PHY_PAGE_NUM 20
#define PHY_ADDR 32 // = PHY_PAGE_NUM + PAGE_OFFSET

/**
 * @brief type representing a word in memory
 */
typedef uint32_t word_t;

/**
 * @brief type representing a byte in memory
 */
typedef uint8_t byte_t;

/**
 * @brief type representing a page table entry
 */
typedef uint32_t pte_t;

/**
 * @brief type representing a virtual address in memory organized in 4 chunks of 
 * 9 bits a page offset  of 12 bits (shared by the the physical address) 
 * and 16 reserved bits. 
 */

typedef struct
{
	uint16_t reserved : VIRT_ADDR_RES;
	uint16_t pgd_entry : PGD_ENTRY;
	uint16_t pud_entry : PUD_ENTRY;
	uint16_t pmd_entry : PMD_ENTRY;
	uint16_t pte_entry : PTE_ENTRY;
	uint16_t page_offset : PAGE_OFFSET;
} virt_addr_t;

/**
 * @brief type representing a physical address in memory  with physical_ page number generated by the TLB process
 * and a page offset  of 12 bits (shared by the the virtual address) 
 */

typedef struct
{
	uint32_t phy_page_num : PHY_PAGE_NUM;
	uint16_t page_offset : PAGE_OFFSET;
} phy_addr_t;
