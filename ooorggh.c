#include <linux/kernel.h>
#include <linux/module.h>
MODULE_LICENSE("GPL"); // required by the compiler

int init_module(void){
    panic("ooorggh");
    return 0;
}
