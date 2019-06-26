// User-level page fault handler support.
// Rather than register the C page fault handler directly with the
// kernel as the page fault handler, we register the assembly language
// wrapper in pfentry.S, which in turns calls the registered C
// function.

#include <inc/lib.h>


// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _pgfault_upcall(void);

// Pointer to currently installed C-language pgfault handler.
void (*_pgfault_handler)(struct UTrapframe *utf);

//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _pgfault_upcall routine when a page fault occurs.
//
void
set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
{
	int r;

	if (_pgfault_handler == 0) {
		// First time through!
		// LAB 4: Your code here.
		envid_t envid = sys_getenvid();
		sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_U|PTE_P|PTE_W);
		// 这里把_pgfault_upcall的虚拟地址传给了env，所以env肯定会从pfentry中对应的_pgfault_upcall的位置处运行
		sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	}

	// Save handler pointer for assembly to call.
	// 修改_pgfault_handler这一变量对应指向的虚拟内存，从而可以在既有框架下运行自定义的handler
	_pgfault_handler = handler;
}
