#ifndef _UTILS_H_
#define _UTILS_H_

#include <linux/types.h>

static inline size_t get_length( u64 n ) {
    return snprintf( NULL, 0, "%llu", n );
} 

#endif /* _UTILS_H_ */