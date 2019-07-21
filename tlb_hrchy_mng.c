#include "tlb_hrchy_mng.h"
#include "tlb_hrchy.h"
#include "error.h"
#include "addr_mng.h"
#include "page_walk.h"
#define VALID 1

static uint64_t compute_index(virt_addr_t *vaddr, tlb_t type)
{
    if (type == L1_DTLB || type == L1_ITLB)
    {
        return virt_addr_t_to_virtual_page_number(vaddr) % L1_ITLB_LINES;
    }
    else
        return virt_addr_t_to_virtual_page_number(vaddr) % L2_TLB_LINES;
}

static uint64_t compute_page_from_tagAndIndex(uint32_t tag, int index, tlb_t type)
{
    if (type == L1_DTLB || type == L1_ITLB)
    {
        return ((((uint64_t)tag) << L1_ITLB_LINES_BITS) | index) << PAGE_OFFSET;
    }
    else
        return ((((uint64_t)tag) << L2_TLB_LINES_BITS) | index) << PAGE_OFFSET;
}

int tlb_entry_init(const virt_addr_t *vaddr,
                   const phy_addr_t *paddr,
                   void *tlb_entry,
                   tlb_t tlb_type)
{
#define init(type, NOB)                                              \
    ((type *)tlb_entry)->v = VALID;                                  \
    ((type *)tlb_entry)->phy_page_num = paddr->phy_page_num;         \
    uint64_t tmp = virt_addr_t_to_virtual_page_number(vaddr) >> NOB; \
    ((type *)tlb_entry)->tag = tmp;

    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(tlb_entry);

    switch (tlb_type)
    {
    case L1_ITLB:
    {
        init(l1_itlb_entry_t, L1_ITLB_LINES_BITS);
    }
    break;
    case L1_DTLB:
    {
        init(l1_dtlb_entry_t, L1_DTLB_LINES_BITS);
    }
    break;
    case L2_TLB:
    {
        init(l2_tlb_entry_t, L2_TLB_LINES_BITS);
    }
    break;
    default:
        break;
    }
    return ERR_NONE;
}

int tlb_flush(void *tlb, tlb_t tlb_type)
{
    M_REQUIRE_NON_NULL(tlb);
    void *ret;
    switch (tlb_type)
    {
    case L1_ITLB:
    {
        ret = memset(tlb, 0, sizeof(l1_itlb_entry_t) * L1_ITLB_LINES);
    }
    break;
    case L1_DTLB:
    {
        ret = memset(tlb, 0, sizeof(l1_dtlb_entry_t) * L1_DTLB_LINES);
    }
    break;
    case L2_TLB:
    {
        ret = memset(tlb, 0, sizeof(l2_tlb_entry_t) * L2_TLB_LINES);
    }
    break;
    default:
        break;
    }
    return ret == NULL ? ERR_MEM : ERR_NONE;
}

int tlb_insert(uint32_t line_index, // correcteur this function can SEGFAULT
               const void *tlb_entry,
               void *tlb,
               tlb_t tlb_type)
{
#define insert(type, NOV)                                                              \
    M_REQUIRE(line_index < NOV, ERR_BAD_PARAMETER, "%u is out of bounds", line_index); \
    ((type *)tlb)[line_index] = *((type *)tlb_entry);

    M_REQUIRE_NON_NULL(tlb);
    M_REQUIRE_NON_NULL(tlb_entry);
    switch (tlb_type)
    {
    case L1_ITLB:
    {
        insert(l1_itlb_entry_t, L1_ITLB_LINES);
    }
    break;
    case L1_DTLB:
    {
        insert(l1_dtlb_entry_t, L1_DTLB_LINES);
    }
    break;
    case L2_TLB:
    {
        insert(l2_tlb_entry_t, L2_TLB_LINES);
    }
    break;
    }
    return ERR_NONE;
}

