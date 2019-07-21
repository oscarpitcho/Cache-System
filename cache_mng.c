/**
 * @file cache_mng.c
 * @brief cache management functions
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */

#include "error.h"
#include "util.h"
#include "cache_mng.h"
#include "cache.h"
#include "lru.h"
#include "addr_mng.h"
#include <inttypes.h> // for PRIx macros

//=========================================================================
int cache_flush(void *cache, cache_t cache_type)
{
#define flush(TYPE, number_lines, number_ways, words_per_line)      \
    {                                                               \
        for (size_t i = 0; i < (number_lines); ++i)                 \
        {                                                           \
            foreach_way(j, number_ways)                             \
            {                                                       \
                cache_tag(TYPE, number_ways, (i), (j)) = 0;         \
                cache_valid(TYPE, number_ways, (i), (j)) = 0;       \
                cache_age(TYPE, number_ways, (i), (j)) = 0;         \
                for (size_t k = 0; k < words_per_line; k++)         \
                {                                                   \
                    cache_line(TYPE, number_ways, (i), (j))[k] = 0; \
                }                                                   \
            }                                                       \
        }                                                           \
    }
    M_REQUIRE_NON_NULL(cache);
    switch (cache_type)
    {
    case L1_ICACHE:
        flush(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS, L1_ICACHE_WORDS_PER_LINE);
        break;
    case L1_DCACHE:
        flush(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS, L1_DCACHE_WORDS_PER_LINE);
        break;
    case L2_CACHE:
        flush(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS, L2_CACHE_WORDS_PER_LINE);
        break;
    }
    return ERR_NONE;
}

//=========================================================================

static inline uint32_t compose_phys_addr(phy_addr_t *paddr)
{
    uint32_t a = (paddr->phy_page_num << PAGE_OFFSET) | paddr->page_offset;
    return a;
}
static uint32_t index_for_paddr(phy_addr_t *paddr, cache_t cache_type)
{
    switch (cache_type)
    {
    case L1_ICACHE:
        return (compose_phys_addr(paddr) / 16) % L1_ICACHE_LINES;
        break;
    case L1_DCACHE:
        return (compose_phys_addr(paddr) / 16) % L1_DCACHE_LINES;
        break;
    case L2_CACHE:
        return (compose_phys_addr(paddr) / 16) % L2_CACHE_LINES;
        break;
    default:
        break;
    }
    return 0;
}
int cache_hit(const void *mem_space, void *cache, phy_addr_t *paddr, const uint32_t **p_line, uint8_t *hit_way, uint16_t *hit_index, cache_t cache_type)
{
#define hit(type, ways, remaining, lines, cache_type)                                                 \
    do                                                                                                \
    {                                                                                                 \
        uint32_t phys_addr = compose_phys_addr(paddr);                                                \
        uint32_t index = index_for_paddr(paddr, cache_type);                                          \
        uint32_t tag = phys_addr >> remaining;                                                        \
        foreach_way(j, ways)                                                                          \
        {                                                                                             \
            if ((cache_tag(type, ways, index, j) == tag) && (cache_valid(type, ways, index, j) == 1)) \
            {                                                                                         \
                *hit_way = j;                                                                         \
                *hit_index = index;                                                                   \
                *p_line = cache_line(type, ways, index, j);                                           \
                LRU_age_update(type, ways, j, index);                                                 \
                return ERR_NONE;                                                                      \
            }                                                                                         \
        }                                                                                             \
        *hit_way = HIT_WAY_MISS;                                                                      \
        *hit_index = HIT_INDEX_MISS;                                                                  \
        return ERR_NONE;                                                                              \
    } while (0);

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(cache);
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(mem_space);

    switch (cache_type)
    {
    case L1_ICACHE:
    {
        hit(l1_icache_entry_t, L1_ICACHE_WAYS, L1_ICACHE_TAG_REMANING, L1_ICACHE_LINES, L1_ICACHE);
        break;
    }
    case L1_DCACHE:
    {
        hit(l1_dcache_entry_t, L1_DCACHE_WAYS, L1_DCACHE_TAG_REMANING, L1_DCACHE_LINES, L1_DCACHE);
        break;
    }
    case L2_CACHE:
    {
        hit(l2_cache_entry_t, L2_CACHE_WAYS, L2_CACHE_TAG_REMANING, L2_CACHE_LINES, L2_CACHE);
        break;
    }
    }
}

