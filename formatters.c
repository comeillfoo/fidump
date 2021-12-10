#include "formatters.h"

size_t uint32_to_str( char** dest, const char* fmt, size_t fmt_len, unsigned int num ) {
    size_t dest_sz = 0;
    size_t len = 0;

    len = get_length( num );
    
    *dest = kmalloc( sizeof( char ) * ( len + fmt_len + 1 ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": uint32_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": uint32_to_str: [ dest: %p, fmt: \"%s\", fmt_len: %zu, num: %u ]\n", dest, fmt, fmt_len, num );
        return 0;
    }
    
    dest_sz = snprintf( *dest, sizeof( char ) * ( len + fmt_len + 1 ), fmt, num );
    return dest_sz;
}


size_t uint64_to_str( char** dest, const char* fmt, size_t fmt_len, u64 num ) {
    size_t dest_sz = 0;
    size_t len = 0;

    len = get_length( num );
    
    *dest = kmalloc( sizeof( char ) * ( len + fmt_len + 1 ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": uint64_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": uint64_to_str: [ dest: %p, fmt: \"%s\", fmt_len: %zu, num: %llu ]\n", dest, fmt, fmt_len, num );
        return 0;
    }
    
    dest_sz = snprintf( *dest, sizeof( char ) * ( len + fmt_len + 1 ), fmt, num );
    return dest_sz;
}


size_t ptr_to_str( void const * const ptr, char** dest, const char* key, size_t key_len ) {
    const char* fmt = "\"%s\": \"%016p\",";
    size_t fmt_len = 7;
    size_t dest_sz = 0;
    size_t len = 16;

    *dest = kmalloc( sizeof( char ) * ( len + fmt_len + key_len + 1 ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: [ dest: %p, ptr: %p ]\n", dest, ptr );
        return 0;
    }
    
    dest_sz = snprintf( *dest, sizeof( char ) * ( len + fmt_len + key_len + 1 ), fmt, key, ptr );
    return dest_sz;
}


size_t str_to_str( const char* str, char** dest, const char* key, size_t key_len ) {
    const char* fmt = "\"%s\": \"%s\",";
    size_t fmt_len = 7;
    size_t dest_sz = 0;
    size_t len = strlen( str );

    *dest = kmalloc( sizeof( char ) * ( len + fmt_len + key_len + 1 ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: [ dest: %p, str: \"%s\" ]\n", dest, str );
        return 0;
    }
    
    dest_sz = snprintf( *dest, sizeof( char ) * ( len + fmt_len + key_len + 1 ), fmt, key, str );
    return dest_sz;
}


size_t fd_to_str( unsigned int fd, char** fd_buf ) {
    return uint32_to_str( fd_buf, "\"%u\": {", 5, fd );
}