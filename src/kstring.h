#ifndef _KSTRING_H_
#define _KSTRING_H_

#include <linux/slab.h> /* essential for kmalloc, kfree */
#include <linux/fdtable.h> /* essential for files_struct */

#include "formatters.h"

#define MIN_KERN_BUF_CAP 16
#define MAX_PATH_LEN 4096

struct kstring {
    size_t capacity;
    size_t size;
    char* data;
};

void kstring_free( struct kstring* ptr_kstr );

struct kstring kstring_init( void );

size_t kstring_write( struct kstring* ptr_kstr, const char* src );

size_t kstring_write_file( struct kstring* ptr_kstr, struct file* ptr_file );

#endif /* _KSTRING_H_ */