//=========================================================================
int cache_insert(uint16_t cache_line_index, uint8_t cache_way, const void *cache_line_in, void *cache, cache_t cache_type)
{
#define insert(type, ways, lines)                                                                                                             \
    M_REQUIRE(cache_way < ways, ERR_BAD_PARAMETER, "your cache way: %d is not between 0 and %d", cache_way, ways - 1);                        \
    M_REQUIRE(cache_line_index < lines, ERR_BAD_PARAMETER, "your cache line index: %d is not between 0 and %d", cache_line_index, lines - 1); \
    memcpy(cache_entry(type, ways, cache_line_index, cache_way), cache_line_in, sizeof(type));                                                \
    //LRU_age_update(type, ways, cache_way, cache_line_index);

    M_REQUIRE_NON_NULL(cache_line_in);
    M_REQUIRE_NON_NULL(cache);

    switch (cache_type)
    {
    case L1_ICACHE:
    {
        insert(l1_icache_entry_t, L1_ICACHE_WAYS, L1_ICACHE_LINES);
        break;
    }
    case L1_DCACHE:
    {
        insert(l1_dcache_entry_t, L1_DCACHE_WAYS, L1_DCACHE_LINES);
        break;
    }
    case L2_CACHE:
    {
        insert(l2_cache_entry_t, L2_CACHE_WAYS, L2_CACHE_LINES);
        break;
    }
    }
    return ERR_NONE;
}

//=========================================================================
int cache_entry_init(const void *mem_space, const phy_addr_t *paddr, void *cache_entry, cache_t cache_type)
{
#define init(type, shift_value, words_per_line)                                  \
    ((type *)cache_entry)->v = 1;                                                \
    ((type *)cache_entry)->age = 0;                                              \
    uint32_t phys = compose_phys_addr(paddr);                                    \
    ((type *)cache_entry)->tag = phys >> shift_value;                            \
    for (size_t i = 0; i < words_per_line; i++)                                  \
    {                                                                            \
        ((type *)cache_entry)->line[i] = ((word_t *)mem_space)[(phys >> 2) + i]; \
    }
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(cache_entry);
    switch (cache_type)
    {
    case L1_ICACHE:
    {
        init(l1_icache_entry_t, L1_ICACHE_TAG_REMANING, L1_ICACHE_WORDS_PER_LINE);
    }
    break;
    case L1_DCACHE:
    {
        init(l1_dcache_entry_t, L1_DCACHE_TAG_REMANING, L1_DCACHE_WORDS_PER_LINE);
    }
    break;
    case L2_CACHE:
    {
        init(l2_cache_entry_t, L2_CACHE_TAG_REMANING, L2_CACHE_WORDS_PER_LINE);
    }
    break;
    }
    return ERR_NONE;
}
static int isWordAligned(phy_addr_t *paddr)
{
    int ret = compose_phys_addr(paddr) % 4 ? ERR_NONE : ERR_BAD_PARAMETER;
    return ret;
}

static void entry_to_evict(phy_addr_t *paddr, void *cache, uint8_t *evict, uint8_t *way_to_write, cache_t cache_type)
{

#define find_entry_to_evict(type, ways, lines, words_per_line, cache_type)        \
    do                                                                            \
    {                                                                             \
        uint32_t line_index = index_for_paddr(paddr, cache_type);                 \
        *evict = 1;                                                               \
        *way_to_write = 0;                                                        \
        uint8_t oldest_way = 0;                                                   \
        foreach_way(j, ways)                                                      \
        {                                                                         \
            if (!cache_valid(type, ways, line_index, j))                          \
            {                                                                     \
                *evict = 0;                                                       \
                *way_to_write = j;                                                \
                return;                                                           \
            }                                                                     \
            else if (cache_age(type, L1_ICACHE_WAYS, line_index, j) > oldest_way) \
            {                                                                     \
                *way_to_write = j;                                                \
                oldest_way = cache_age(type, L1_ICACHE_WAYS, line_index, j);      \
            }                                                                     \
        }                                                                         \
    } while (0);

    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(cache);
    M_REQUIRE_NON_NULL(evict);
    M_REQUIRE_NON_NULL(way_to_write);

    switch (cache_type)
    {
    case L1_ICACHE:
    {
        find_entry_to_evict(l1_icache_entry_t, L1_ICACHE_WAYS, L1_ICACHE_LINES, L1_ICACHE_WORDS_PER_LINE, L1_ICACHE);
        break;
    }
    case L1_DCACHE:
    {
        find_entry_to_evict(l1_dcache_entry_t, L1_DCACHE_WAYS, L1_DCACHE_LINES, L1_DCACHE_WORDS_PER_LINE, L1_DCACHE);
        break;
    }
    case L2_CACHE:
    {
        find_entry_to_evict(l2_cache_entry_t, L2_CACHE_WAYS, L2_CACHE_LINES, L2_CACHE_WORDS_PER_LINE, L2_CACHE);
        break;
    }
    default:
        break;
    }
}

