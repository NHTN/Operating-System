#include <asm/unistd.h>
#include <asm/cacheflush.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <asm/pgtable_types.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <linux/kallsyms.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ThaoNhi_AnhThu");

static void  **system_call_table_addr;

asmlinkage long (*original_openat) (int dirfd, const char* name, int flags);
asmlinkage long (*original_write) (unsigned int fd, const char __user *buf, size_t count);

// FUNCTION HOOK SYSCALL OPENAT
asmlinkage long hook_openat (int dirfd, const char* name, int flags) 
{
    long result = original_openat(dirfd, name, flags);
   
    printk(KERN_INFO "HOOK SYSCALL OPEN\n");
    printk(KERN_INFO "CALLING PROCESS (OPEN): %s\n", current->comm);
	
	int len = strlen(name);
	char* kname = (char*)kmalloc(len + 1, GFP_KERNEL);
	kname[len] = '\0';
	copy_from_user(kname, name, len);
    printk(KERN_INFO "OPENNING FILE: %s\n", name);
    kfree(kname);

    return result;
}

// FUNCTION HOOK SYSCALL WRITE
asmlinkage long hook_write(unsigned int fd, const char __user *buf, size_t count)
{
	long byte_count = original_write(fd, buf, count);

	printk(KERN_INFO "HOOK SYSCALL WRITE\n");
	printk(KERN_INFO "CALLING PROCESS (WRITE): %s\n", current->comm);
	printk(KERN_INFO "BYTE WRITTEN: %ld\n", byte_count);
	
	return byte_count;
}

// FUNCTION MAKE PAGE WRITABLE
void make_rw(void)
{
    write_cr0 (read_cr0() & (~ 0x10000));
}

//FUNCTION MAKE PAGE PROTECTED
void make_ro(void)
{
    write_cr0 (read_cr0() | 0x10000);
}

// FUNCTION WIIL EXECUTE WHEN INSERTING KERNEL MODULE
static int __init entry_point(void)
{
    printk(KERN_INFO "MODULE LOADED SUCCESSFULLY\n");
    system_call_table_addr = (void*)kallsyms_lookup_name("sys_call_table");

    original_openat = system_call_table_addr[__NR_openat];
    original_write = system_call_table_addr[__NR_write];

    __flush_tlb();

    make_rw();
    system_call_table_addr[__NR_openat] = hook_openat;
    system_call_table_addr[__NR_write] = hook_write;
    make_ro();

    return 0;
}

// FUNCTION WILL EXECUTE WHEN REMOVING KERNEL MODULE
static void __exit exit_point(void){

    __flush_tlb();

    make_rw();
    system_call_table_addr[__NR_openat] = original_openat;
    system_call_table_addr[__NR_write] = original_write;
    make_ro();

    printk(KERN_INFO "MODULE UNLOADED SUCCESSFULLY\n");
}

module_init(entry_point);
module_exit(exit_point);
