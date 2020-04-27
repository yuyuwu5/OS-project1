#include <linux/linkage.h>
#include <linux/kernel.h>

//syscall 334

asmlinkage void sys_prink_pj1(int pid, long start, long end) {
	static const long BASE = 1000000000;
	printk("[Project1] %d %ld.%09ld %ld.%09ld", pid, start/BASE, start%BASE, end/BASE, end%BASE);
}