static l2_cache_entry_t transform_l1_entry_to_l2(void *l1_entry, cache_t source_type)
{
    l2_cache_entry_t newEntry;
    newEntry.age = 0;
    newEntry.v = 1;
    switch (source_type)
    {
    case L1_ICACHE:
    {
        memcpy(newEntry.line, ((l1_icache_entry_t *)l1_entry)->line, L1_ICACHE_WORDS_PER_LINE * sizeof(word_t));
        newEntry.tag = ((l1_icache_entry_t *)l1_entry)->tag >> 3;
    }
    break;
    case L1_DCACHE:
    {
        memcpy(newEntry.line, ((l1_dcache_entry_t *)l1_entry)->line, L1_DCACHE_WORDS_PER_LINE * sizeof(word_t));
        newEntry.tag = ((l1_dcache_entry_t *)l1_entry)->tag >> 3;
    }
    break;
    default:
        break;
    }
    return newEntry;
}

static phy_addr_t paddr_from_index_and_tag(uint16_t index, uint32_t tag, cache_t source_cache)
{
    uint32_t addr = 0;
    switch (source_cache)
    {
    case L1_ICACHE || L1_DCACHE:
        addr = (tag << 10) | (index << 4);
        break;
    case L2_CACHE:
        addr = (tag << 13) | (index << 4);
        break;
    default:
        break;
    }
    phy_addr_t p;
    init_phy_addr(&p, addr >> PAGE_OFFSET, addr & 0xFFF);
    return p;
}

