#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


<<<<<<< HEAD
MODULE_INFO(srcversion, "884209F7A8CFA9A59908D74");
=======
MODULE_INFO(srcversion, "B6D8525419AB4E9618AC2CA");
>>>>>>> 864b13bc5a26dc925aa24ec3dfb7c6df3a29cf8e
