#include <linux/slab.h>
#include "kstring.h"

#include "formatters.h"


void kstring_free( struct kstring* ptr_kstr ) {
    kfree( ptr_kstr->data );
    ptr_kstr->data = NULL;
    ptr_kstr->size = 0;
    ptr_kstr->capacity = 0;
}
EXPORT_SYMBOL( kstring_free );


static size_t kstring_get_capacity( size_t old_cap ) {
    return old_cap + old_cap / 2;
}


static bool kstring_is_full( struct kstring* ptr_kstr, size_t sz ) {
    return ( ( ptr_kstr->size + sz ) >= ptr_kstr->capacity );
}


static int kstring_grow( struct kstring* ptr_kstr, size_t sz ) {
    while ( kstring_is_full( ptr_kstr, sz ) )
        ptr_kstr->capacity = kstring_get_capacity( ptr_kstr->capacity );
    ptr_kstr->data = ( char* ) krealloc( ptr_kstr->data, ptr_kstr->capacity, GFP_KERNEL );
    if ( ptr_kstr->data == NULL ) {
        printk( KERN_CRIT MOD_NAME ": kstring_grow: can't reallocate kernel string\n" );
        printk( KERN_CRIT MOD_NAME ": kstring_grow: [ ptr_kstr: %p, capacity: %zu, old size: %zu ]\n",
            ptr_kstr, ptr_kstr->capacity, ptr_kstr->size );
        return -1;   
    }
    return 0;
}


struct kstring kstring_init( void ) {
    struct kstring kstr = {0};
    kstr.data = kmalloc( sizeof( char ) * MIN_KERN_BUF_CAP, GFP_KERNEL );
    kstr.capacity = MIN_KERN_BUF_CAP;
    kstr.size = 0;
    return kstr;
}
EXPORT_SYMBOL( kstring_init );


size_t kstring_write( struct kstring* ptr_kstr, const char* src ) {
    size_t src_sz = strlen( src );
    printk( KERN_INFO MOD_NAME ": kstring_write: starting write from \" %s \" to %p ]\n", src, ptr_kstr );

    if ( kstring_is_full( ptr_kstr, src_sz ) )
        if ( kstring_grow( ptr_kstr, src_sz ) == -1 )
            return ptr_kstr->size = 0;

    ptr_kstr->size += snprintf( ptr_kstr->data + ptr_kstr->size, sizeof( char ) * ( ptr_kstr->capacity - ptr_kstr->size + 1 ), src );
    printk( KERN_INFO MOD_NAME ": kstring_write: [ count: %zu, capacity: %zu ]\n", ptr_kstr->size, ptr_kstr->capacity );
    return ptr_kstr->size;
}
EXPORT_SYMBOL( kstring_write );


size_t kstring_write_file( struct kstring* ptr_kstr, struct file* ptr_file ) {
    size_t cnt = 0;
    char* ptr = NULL;

    char* full_path = kmalloc( sizeof( char ) * MAX_PATH_LEN, GFP_KERNEL );
    str_to_str( dentry_path_raw( ptr_file->f_path.dentry, full_path, MAX_PATH_LEN ), &ptr, "f_path.dentry", 13 );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    kfree( full_path );
    if ( !cnt ) return 0;

    ptr = NULL;
    ptr_to_str( ptr_file->f_inode, &ptr, "f_inode", 7 );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    ptr = NULL;
    ptr_to_str( ptr_file->f_op, &ptr, "f_op", 4 );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    ptr = NULL;
    uint32_to_str( &ptr, "\"f_mode\": \"%x\",", 13, ptr_file->f_mode );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    ptr = NULL;
    uint32_to_str( &ptr, "\"f_flags\": \"%x\",", 14, ptr_file->f_flags );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    ptr = NULL;
    uint64_to_str( &ptr, "\"f_pos\": %llu,", 10, ptr_file->f_pos );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    ptr = NULL;
    uint64_to_str( &ptr, "\"f_version\": %llu,", 14, ptr_file->f_version );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    ptr = NULL;
    uint32_to_str( &ptr, "\"f_wb_err\": %u,", 13, ptr_file->f_wb_err );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    ptr = NULL;
    uint32_to_str( &ptr, "\"f_sb_err\": %u", 12, ptr_file->f_sb_err );
    cnt += kstring_write( ptr_kstr, ptr );
    kfree( ptr );
    if ( !cnt ) return 0;

    return cnt;
}
EXPORT_SYMBOL( kstring_write_file );