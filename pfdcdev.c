#include <linux/module.h> /* essential for modules */
#include <linux/kernel.h> /* essential for KERNEL_INFO */

#include <linux/sched.h> /* essential for task_struct */
#include <linux/sched/signal.h> /* essential for_each_process */
#include <linux/proc_fs.h> /* essential for procfs */
#include <linux/slab.h> /* essential for kmalloc, kfree */
#include <linux/fdtable.h> /* essential for files_struct */
#include <linux/pid.h> /* essential for get_task_pid, find_get_pid */
#include <linux/mutex.h> /* essential for mutex */

MODULE_LICENSE( "Dual MIT/GPL" );

#define MOD_NAME "pfdcdev"
#define ROOT_PROC_NAME "pfdcdev"
#define MIN_KERN_BUF_CAP 16
#define MAX_PATH_LEN 4096
static DEFINE_MUTEX( pfdcdev_mutex );


static int proc_pfdcdev_open( struct inode* ptr_inode, struct file* ptr_file ) {
    if ( !mutex_trylock( &pfdcdev_mutex ) ) {
        printk( KERN_ALERT MOD_NAME ": proc_pfdcdev_open: lock not captured\n" );
        return -EBUSY;
    }
    return 0;
}

static int proc_pfdcdev_release( struct inode* ptr_inode, struct file* ptr_file ) {
    mutex_unlock( &pfdcdev_mutex );
    return 0;
}

static ssize_t proc_pfdcdev_read( struct file *ptr_file, char __user * usr_buf, size_t length, loff_t * ptr_pos );
static ssize_t proc_pfdcdev_write( struct file *ptr_file, const char __user * usr_buf, size_t length, loff_t * ptr_pos );


static const struct proc_ops  proc_pfdcdev_ops = {
    .proc_open = proc_pfdcdev_open,
    .proc_read = proc_pfdcdev_read,
    .proc_write = proc_pfdcdev_write,
    .proc_release = proc_pfdcdev_release
};


static struct proc_dir_entry* proc_pfdcdev_root;


static int __init init_pfdcdev( void ) {
    printk( KERN_INFO MOD_NAME ": init_pfdcdev: module loaded\n" );
    proc_pfdcdev_root = proc_create( ROOT_PROC_NAME, 0666, NULL, &proc_pfdcdev_ops );
    printk( KERN_INFO MOD_NAME ": init_pfdcdev: created root proc entry " ROOT_PROC_NAME "\n" );
    return 0;
}


static void __exit cleanup_pfdcdev( void ) {
    printk( KERN_INFO MOD_NAME ": cleanup_pfdcdev: module unloaded\n" );
    mutex_destroy( &pfdcdev_mutex );
    proc_remove( proc_pfdcdev_root );
}


module_init( init_pfdcdev );
module_exit( cleanup_pfdcdev );


static size_t __do_get_length( u64 n ) {
    if ( n == 0 )
        return 0;
    else return __do_get_length( n / 10 ) + 1;
}


static size_t get_length( u64 n ) {
    const size_t length = __do_get_length( n );
    return ( length == 0 )? 1 : length;
} 