int tlb_hit(const virt_addr_t *vaddr,
            phy_addr_t *paddr,
            const void *tlb,
            tlb_t tlb_type)
{
#define hit(type, NOB, NOV)                                                                               \
    int index = virt_page_nbr % NOV;                                                                      \
    type tmp = ((type *)tlb)[index];                                                                      \
    uint64_t computed_virt_page = compute_page_from_tagAndIndex(tmp.tag, index, tlb_type) >> PAGE_OFFSET; \
    if (computed_virt_page == virt_page_nbr && tmp.v)                                                     \
    {                                                                                                     \
        init_phy_addr(paddr, tmp.phy_page_num << PAGE_OFFSET, vaddr->page_offset);                        \
        return 1;                                                                                         \
    }                                                                                                     \
    M_REQUIRE_NON_NULL_CUSTOM_ERR(vaddr, 0);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(paddr, 0);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(tlb, 0);
    uint64_t virt_page_nbr = virt_addr_t_to_virtual_page_number(vaddr);
    switch (tlb_type)
    {
    case L1_ITLB:
    {
        hit(l1_itlb_entry_t, L1_ITLB_LINES_BITS, L1_ITLB_LINES);
    }
    break;
    case L1_DTLB:
    {
        hit(l1_dtlb_entry_t, L1_DTLB_LINES_BITS, L1_DTLB_LINES);
    }
    break;
    case L2_TLB:
    {
        hit(l2_tlb_entry_t, L2_TLB_LINES_BITS, L2_TLB_LINES);
    }
    break;
    }
    return 0;
}

int tlb_search(const void *mem_space,
               const virt_addr_t *vaddr,
               phy_addr_t *paddr,
               mem_access_t access,
               l1_itlb_entry_t *l1_itlb,
               l1_dtlb_entry_t *l1_dtlb,
               l2_tlb_entry_t *l2_tlb,
               int *hit_or_miss)
{
#define search(type, not_type, tlb, not_tlb)                                                            \
    *hit_or_miss = tlb_hit(vaddr, paddr, tlb, type);                                                    \
    if (*hit_or_miss)                                                                                   \
        return ERR_NONE;                                                                                \
    *hit_or_miss = tlb_hit(vaddr, paddr, l2_tlb, L2_TLB);                                               \
    l1_dtlb_entry_t lvl1;                                                                               \
    uint64_t tlb1_index = compute_index(vaddr, type);                                                   \
    if (*hit_or_miss)                                                                                   \
    {                                                                                                   \
        tlb_entry_init(vaddr, paddr, &lvl1, type);                                                      \
        tlb_insert(tlb1_index, &lvl1, tlb, type);                                                       \
        return ERR_NONE;                                                                                \
    }                                                                                                   \
    M_EXIT_IF_ERR(page_walk(mem_space, vaddr, paddr), "error occured while pagewalking in tlb search"); \
    uint64_t tlb2_index = compute_index(vaddr, L2_TLB);                                                 \
    l2_tlb_entry_t tmp = l2_tlb[tlb2_index];                                                            \
    virt_addr_t virt_addr;                                                                              \
    phy_addr_t phys_addr;                                                                               \
    init_virt_addr64(&virt_addr, compute_page_from_tagAndIndex(tmp.tag, tlb2_index, L2_TLB));           \
    int check = tlb_hit(&virt_addr, &phys_addr, not_tlb, not_type);                                     \
    if (check)                                                                                          \
    {                                                                                                   \
        uint64_t lolz = compute_index(&virt_addr, type);                                                \
        not_tlb[lolz].v = 0;                                                                            \
    }                                                                                                   \
    l2_tlb_entry_t lvl2;                                                                                \
    tlb_entry_init(vaddr, paddr, &lvl2, L2_TLB);                                                        \
    tlb_insert(tlb2_index, &lvl2, l2_tlb, L2_TLB);                                                      \
    tlb_entry_init(vaddr, paddr, &lvl1, type);                                                          \
    tlb_insert(tlb1_index, &lvl1, tlb, type);

    // tlb_hit can hit in l2 even if not in l1, so might remove too often

    switch (access)
    {
    case INSTRUCTION:
    {
        search(L1_ITLB, L1_DTLB, l1_itlb, l1_dtlb);
    }
    break;
    case DATA:
    {
        search(L1_DTLB, L1_ITLB, l1_dtlb, l1_itlb);
    }
    break;
    }
    return ERR_NONE;
}
