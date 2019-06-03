// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

extern volatile pte_t uvpt[];     // VA of "virtual page table"
extern volatile pde_t uvpd[];     // VA of current page directory
extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);
	if((FEC_WR & err) && (uvpt[(uintptr_t)addr / PGSIZE] & PTE_COW)){
		sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W);
		memmove(PFTEMP, addr, PGSIZE);
		sys_page_map(0, PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W);
		sys_page_unmap(0, PFTEMP);
	}else{
		panic("page fault on not cow");
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)//因为如果已经是ｃｏｗ的话，说明父进程的父进程也共享同一副本，我们需要写一下这个副本，使得父进程
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	void* addr = (void*)(pn * PGSIZE);
	if(pn == (UXSTACKTOP/PGSIZE - 1)){
		sys_page_alloc(envid, (void*)(UXSTACKTOP - PGSIZE), PTE_P|PTE_W|PTE_U);
	}
	else if((uvpt[pn] & PTE_P) == 0 || (uvpt[pn] & PTE_U) == 0){
		return 0;
	}
	else if (uvpt[pn] & (PTE_W | PTE_COW)){
		sys_page_map(0, addr, envid, addr, PTE_U|PTE_P|PTE_COW);
		sys_page_map(0, addr, 0, addr, PTE_U|PTE_P|PTE_COW);
	}
	else {
		sys_page_map(0, addr, envid, addr, PTE_U|PTE_P);
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();
	if (envid < 0) {
		panic("sys_exofork: %e", envid);
	}
	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	for(int i = 0; i<UTOP/PGSIZE; i++){
		if(uvpd[i/NPTENTRIES] & PTE_P){
			duppage(envid, i);
		}
	}
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	sys_env_set_status(envid, ENV_RUNNABLE);
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
