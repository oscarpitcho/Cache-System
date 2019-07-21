
#include "tlb_mng.h"
#include "tlb.h"
#include "addr.h"
#include "addr_mng.h"
#include <stdint.h>
#include "page_walk.h"

#define VALID 1;

int tlb_entry_init(const virt_addr_t *vaddr, const phy_addr_t *paddr, tlb_entry_t *tlb_entry)
{
    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(tlb_entry);
    tlb_entry->v = VALID;
    tlb_entry->phy_page_num = paddr->phy_page_num;
    tlb_entry->tag = virt_addr_t_to_virtual_page_number(vaddr);
    return ERR_NONE;
}

int tlb_flush(tlb_entry_t *table)
{
    M_REQUIRE_NON_NULL(table);
    void *ret = memset(table, 0, sizeof(tlb_entry_t) * TLB_LINES);
    return ret == NULL ? ERR_MEM : ERR_NONE;
}

int tlb_insert(uint32_t line_index,
               const tlb_entry_t *tlb_entry,
               tlb_entry_t *tlb)
{

    M_REQUIRE(line_index < TLB_LINES, ERR_BAD_PARAMETER, "%u is out of bounds", line_index);
    M_REQUIRE_NON_NULL(tlb);
    M_REQUIRE_NON_NULL(tlb_entry);
    tlb[line_index] = *tlb_entry;
    return ERR_NONE;
}

int tlb_hit(const virt_addr_t *vaddr,
            phy_addr_t *paddr,
            const tlb_entry_t *tlb,
            replacement_policy_t *replacement_policy)
{

    //0 to indicate it failed (and we did not find the desired entry in the tlb)
    M_REQUIRE_NON_NULL_CUSTOM_ERR(vaddr, 0);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(paddr, 0);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(tlb, 0);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(replacement_policy, 0);
    uint64_t virt_page_nbr = virt_addr_t_to_virtual_page_number(vaddr);
    for_all_nodes_reverse(node, replacement_policy->ll)
    {
        tlb_entry_t tmp = tlb[node->value];
        if (tmp.tag == virt_page_nbr && tmp.v)
        {
            init_phy_addr(paddr, tmp.phy_page_num << PAGE_OFFSET, vaddr->page_offset);
            replacement_policy->move_back(replacement_policy->ll, node);
            return 1;
        }
    }
    return 0;
}

int tlb_search(const void *mem_space,
               const virt_addr_t *vaddr,
               phy_addr_t *paddr,
               tlb_entry_t *tlb,
               replacement_policy_t *replacement_policy,
               int *hit_or_miss)
{
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(tlb);
    M_REQUIRE_NON_NULL(replacement_policy);
    M_REQUIRE_NON_NULL(hit_or_miss);
    *hit_or_miss = tlb_hit(vaddr, paddr, tlb, replacement_policy);
    if (!*hit_or_miss)
    {
        M_EXIT_IF_ERR(page_walk(mem_space, vaddr, paddr), "error occured while pagewalking in tlb search");
        int tlb_index = replacement_policy->ll->front->value;
        tlb_entry_t t;
        M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &t), "ERROR in tlb_entry_init");
        M_EXIT_IF_ERR(tlb_insert(tlb_index, &t, tlb), "Error in tlb_inset");
        move_back(replacement_policy->ll, replacement_policy->ll->front);
    }
    return ERR_NONE;
}
