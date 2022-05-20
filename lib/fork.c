// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
// extern void _pgfault_upcall(void);
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
	if(((utf->utf_err & FEC_WR )!= FEC_WR) || ((uvpt[PGNUM(utf->utf_fault_va)] & PTE_COW) != PTE_COW)){
		panic("pgfault function fork.c line 29");
	}
	int success_page_alloc = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W);
	if(success_page_alloc != 0){
		panic("pgfault function fork.c line 33");
	}
	memcpy(PFTEMP,(void *)PTE_ADDR(utf->utf_fault_va),PGSIZE);
	int success_map = sys_page_map(0, PFTEMP, 0, (void *)PTE_ADDR(utf->utf_fault_va), PTE_P | PTE_U | PTE_W);
	if(success_map != 0){
		panic("pgfault function fork.c line 37");
	}
	int success_unmap = sys_page_unmap(0, PFTEMP);
	if(success_unmap != 0){
		panic("pgfault function fork.c line 41");
	}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	// panic("duppage not implemented");
	// return 0;
	
	if(((uvpt[pn] & PTE_W) == PTE_W) || ((uvpt[pn] & PTE_COW) == PTE_COW)){
		int success_map_child = sys_page_map(0,(void *)( pn*PGSIZE), envid, (void *)(pn*PGSIZE), PTE_P| PTE_U | PTE_COW);
		if(success_map_child != 0){
			return success_map_child;
		}
		int success_map_current = sys_page_map(0,(void *) (pn*PGSIZE), 0, (void *)(pn*PGSIZE), PTE_P| PTE_U | PTE_COW);
		if(success_map_current != 0){
			return success_map_current;
		}
	} else{
		int success_map = sys_page_map(0,(void *) (pn*PGSIZE), envid,(void *) (pn*PGSIZE), PTE_P| PTE_U);
		if(success_map != 0){
			return success_map;
		}
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
	// panic("fork not implemented");
	set_pgfault_handler(pgfault);

	envid_t envid = sys_exofork();
	if (envid < 0)
		return envid;
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	uint32_t address;
	for (address = 0; address < USTACKTOP; address += PGSIZE){
		if(((uvpd[PDX(address)] & PTE_P) == PTE_P )&& ((uvpt[PGNUM(address)] & PTE_P) == PTE_P)){
			duppage(envid, PGNUM(address));
		}
	}

	void _pgfault_upcall();
	int success_page_alloc =  sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W);
	if(success_page_alloc != 0){
		return success_page_alloc;
	}

	int success_pagefault = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
	if(success_pagefault != 0){
		return success_pagefault;
	}

	int success_status = sys_env_set_status(envid, ENV_RUNNABLE);
	if(success_status != 0){
		return success_status;
	}

	return envid;
	
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