//=========================================================================
int cache_read(const void *mem_space, phy_addr_t *paddr, mem_access_t access, void *l1_cache, void *l2_cache, uint32_t *word, cache_replace_t replace)
{

#define cache_read_modular(type, words_per_line, ways, cache_type)                                                                                                                            \
    do                                                                                                                                                                                        \
    {                                                                                                                                                                                         \
        void *cache = l1_cache;                                                                                                                                                               \
        word_t *p_line;                                                                                                                                                                       \
        M_EXIT_IF_ERR(cache_hit(mem_space, cache, paddr, &p_line, &hit_way, &hit_index, cache_type), "Error in cache hit");                                                                   \
        if (hit_way != HIT_WAY_MISS)                                                                                                                                                          \
        {                                                                                                                                                                                     \
            *word = p_line[(paddr->page_offset >> 2) % words_per_line];                                                                                                                       \
            LRU_age_update(type, ways, hit_way, hit_index);                                                                                                                                   \
        }                                                                                                                                                                                     \
        else                                                                                                                                                                                  \
        {                                                                                                                                                                                     \
            cache = l2_cache;                                                                                                                                                                 \
            M_EXIT_IF_ERR(cache_hit(mem_space, cache, paddr, &p_line, &hit_way, &hit_index, L2_CACHE), "Error in cache hit");                                                                 \
            if (hit_way != HIT_WAY_MISS)                                                                                                                                                      \
            {                                                                                                                                                                                 \
                type new_l1_entry;                                                                                                                                                            \
                *word = p_line[(paddr->page_offset >> 2) % words_per_line];                                                                                                                   \
                new_l1_entry.age = 0;                                                                                                                                                         \
                new_l1_entry.v = 1;                                                                                                                                                           \
                M_REQUIRE_NON_NULL_CUSTOM_ERR(memcpy(new_l1_entry.line, cache_line(l2_cache_entry_t, L2_CACHE_WAYS, hit_index, hit_way), L2_CACHE_WORDS_PER_LINE * sizeof(word_t)), ERR_MEM); \
                new_l1_entry.tag = (cache_tag(l2_cache_entry_t, L2_CACHE_WAYS, hit_index, hit_way) << 3) | (hit_index >> 6);                                                                  \
                uint8_t evict = 1;               ;                                                                                                                                             \
                uint8_t way_to_write = 0;                                                                                                                                                     \
                entry_to_evict(paddr, l1_cache, &evict, &way_to_write, cache_type);                                                                                                           \
                if (evict)                                                                                                                                                                    \
                {                                                                                                                                                                             \
                    cache = l1_cache;                                                                                                                                                         \
                    l2_cache_entry_t new_l2_entry = transform_l1_entry_to_l2(cache_entry(type, ways, index_for_paddr(paddr, cache_type), way_to_write), L1_ICACHE);                           \
                    cache = l2_cache;                                                                                                                                                         \
                                                                                                                                                                                              \
                    M_EXIT_IF_ERR(cache_insert(index_for_paddr(paddr, cache_type), way_to_write, &new_l1_entry, l1_cache, cache_type), "Error in cache insert");                              \
                    phy_addr_t first_word_paddr = paddr_from_index_and_tag(hit_index, new_l2_entry.tag, L2_CACHE);                                                                            \
                                                                                                                                                                                              \
                    entry_to_evict(&first_word_paddr, l2_cache, evict, way_to_write, L2_CACHE);                                                                                               \
                                                                                                                                                                                              \
                    M_EXIT_IF_ERR(cache_insert(index_for_paddr(paddr, L2_CACHE), way_to_write, &new_l2_entry, l2_cache, L2_CACHE), "Error in cache insert");                                  \
                }                                                                                                                                                                             \
                else                                                                                                                                                                          \
                {                                                                                                                                                                             \
                    cache = l1_cache;                                                                                                                                                         \
                    M_EXIT_IF_ERR(cache_insert(index_for_paddr(paddr, cache_type), way_to_write, &new_l1_entry, l1_cache, cache_type), "Error in cache insert");                              \
                    LRU_age_increase(type, ways, way_to_write, index_for_paddr(paddr, cache_type));                                                                                           \
                }                                                                                                                                                                             \
            }                                                                                                                                                                                 \
            else                                                                                                                                                                              \
            {                                                                                                                                                                                 \
                uint8_t evict = 1;                                                                                                                                                            \
                uint8_t way_to_write = 0;                                                                                                                                                     \
                entry_to_evict(paddr, l1_cache, &evict, &way_to_write, cache_type);                                                                                                           \
                type new_l1_entry;                                                                                                                                                            \
                cache_entry_init(mem_space, paddr, &new_l1_entry, cache_type);                                                                                                                \
                if (evict)                                                                                                                                                                    \
                {                                                                                                                                                                             \
                    cache = l1_cache;                                                                                                                                                         \
                    l2_cache_entry_t new_l2_entry = transform_l1_entry_to_l2(cache_entry(type, ways, index_for_paddr(paddr, cache_type), way_to_write), cache_type);                          \
                    cache = l2_cache;                                                                                                                                                         \
                                                                                                                                                                                              \
                    M_EXIT_IF_ERR(cache_insert(index_for_paddr(paddr, cache_type), way_to_write, &new_l1_entry, l1_cache, cache_type), "error in insertion");                                 \
                    phy_addr_t first_word_paddr = paddr_from_index_and_tag(hit_index, new_l2_entry.tag, L2_CACHE);                                                                            \
                                                                                                                                                                                              \
                    entry_to_evict(&first_word_paddr, l2_cache, evict, way_to_write, L2_CACHE);                                                                                               \
                    M_EXIT_IF_ERR(cache_insert(index_for_paddr(paddr, L2_CACHE), way_to_write, &new_l2_entry, l2_cache, L2_CACHE), "error in insertion");                                     \
                    LRU_age_update(type, ways, hit_way, index_for_paddr(paddr, cache_type));                                                                                                  \
                }                                                                                                                                                                             \
                else                                                                                                                                                                          \
                {                                                                                                                                                                             \
                    cache = l1_cache;                                                                                                                                                         \
                    M_EXIT_IF_ERR(cache_entry_init(mem_space, paddr, &new_l1_entry, cache_type), "Err in cache entry init");                                                                  \
                    M_EXIT_IF_ERR(cache_insert(index_for_paddr(paddr, cache_type), way_to_write, &new_l1_entry, l1_cache, cache_type), "ERRin cache insert");                                 \
                    LRU_age_increase(type, ways, way_to_write, index_for_paddr(paddr, cache_type));                                                                                           \
                }                                                                                                                                                                             \
                uint16_t hit_index = HIT_INDEX_MISS;                                                                                                                                          \
                uint8_t hit_way = HIT_WAY_MISS;                                                                                                                                               \
                word_t p_line[words_per_line];                                                                                                                                                \
                M_EXIT_IF_ERR(cache_hit(mem_space, l1_cache, paddr, &p_line, &hit_way, &hit_index, cache_type), "Err in cache hit");                                                          \
                                                                                                                                                                                              \
                *word = p_line[(paddr->page_offset >> 2) % words_per_line];                                                                                                                   \
                return ERR_NONE;                                                                                                                                                              \
            }                                                                                                                                                                                 \
        }                                                                                                                                                                                     \
    } while (0);

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(access);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(word);
    M_EXIT_ERR(isWordAligned(paddr), "address is not word aligned");
    M_REQUIRE(replace == LRU, ERR_BAD_PARAMETER, "only LRU replacement policy implemented");

    int hit_way = HIT_WAY_MISS;
    int hit_index = HIT_INDEX_MISS;
    switch (access)
    {
    case INSTRUCTION:
    {
        cache_read_modular(l1_icache_entry_t, L1_ICACHE_WORDS_PER_LINE, L1_ICACHE_WAYS, L1_ICACHE);
        break;
    }
    case DATA:
    {
        cache_read_modular(l1_dcache_entry_t, L1_DCACHE_WORDS_PER_LINE, L1_DCACHE_WAYS, L1_DCACHE);
        break;
    }
    default:
        break;
    }
}

