/**
 * @memory.c
 * @brief memory management functions (dump, init from file, etc.)
 *
 * @date 2018-19
 */

#if defined _WIN32 || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "memory.h"
#include "page_walk.h"
#include "addr_mng.h"
#include "util.h" // for SIZE_T_FMT
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>   // for memset()
#include <inttypes.h> // for SCNx macros
#include <assert.h>

#define BYTE_SIZE 1
#define FOURKI 4096

// ======================================================================
/**
 * @brief Tool function to print an address.
 *
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param reference the reference address; i.e. the top of the main memory
 * @param addr the address to be displayed
 * @param sep a separator to print after the address (and its colon, printed anyway)
 *
 */
static void address_print(addr_fmt_t show_addr, const void *reference,
                          const void *addr, const char *sep)
{
    switch (show_addr)
    {
    case POINTER:
        (void)printf("%p", addr);
        break;
    case OFFSET:
        (void)printf("%zX", (const char *)addr - (const char *)reference);
        break;
    case OFFSET_U:
        (void)printf(SIZE_T_FMT, (const char *)addr - (const char *)reference);
        break;
    default:
        // do nothing
        return;
    }
    (void)printf(":%s", sep);
}

// This function is actually the equivalent of page_file_read indicated in the pdf files
static int page_read(const char *filename, void *memory, size_t address)
{
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE(address % PAGE_SIZE == 0, ERR_BAD_PARAMETER, "Wrong address");
    FILE *pageDump = fopen(filename, "rb");
    M_REQUIRE_NON_NULL_CUSTOM_ERR(pageDump, ERR_IO);
    M_REQUIRE(fread(memory + address, 1, FOURKI, pageDump) == FOURKI, ERR_IO, "did not read all the file");
    fclose(pageDump);
    return ERR_NONE;
}

// ======================================================================
/**
 * @brief Tool function to print the content of a memory area
 *
 * @param reference the reference address; i.e. the top of the main memory
 * @param from first address to print
 * @param to first address NOT to print; if less that `from`, nothing is printed;
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param line_size how many memory byted to print per stdout line
 * @param sep a separator to print after the address and between bytes
 *
 */
static void mem_dump_with_options(const void *reference, const void *from, const void *to,
                                  addr_fmt_t show_addr, size_t line_size, const char *sep)
{
    assert(line_size != 0);
    size_t nb_to_print = line_size;
    for (const uint8_t *addr = from; addr < (const uint8_t *)to; ++addr)
    {
        if (nb_to_print == line_size)
        {
            address_print(show_addr, reference, addr, sep);
        }
        (void)printf("%02" PRIX8 "%s", *addr, sep);
        if (--nb_to_print == 0)
        {
            nb_to_print = line_size;
            putchar('\n');
        }
    }
    if (nb_to_print != line_size)
        putchar('\n');
}

// ======================================================================

int mem_init_from_dumpfile(const char *filename, void **memory, size_t *mem_capacity_in_bytes)
{
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);
    FILE *memoryDumpFile = fopen(filename, "rb");
    M_REQUIRE_NON_NULL_CUSTOM_ERR(memoryDumpFile, ERR_IO);
    fseek(memoryDumpFile, 0L, SEEK_END);
    *mem_capacity_in_bytes = (size_t)ftell(memoryDumpFile);
    rewind(memoryDumpFile);
    *memory = malloc(*mem_capacity_in_bytes);
    if (memory == NULL)
        return ERR_MEM;
    size_t readVal = fread(*memory, 1, *mem_capacity_in_bytes, memoryDumpFile);
    if (readVal != *mem_capacity_in_bytes)
	// correcteur close file in case of error
        return ERR_IO;
    fclose(memoryDumpFile);
    return ERR_NONE;
}

// ==========================================================================

int mem_init_from_description(const char *master_filename, void **memory, size_t *mem_capacity_in_bytes)
{
#define CLOSE_AND_RETURN(ERR)   \
    {                           \
        if (ERR != ERR_NONE)    \
        {                       \
            fclose(masterFile); \
            *memory = NULL;     \
            return ERR;         \
        }                       \
    }

    M_REQUIRE_NON_NULL(master_filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);
    int error = ERR_NONE;
    FILE *masterFile = fopen(master_filename, "rb"); // check if masterfile is really opened
    fscanf(masterFile, "%zu", mem_capacity_in_bytes);
    *memory = malloc(*mem_capacity_in_bytes);
    if (memory == NULL)
    {
        CLOSE_AND_RETURN(ERR_MEM);
    }
    char subFileName[100];

    fscanf(masterFile, "%s", subFileName);
    CLOSE_AND_RETURN(page_read(subFileName, *memory, 0));
    int numberOfPages = 0;
    fscanf(masterFile, "%d", &numberOfPages);
    uint64_t address = 0;

    for (size_t i = 0; i < numberOfPages; i++)
    {
        fscanf(masterFile, "%lx" SCNx32, &address);
        fscanf(masterFile, "%s", subFileName);
        CLOSE_AND_RETURN(page_read(subFileName, *memory, address));
    }
    virt_addr_t a = {0, 0, 0, 0, 0, 0};
    phy_addr_t b = {0, 0};
    while (!feof(masterFile) && !ferror(masterFile))
    {
        fscanf(masterFile, "%lx" SCNx64, &address);
        CLOSE_AND_RETURN(init_virt_addr64(&a, address));
        CLOSE_AND_RETURN(page_walk(*memory, &a, &b));
        fscanf(masterFile, "%s", subFileName);
        if (!feof(masterFile))
            CLOSE_AND_RETURN(page_read(subFileName, *memory, (b.phy_page_num << PAGE_OFFSET) | b.page_offset));
    }
    if (ferror(masterFile))
    {
        CLOSE_AND_RETURN(ERR_IO);
    }
    fclose(masterFile);
    return ERR_NONE;
}
// See memory.h for description
int vmem_page_dump_with_options(const void *mem_space, const virt_addr_t *from,
                                addr_fmt_t show_addr, size_t line_size, const char *sep)
{
#ifdef DEBUG
    debug_print("mem_space=%p\n", mem_space);
    (void)fprintf(stderr, __FILE__ ":%d:%s(): virt. addr.=", __LINE__, __func__);
    print_virtual_address(stderr, from);
    (void)fputc('\n', stderr);
#endif
    phy_addr_t paddr;
    zero_init_var(paddr);

    M_EXIT_IF_ERR(page_walk(mem_space, from, &paddr),
                  "calling page_walk() from vmem_page_dump_with_options()");
#ifdef DEBUG
    (void)fprintf(stderr, __FILE__ ":%d:%s(): phys. addr.=", __LINE__, __func__);
    print_physical_address(stderr, &paddr);
    (void)fputc('\n', stderr);
#endif

    const uint32_t paddr_offset = ((uint32_t)paddr.phy_page_num << PAGE_OFFSET);
    const char *const page_start = (const char *)mem_space + paddr_offset;
    const char *const start = page_start + paddr.page_offset;
    const char *const end_line = start + (line_size - paddr.page_offset % line_size);
    const char *const end = page_start + PAGE_SIZE;

    mem_dump_with_options(mem_space, page_start, start, show_addr, line_size, sep);
    const size_t indent = paddr.page_offset % line_size;
    if (indent == 0)
        putchar('\n');
    address_print(show_addr, mem_space, start, sep);
    for (size_t i = 1; i <= indent; ++i)
        printf("  %s", sep);
    mem_dump_with_options(mem_space, start, end_line, NONE, line_size, sep);
    mem_dump_with_options(mem_space, end_line, end, show_addr, line_size, sep);
    return ERR_NONE;
}
