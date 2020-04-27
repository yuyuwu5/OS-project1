#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/timer.h>

// syscall 333

asmlinkage long time_pj1(){
		static const long BASE = 1000000000;
		struct timespec t;
		getnstimeofday(&t);
		return t.tv_sec * BASE + t.tv_nsec;
}	
