#ifndef _FORMATTERS_H_
#define _FORMATTERS_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "utils.h"

#define MOD_NAME "fidump"

size_t uint32_to_str( char** dest, const char* fmt, size_t fmt_len, unsigned int num );


size_t uint64_to_str( char** dest, const char* fmt, size_t fmt_len, u64 num );


size_t ptr_to_str( void const * const ptr, char** dest, const char* key, size_t key_len );


size_t str_to_str( const char* str, char** dest, const char* key, size_t key_len );


size_t fd_to_str( unsigned int fd, char** fd_buf );

#endif /* _FORMATTERS_H_ */