static size_t uint32_to_str( char** dest, const char* fmt, size_t fmt_len, unsigned int num ) {
    size_t dest_sz = 0;
    size_t len = 0;

    len = get_length( num );
    
    *dest = kmalloc( sizeof( char ) * ( len + fmt_len ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": uint32_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": uint32_to_str: [ dest: %p, fmt: \"%s\", fmt_len: %zu, num: %u ]\n", dest, fmt, fmt_len, num );
        return 0;
    }
    
    dest_sz = sprintf( *dest, fmt, num );
    return dest_sz;
}


static size_t uint64_to_str( char** dest, const char* fmt, size_t fmt_len, u64 num ) {
    size_t dest_sz = 0;
    size_t len = 0;

    len = get_length( num );
    
    *dest = kmalloc( sizeof( char ) * ( len + fmt_len ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": uint64_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": uint64_to_str: [ dest: %p, fmt: \"%s\", fmt_len: %zu, num: %llu ]\n", dest, fmt, fmt_len, num );
        return 0;
    }
    
    dest_sz = sprintf( *dest, fmt, num );
    return dest_sz;
}


static size_t ptr_to_str( void const * const ptr, char** dest, const char* key, size_t key_len ) {
    const char* fmt = "\"%s\": \"%016p\",";
    size_t fmt_len = 7;
    size_t dest_sz = 0;
    size_t len = 16;

    *dest = kmalloc( sizeof( char ) * ( len + fmt_len + key_len ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: [ dest: %p, ptr: %p ]\n", dest, ptr );
        return 0;
    }
    
    dest_sz = sprintf( *dest, fmt, key, ptr );
    return dest_sz;
}


static size_t str_to_str( const char* str, char** dest, const char* key, size_t key_len ) {
    const char* fmt = "\"%s\": \"%s\",";
    size_t fmt_len = 7;
    size_t dest_sz = 0;
    size_t len = strlen( str );

    *dest = kmalloc( sizeof( char ) * ( len + fmt_len + key_len ), GFP_KERNEL );
    if ( *dest == NULL ) {
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: can't allocate memory for the buffer\n" );
        printk( KERN_CRIT MOD_NAME ": ptr_to_str: [ dest: %p, str: \"%s\" ]\n", dest, str );
        return 0;
    }
    
    dest_sz = sprintf( *dest, fmt, key, str );
    return dest_sz;
}


static size_t fd_to_str( unsigned int fd, char** fd_buf ) {
    return uint32_to_str( fd_buf, "\"%u\": {", 5, fd );
}


struct kstring {
    size_t capacity;
    size_t size;
    char* data;
};


static void kstring_free( struct kstring* ptr_kstr ) {
    kfree( ptr_kstr->data );
    ptr_kstr->data = NULL;
    ptr_kstr->size = 0;
    ptr_kstr->capacity = 0;
}

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


static struct kstring kstring_init(void) {
    struct kstring kstr = {0};
    kstr.data = kmalloc( sizeof( char ) * MIN_KERN_BUF_CAP, GFP_KERNEL );
    kstr.capacity = MIN_KERN_BUF_CAP;
    kstr.size = 0;
    return kstr;
}


static size_t kstring_write( struct kstring* ptr_kstr, const char* src ) {
    size_t src_sz = strlen( src );
    printk( KERN_INFO MOD_NAME ": kstring_write: starting write from \" %s \" to %p ]\n", src, ptr_kstr );

    if ( kstring_is_full( ptr_kstr, src_sz ) )
        if ( kstring_grow( ptr_kstr, src_sz ) == -1 )
            return ptr_kstr->size = 0;

    ptr_kstr->size += sprintf( ptr_kstr->data + ptr_kstr->size, src );
    printk( KERN_INFO MOD_NAME ": kstring_write: [ count: %zu, capacity: %zu ]\n", ptr_kstr->size, ptr_kstr->capacity );
    return ptr_kstr->size;
}

static size_t kstring_write_file( struct kstring* ptr_kstr, struct file* ptr_file ) {
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


struct param_n_kstring {
    unsigned int fd;
    struct kstring* ptr_kstring; 
};


static int fd_itr_callback_fn( const void * ptr, struct file* ptr_file, unsigned int fd ) {
    struct param_n_kstring* ptr_pnkstr = ( struct param_n_kstring* ) ptr;
    unsigned int need_fd = ptr_pnkstr->fd;
    struct kstring* ptr_kern_buf = ptr_pnkstr->ptr_kstring;
    if ( need_fd == fd ) {
        size_t kstr_written_cnt = 0;
        char* fd_str = NULL;
        printk( KERN_INFO MOD_NAME ": fd_itr_callback_fn: needed fd found [ %u == %u ]", need_fd, fd );

        // read start
        fd_to_str( fd, &fd_str );
        kstr_written_cnt = kstring_write( ptr_kern_buf, fd_str );
        kfree( fd_str );
        if ( !kstr_written_cnt ) return ~0;
        // read finish

        // read start
        if ( !kstring_write_file( ptr_kern_buf, ptr_file ) ) return ~0;
        // read finish

        // read start
        if ( !kstring_write( ptr_kern_buf, "}" ) ) return ~0;
        // read finish

        return ~0;
    } else return 0;
}


static void fill_the_entry( int pid, unsigned int fd, struct kstring* ptr_kern_buf ) {
    struct task_struct* ptr_task_itr = get_pid_task( find_get_pid( pid ), PIDTYPE_PID );
    struct param_n_kstring params = { .fd = fd, .ptr_kstring = ptr_kern_buf };
    *ptr_kern_buf = kstring_init();
    printk( KERN_INFO MOD_NAME ": fill_the_entry: pid=%u; fd=%u\n", pid, fd );
    
    // read start
    if ( !kstring_write( ptr_kern_buf, "{" ) ) return;
    // read finish
    if ( ptr_task_itr != NULL )
        iterate_fd( ptr_task_itr->files, 0, fd_itr_callback_fn, &params );

    // read start
    if ( !kstring_write( ptr_kern_buf, "}\n\0" ) ) return;
    // read finish
}

static const char* error_msg = "{ error: \"parameters not set\" }\n";
static struct kstring com_buf = { .data = NULL, .size = 0, .capacity = 0 };

static ssize_t proc_pfdcdev_read( struct file* ptr_file, char __user * usr_buf, size_t length, loff_t* ptr_pos ) {
    printk( KERN_INFO MOD_NAME ": proc_pfdcdev_read: [ ptr_file: %p ], [ usr_buf: %p ], [ length: %zu ], [ ptr_pos: %p/%llu ]\n", ptr_file, usr_buf, length, ptr_pos, *ptr_pos );

    if ( *ptr_pos > 0 ) return 0;

    if ( com_buf.data == NULL ) {
        printk( KERN_INFO MOD_NAME ": proc_pfdcdev_read: com_buf=[ .capacity=%zu; .size=%zu; .data=\"%s\" ]\n", com_buf.capacity, com_buf.size, com_buf.data );
        com_buf = kstring_init();
        printk( KERN_INFO MOD_NAME ": proc_pfdcdev_read: com_buf=[ .capacity=%zu; .size=%zu; .data=\"%s\" ]\n", com_buf.capacity, com_buf.size, com_buf.data );
        kstring_write( &com_buf, error_msg );
        printk( KERN_INFO MOD_NAME ": proc_pfdcdev_read: com_buf=[ .capacity=%zu; .size=%zu; .data=\"%s\" ]\n", com_buf.capacity, com_buf.size, com_buf.data );
    }

    if ( !copy_to_user( usr_buf, com_buf.data, com_buf.size ) )
        *ptr_pos += com_buf.size;
    else {
        printk( KERN_CRIT MOD_NAME ": proc_pfdcdev_pid_read: can't copy to user buffer\n" );
        if ( com_buf.data != NULL ) kstring_free( &com_buf );
        return 0;
    }

    // if ( com_buf.data == NULL ) {
    //     printk( KERN_CRIT MOD_NAME ": proc_pfdcdev_pid_read: can't free the kernel buffer 'cause of corrupted pointer\n" );
    //     return 0;
    // } else kstring_free( &com_buf );

    return com_buf.size;
}

static size_t get_right_bound( const char __user* usr_buf, size_t length, const size_t left, const char term ) {
    size_t right = left;
    while ( usr_buf[ right ] != term && right < length ) right++;
    return right - 1;
}
// 012345678
// 2050 121 
static size_t extract_param( const char __user* usr_buf, size_t length, const size_t start, const char term, char** param ) {
    const size_t finish = get_right_bound( usr_buf, length, start, term );
    const size_t count = finish - start + 1;
    printk( KERN_INFO MOD_NAME ": extract_param: start is %zu, finish is %zu and count is %zu\n", start, finish, count );
    *param = kmalloc( sizeof( char ) * ( count + 1 ), GFP_KERNEL );
    *param = memcpy( *param, usr_buf + start, count + 1 );
    (*param)[ count ] = 0;
    printk( KERN_INFO MOD_NAME ": extract_param: param is %p, another param is %p\n", *param, param );
    return finish; 
}

static size_t get_param( const char __user* usr_buf, size_t length, size_t start, unsigned int* param ) {
    char* raw_param = NULL;
    size_t raw_param_end = extract_param( usr_buf, length, start, ' ', &raw_param );
    printk( KERN_INFO MOD_NAME ": get_param: [ raw_param=%s ]\n", raw_param );
    printk( KERN_INFO MOD_NAME ": get_param: kstrtouint error = %d\n", kstrtouint( raw_param, 10, param ) );
    printk( KERN_INFO MOD_NAME ": get_param: [ param=%u ]\n", *param );
    kfree( raw_param );
    return raw_param_end;
}

static ssize_t get_params( const char __user* usr_buf, size_t length,  unsigned int* pid, unsigned int* fd ) {
    size_t raw_pid_end = get_param( usr_buf, length, 0, pid );
    size_t raw_fd_end = get_param( usr_buf, length, raw_pid_end + 2, fd );
    printk( KERN_INFO MOD_NAME ": get_params: [ pid = %.*s ]\n", ( int ) raw_pid_end + 1, usr_buf );
    printk( KERN_INFO MOD_NAME ": get_params: [ fd = %.*s ]\n", ( int ) ( raw_fd_end - raw_pid_end - 1 ), usr_buf + raw_pid_end + 2 );
    return raw_fd_end + 2;
}

static ssize_t proc_pfdcdev_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_pos ) {
    unsigned int pid = 1;
    unsigned int fd = 0;
    ssize_t count = 0;
    printk( KERN_INFO MOD_NAME ": proc_pfdcdev_write: writing started\n" );
    printk( KERN_INFO MOD_NAME ": proc_pfdcdev_write: [ ptr_file: %p ], [ usr_buf: %p ], [ length: %zu ], [ ptr_pos: %p/%llu ]\n", ptr_file, usr_buf, length, ptr_pos, *ptr_pos );
    if ( *ptr_pos > 0 ) return -EINVAL;
    count += get_params( usr_buf, length, &pid, &fd ) + 1;
    *ptr_pos += count;

    if ( com_buf.data != NULL )
        kstring_free( &com_buf );

    fill_the_entry( pid, fd, &com_buf );

    printk( KERN_INFO MOD_NAME ": proc_pfdcdev_write: writing finished; position is %llu\n", *ptr_pos );
    return count;
}