//=========================================================================
int cache_read_byte(const void *mem_space,
                    phy_addr_t *p_paddr,
                    mem_access_t access,
                    void *l1_cache,
                    void *l2_cache,
                    uint8_t *p_byte,
                    cache_replace_t replace)
{
    word_t word = 0;
    M_EXIT_IF_ERR(cache_read(mem_space, p_paddr, access, l1_cache, l2_cache, &word, replace), "Error when reading");
    *p_byte = word & 0x0000000F;
    return ERR_NONE;
}
//=========================================================================
int cache_write(void *mem_space,
                phy_addr_t *paddr,
                void *l1_cache,
                void *l2_cache,
                const uint32_t *word,
                cache_replace_t replace)
{
#define hit_in_cache(cache_type, lines)                                                                   \
    do                                                                                                    \
    {                                                                                                     \
        size_t lineIndex = (paddr->phy_page_num >> 4) % lines;                                            \
        p_line[hit_way] = *word;                                                                          \
        M_EXIT_IF_ERR(cache_insert(lineIndex, hit_way, p_line, cache, cache_type), "Error in insertion"); \
        ((word_t *)mem_space)[compose_phys_addr(paddr) >> 2] = *word;                                     \
    } while (0)

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(word);
    M_REQUIRE(paddr->page_offset % 4, ERR_BAD_PARAMETER, "Physical address not aligned", NULL);

    word_t p_line[L1_DCACHE_WORDS_PER_LINE];
    uint8_t hit_way = 0;
    uint16_t hit_index = 0;
    void *cache = l1_cache; //For functionning macros

    //L1 lookup
    M_EXIT_IF_ERR(cache_hit(mem_space, cache, paddr, &p_line, &hit_way, &hit_index, L1_DCACHE), "Error in hit");
    if (hit_way != HIT_WAY_MISS)
    {
        p_line[hit_way] = *word;
        cache_line(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_index, hit_way)[(compose_phys_addr(paddr) >> 2) % L1_DCACHE_WORDS_PER_LINE] = *word;
        LRU_age_update(l1_dcache_entry_t, L1_DCACHE_WAYS, hit_way, hit_index);
        return ERR_NONE;
    }

    //L2 lookup
    cache = l2_cache;
    M_EXIT_IF_ERR(cache_hit(mem_space, cache, paddr, &p_line, &hit_way, hit_index, L2_CACHE), "Error in hit");
    if (hit_way != HIT_WAY_MISS)
    {
        hit_in_cache(L2_CACHE, L2_CACHE_LINES);
    }
    ((word_t *)mem_space)[compose_phys_addr(paddr) >> 2] = *word;

    uint32_t tag = paddr->phy_page_num >> L1_DCACHE_TAG_REMANING;
    size_t lineIndex = (paddr->phy_page_num >> 4) % L1_DCACHE_LINES;
    uint8_t oldest = 0;
    uint8_t overwrite = 0;
    ;
    //Put evicted in victim cache
    if (overwrite)
    {
    }

    //TODO: Put in the new word, evict the old and eventually put in 2 if it should be done
    //Also do the copyback from L2 to L1.
}

