#include <stdio.h>
#include <inttypes.h>
#include "addr_mng.h"
#include "addr.h"
#include "error.h"

#define MAX_VAL_ENTRIES 0x1FF
#define MAX_VAL_OFFSET 0xFFF
#define MAX_VAL_VADDR 0x1FFFFFFFFFFFF

int print_virtual_address(FILE *where, const virt_addr_t *vaddr)
{
	M_REQUIRE_NON_NULL(where);
	M_REQUIRE_NON_NULL(vaddr);
	int a = fprintf(where, "PGD=0x%" PRIX16 "; PUD=0x%" PRIX16 "; PMD=0x%" PRIX16 "; PTE=0x%" PRIX16 "; offset=0x%" PRIX16,
					vaddr->pgd_entry, vaddr->pud_entry, vaddr->pmd_entry, vaddr->pte_entry, vaddr->page_offset);
	return a;
}

int init_virt_addr(virt_addr_t *vaddr,
				   uint16_t pgd_entry,
				   uint16_t pud_entry, uint16_t pmd_entry,
				   uint16_t pte_entry, uint16_t page_offset)
{
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE(page_offset <= MAX_VAL_OFFSET, ERR_BAD_PARAMETER, "Input value %lu is not a 12 bit number", page_offset);
	M_REQUIRE(pte_entry <= MAX_VAL_ENTRIES, ERR_BAD_PARAMETER, "Input value %lu is not a 12 bit number", pte_entry);
	M_REQUIRE(pmd_entry <= MAX_VAL_ENTRIES, ERR_BAD_PARAMETER, "Input value %lu is not a 12 bit number", pmd_entry);
	M_REQUIRE(pud_entry <= MAX_VAL_ENTRIES, ERR_BAD_PARAMETER, "Input value %lu is not a 12 bit number", pud_entry);
	M_REQUIRE(pgd_entry <= MAX_VAL_ENTRIES, ERR_BAD_PARAMETER, "Input value %lu is not a 12 bit number", pgd_entry);

	vaddr->pgd_entry = pgd_entry;
	vaddr->pud_entry = pud_entry;
	vaddr->pmd_entry = pmd_entry;
	vaddr->pte_entry = pte_entry;
	vaddr->page_offset = page_offset;
	vaddr->reserved = 0;
	return ERR_NONE;
}

int init_virt_addr64(virt_addr_t *vaddr, uint64_t vaddr64)
{
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE(vaddr64 <= MAX_VAL_VADDR, ERR_BAD_PARAMETER, "Input value %lu is not a 49 bit number", vaddr64);
	uint16_t page_offset = vaddr64 & MAX_VAL_OFFSET;
	uint16_t pgd = (vaddr64 >> PGD_PREV_SIZE) & MAX_VAL_ENTRIES;
	uint16_t pud = (vaddr64 >> PUD_PREV_SIZE) & MAX_VAL_ENTRIES;
	uint16_t pmd = (vaddr64 >> PMD_PREV_SIZE) & MAX_VAL_ENTRIES;
	uint16_t pte = (vaddr64 >> PTE_PREV_SIZE) & MAX_VAL_ENTRIES;
	return init_virt_addr(vaddr, pgd, pud, pmd, pte, page_offset);
}

int print_physical_address(FILE *where, const phy_addr_t *paddr)
{
	M_REQUIRE_NON_NULL(where);
	M_REQUIRE_NON_NULL(paddr);
	int a = fprintf(where, "page num=0x%" PRIX16 "; offset=0x%" PRIX16,
					paddr->phy_page_num, paddr->page_offset);
	return a;
}

uint64_t virt_addr_t_to_uint64_t(const virt_addr_t *vaddr)
{
	M_REQUIRE_NON_NULL(vaddr);
	return (virt_addr_t_to_virtual_page_number(vaddr) << PAGE_OFFSET) | vaddr->page_offset;
}

uint64_t virt_addr_t_to_virtual_page_number(const virt_addr_t *vaddr)
{
	M_REQUIRE_NON_NULL(vaddr);
	uint64_t page_number = 0;
	page_number |= ((uint64_t)vaddr->pte_entry);
	page_number |= ((uint64_t)vaddr->pmd_entry) << PTE_ENTRY;
	page_number |= ((uint64_t)vaddr->pud_entry) << (PTE_ENTRY + PMD_ENTRY);
	page_number |= ((uint64_t)vaddr->pgd_entry) << (PTE_ENTRY + PMD_ENTRY + PUD_ENTRY);
	return page_number;
}

int init_phy_addr(phy_addr_t *paddr, uint32_t page_begin, uint32_t page_offset)
{
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE(page_offset <= MAX_VAL_OFFSET, ERR_BAD_PARAMETER, "Input value %lu is not a 12 bit unsigned integer", page_offset);
	M_REQUIRE(page_begin % PAGE_SIZE == 0, ERR_BAD_PARAMETER, "Wrong page begin argument, not multiple of page size");
	paddr->page_offset = page_offset;
	paddr->phy_page_num = (page_begin) >> PAGE_OFFSET;
	return ERR_NONE;
}
