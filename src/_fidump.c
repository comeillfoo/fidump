#include <linux/module.h> /* essential for modules */
#include <linux/kernel.h> /* essential for KERNEL_INFO */

#include <linux/sched.h> /* essential for task_struct */
#include <linux/sched/signal.h> /* essential for_each_process */
#include <linux/proc_fs.h> /* essential for procfs */
#include <linux/slab.h> /* essential for kmalloc, kfree */
#include <linux/fdtable.h> /* essential for files_struct */
#include <linux/pid.h> /* essential for get_task_pid, find_get_pid */
#include <linux/mutex.h> /* essential for mutex */
// #include <linux/rwlock_types.h> /* essential for rwlock_t */

MODULE_LICENSE( "Dual MIT/GPL" );

// -- constants
#define ROOT_PROC_NAME "fidump"
// -- constants

#include "kstring.h"

// static DEFINE_MUTEX( fidump_mutex ); // -- for the sake of multithreading

// static rwlock_t fidump_rw_lock = __RW_LOCK_UNLOCKED( fidump_rw_lock ); // -- for the sake of multithreading
// static unsigned long fidump_rw_lock_flags;

// -- for proc dir entry
static int proc_fidump_open( struct inode* ptr_inode, struct file* ptr_file ) {
    // if ( !mutex_trylock( &fidump_mutex ) ) {
    //     printk( KERN_ALERT MOD_NAME ": proc_fidump_open: lock not captured\n" );
    //     return -EBUSY;
    // }
    return 0;
}

static int proc_fidump_release( struct inode* ptr_inode, struct file* ptr_file ) {
    // mutex_unlock( &fidump_mutex );
    return 0;
}

static ssize_t proc_fidump_read( struct file *ptr_file, char __user * usr_buf, size_t length, loff_t * ptr_pos );
static ssize_t proc_fidump_write( struct file *ptr_file, const char __user * usr_buf, size_t length, loff_t * ptr_pos );


static const struct proc_ops  proc_fidump_ops = {
    .proc_open = proc_fidump_open,
    .proc_read = proc_fidump_read,
    .proc_write = proc_fidump_write,
    .proc_release = proc_fidump_release
};


static struct proc_dir_entry* proc_fidump_root;
// -- for proc dir entry

static int __init init_fidump( void ) {
    printk( KERN_INFO MOD_NAME ": init_fidump: module loaded\n" );
    proc_fidump_root = proc_create( ROOT_PROC_NAME, 0666, NULL, &proc_fidump_ops );
    printk( KERN_INFO MOD_NAME ": init_fidump: created root proc entry " ROOT_PROC_NAME "\n" );
    return 0;
}


static void __exit cleanup_fidump( void ) {
    printk( KERN_INFO MOD_NAME ": cleanup_fidump: module unloaded\n" );
    // mutex_destroy( &fidump_mutex );
    proc_remove( proc_fidump_root );
}


module_init( init_fidump );
module_exit( cleanup_fidump );


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

static const char* error_msg = "{ \"error\": \"parameters not set\" }\n";
static struct kstring com_buf = { .data = NULL, .size = 0, .capacity = 0 };

static ssize_t proc_fidump_read( struct file* ptr_file, char __user * usr_buf, size_t length, loff_t* ptr_pos ) {
    printk( KERN_INFO MOD_NAME ": proc_fidump_read: [ ptr_file: %p ], [ usr_buf: %p ], [ length: %zu ], [ ptr_pos: %p/%llu ]\n", ptr_file, usr_buf, length, ptr_pos, *ptr_pos );
    // read_lock( &fidump_rw_lock );
    if ( *ptr_pos > 0 ) {
        // read_unlock( &fidump_rw_lock );
        return 0;
    }

    if ( com_buf.data == NULL ) {
        printk( KERN_INFO MOD_NAME ": proc_fidump_read: com_buf=[ .capacity=%zu; .size=%zu; .data=\"%s\" ]\n", com_buf.capacity, com_buf.size, com_buf.data );
        com_buf = kstring_init();
        printk( KERN_INFO MOD_NAME ": proc_fidump_read: com_buf=[ .capacity=%zu; .size=%zu; .data=\"%s\" ]\n", com_buf.capacity, com_buf.size, com_buf.data );
        kstring_write( &com_buf, error_msg );
        printk( KERN_INFO MOD_NAME ": proc_fidump_read: com_buf=[ .capacity=%zu; .size=%zu; .data=\"%s\" ]\n", com_buf.capacity, com_buf.size, com_buf.data );
    }

    if ( !copy_to_user( usr_buf, com_buf.data, com_buf.size ) )
        *ptr_pos += com_buf.size;
    else {
        printk( KERN_CRIT MOD_NAME ": proc_fidump_pid_read: can't copy to user buffer\n" );
        if ( com_buf.data != NULL ) kstring_free( &com_buf );
        // read_unlock( &fidump_rw_lock );
        return 0;
    }

    // if ( com_buf.data == NULL ) {
    //     printk( KERN_CRIT MOD_NAME ": proc_fidump_pid_read: can't free the kernel buffer 'cause of corrupted pointer\n" );
    //     return 0;
    // } else kstring_free( &com_buf );
    // read_unlock( &fidump_rw_lock );
    return com_buf.size;
}

static size_t get_right_bound( const char __user* usr_buf, size_t length, const size_t left, const char term ) {
    size_t right = left;
    while ( usr_buf[ right ] != term && right < length ) right++;
    return right - 1;
}


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


static ssize_t proc_fidump_write( struct file* ptr_file, const char __user* usr_buf, size_t length, loff_t* ptr_pos ) {
    unsigned int pid = 1;
    unsigned int fd = 0;
    ssize_t count = 0;
    // write_lock( &fidump_rw_lock );
    printk( KERN_INFO MOD_NAME ": proc_fidump_write: writing started\n" );
    printk( KERN_INFO MOD_NAME ": proc_fidump_write: [ ptr_file: %p ], [ usr_buf: %p ], [ length: %zu ], [ ptr_pos: %p/%llu ]\n", ptr_file, usr_buf, length, ptr_pos, *ptr_pos );
    if ( *ptr_pos > 0 ) {
        // write_unlock( &fidump_rw_lock );
        return -EINVAL;
    }
    count += get_params( usr_buf, length, &pid, &fd ) + 1;
    *ptr_pos += count;

    if ( com_buf.data != NULL )
        kstring_free( &com_buf );

    fill_the_entry( pid, fd, &com_buf );

    printk( KERN_INFO MOD_NAME ": proc_fidump_write: writing finished; position is %llu\n", *ptr_pos );
    // write_unlock( &fidump_rw_lock );
    return count;
}