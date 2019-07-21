#include "page_walk.h"
#include "addr.h"
#include "addr_mng.h"
#include "error.h"

static inline pte_t read_page_entry(const pte_t *start, pte_t page_start, uint16_t index)
{
  return start[(page_start >> 2) + (index)];
}

int page_walk(const void *mem_space, const virt_addr_t *vaddr, phy_addr_t *paddr)
{
  M_REQUIRE_NON_NULL(mem_space);
  M_REQUIRE_NON_NULL(vaddr);
  M_REQUIRE_NON_NULL(paddr);
  pte_t pudAddress = read_page_entry(mem_space, 0, vaddr->pgd_entry);

  // correcteur check return values
  pte_t pmdAddress = read_page_entry(mem_space, pudAddress, vaddr->pud_entry);
  pte_t pteAddress = read_page_entry(mem_space, pmdAddress, vaddr->pmd_entry);
  pte_t physicalAddress = read_page_entry(mem_space, pteAddress, vaddr->pte_entry);
  return init_phy_addr(paddr, physicalAddress, vaddr->page_offset);
}
