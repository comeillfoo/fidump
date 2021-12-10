#ifndef _UTILS_H_
#define _UTILS_H_

#include <linux/types.h>

static inline size_t __do_get_length( u64 n ) {
    if ( n == 0 )
        return 0;
    else return __do_get_length( n / 10 ) + 1;
}


static inline size_t get_length( u64 n ) {
    const size_t length = __do_get_length( n );
    return ( length == 0 )? 1 : length;
} 

#endif /* _UTILS_H_ */