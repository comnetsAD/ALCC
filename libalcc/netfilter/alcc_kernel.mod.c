#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0xc79d2779, "module_layout" },
	{ 0x37a0cba, "kfree" },
	{ 0x2e54a292, "nf_unregister_net_hook" },
	{ 0x70dd3f80, "netlink_kernel_release" },
	{ 0xd06a7527, "nf_register_net_hook" },
	{ 0xba730215, "__netlink_kernel_create" },
	{ 0xc5ee075, "init_net" },
	{ 0x26c2e0b5, "kmem_cache_alloc_trace" },
	{ 0x8537dfba, "kmalloc_caches" },
	{ 0xc5850110, "printk" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xfc5faae8, "netlink_broadcast" },
	{ 0xa916b694, "strnlen" },
	{ 0x8ab0781f, "__nlmsg_put" },
	{ 0xbf8a9ea3, "__alloc_skb" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "1FAB3DFE31C8670D0256BD4");
