#include "cache.h"
#include "cache_mng.h"

#define LRU_age_increase(TYPE, WAYS, WAY_INDEX, LINE_INDEX)  \
    do                                                       \
    {                                                        \
        foreach_way(j, WAYS)                                 \
        {                                                    \
            if (cache_age(TYPE, WAYS, LINE_INDEX, j) < WAYS) \
            {                                                \
                cache_age(TYPE, WAYS, LINE_INDEX, j)++;      \
            }                                                \
        }                                                    \
        cache_age(TYPE, WAYS, LINE_INDEX, WAY_INDEX) = 0;    \
                                                             \
    } while (0);

#define LRU_age_update(TYPE, WAYS, WAY_INDEX, LINE_INDEX)           \
    do                                                              \
    {                                                               \
        uint8_t age = cache_age(TYPE, WAYS, LINE_INDEX, WAY_INDEX); \
        foreach_way(j, WAYS)                                        \
        {                                                           \
            if (cache_age(TYPE, WAYS, LINE_INDEX, j) < age)         \
            {                                                       \
                cache_age(TYPE, WAYS, LINE_INDEX, j)++;             \
            }                                                       \
        }                                                           \
        cache_age(TYPE, WAYS, LINE_INDEX, WAY_INDEX) = 0;           \
    } while (0);