//=========================================================================
int cache_write_byte(void *mem_space,
                     phy_addr_t *paddr,
                     void *l1_cache,
                     void *l2_cache,
                     uint8_t p_byte,
                     cache_replace_t replace)
{
    word_t word = 0;
    M_EXIT_IF_ERR(cache_read(mem_space, paddr, DATA, l1_cache, l2_cache, word, replace), "cache read error");
    word = (word & 0xFFFFFFF0) | (p_byte & ~(0xFFFFFFF0));
    M_EXIT_IF_ERR(cache_write(mem_space, paddr, l1_cache, l2_cache, word, replace), "cache write error");
    return ERR_NONE;
}

//=========================================================================
int cache_dump(FILE *output, const void *cache, cache_t cache_type);

//=========================================================================
#define PRINT_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE)                 \
    do                                                                                         \
    {                                                                                          \
        fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: %1" PRIx8 ", TAG: 0x%03" PRIx16 ", values: ( ", \
                cache_valid(TYPE, WAYS, LINE_INDEX, WAY),                                      \
                cache_age(TYPE, WAYS, LINE_INDEX, WAY),                                        \
                cache_tag(TYPE, WAYS, LINE_INDEX, WAY));                                       \
        for (int i_ = 0; i_ < WORDS_PER_LINE; i_++)                                            \
            fprintf(OUTFILE, "0x%08" PRIx32 " ",                                               \
                    cache_line(TYPE, WAYS, LINE_INDEX, WAY)[i_]);                              \
        fputs(")\n", OUTFILE);                                                                 \
    } while (0)

#define PRINT_INVALID_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE)                                    \
    do                                                                                                                    \
    {                                                                                                                     \
        fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: -, TAG: -----, values: ( ---------- ---------- ---------- ---------- )\n", \
                cache_valid(TYPE, WAYS, LINE_INDEX, WAY));                                                                \
    } while (0)

#define DUMP_CACHE_TYPE(OUTFILE, TYPE, WAYS, LINES, WORDS_PER_LINE)                                  \
    do                                                                                               \
    {                                                                                                \
        for (uint16_t index = 0; index < LINES; index++)                                             \
        {                                                                                            \
            foreach_way(way, WAYS)                                                                   \
            {                                                                                        \
                fprintf(output, "%02" PRIx8 "/%04" PRIx16 ": ", way, index);                         \
                if (cache_valid(TYPE, WAYS, index, way))                                             \
                    PRINT_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE);         \
                else                                                                                 \
                    PRINT_INVALID_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE); \
            }                                                                                        \
        }                                                                                            \
    } while (0)

//=========================================================================
// see cache_mng.h
int cache_dump(FILE *output, const void *cache, cache_t cache_type)
{
    M_REQUIRE_NON_NULL(output);
    M_REQUIRE_NON_NULL(cache);

    fputs("WAY/LINE: V: AGE: TAG: WORDS\n", output);
    switch (cache_type)
    {
    case L1_ICACHE:
        DUMP_CACHE_TYPE(output, l1_icache_entry_t, L1_ICACHE_WAYS,
                        L1_ICACHE_LINES, L1_ICACHE_WORDS_PER_LINE);
        break;
    case L1_DCACHE:
        DUMP_CACHE_TYPE(output, l1_dcache_entry_t, L1_DCACHE_WAYS,
                        L1_DCACHE_LINES, L1_DCACHE_WORDS_PER_LINE);
        break;
    case L2_CACHE:
        DUMP_CACHE_TYPE(output, l2_cache_entry_t, L2_CACHE_WAYS,
                        L2_CACHE_LINES, L2_CACHE_WORDS_PER_LINE);
        break;
    default:
        debug_print("%d: unknown cache type", cache_type);
        return ERR_BAD_PARAMETER;
    }
    putc('\n', output);

    return ERR_NONE;
}
