% How paging works with kernel
% Erik Helin
% January 25, 2012

# Paging and the kernel

This document will describe how paging affects the kernel and how the issues 
can be solved.

## What is paging?

Paging is the technique used in x86 to enable _virtual memory_. Virtual memory
means that the each process will get the impression that the available memory 
range is `0x00000000` - `0xFFFFFFFF` even though the actual size of the memory 
is way less. When a process addresses a byte of memory, they will use a
_virtual address_ instead of physical one (although the code in the user
process won't notice any difference). The virtual address will then get
translated to a physical address by the MMU and the page table. If the virtual
address isn't mapped to a physical address, the CPU will raise a page fault
interrupt.

## How does this affect the kernel?

Not at all, is the first answer, which is also correct. However, if one wants
to have processes operating at different privilege level (PL) than the kernel,
it does. A process operating at PL 3 will hereafter be called a user mode
process. In order to protect the user mode processes from writing and reading
other user mode processes (or the kernels) memory, each user process will map
the virtual address space to a different physical piece of memory.

## What does user mode processes has to with the kernel?

The way a user mode process communicates with the kernel is by performing a
`trap`, by issuing the `INT 0xAE` assembly instruction. When the CPU executes 
`INT 0xAE`, it will load the address stored in register `IDTR`, which contains
the address to the interrupt vector table. Since paging is enabled, reading
this address will cause page fault, _if_ the kernel isn't added to the user
mode process table. Hence, the kernel must reside at some virtual address
in each user mode process.

## Does it matter at which virtual address the kernel is placed?

Yes, it does. If the kernel is placed at the beginning of the virtual address,
that is, the virtual address `0x00000000` - `"size of kernel"` will map to the
location of the kernel in memory, there will be issues when linking the user
mode process code. Normally, during linking, the linker assumes that the code
will be loaded into the memory position `0x00000000`. Therefore, when resolving
indirect references, `0x00000000` will be base address for calculating the
exact position. But if the kernel is mapped onto the virtual address space
(`0x00000000`, `"size of kernel"`), the user mode process will be loaded at
virtual memory address `size of kernel` (probably a little bit after due to
alignment). Therefore, the assumption from the linker that the user mode
process is loaded into memory at position `0x00000000` is wrong. This can be
corrected by using a linker script which tells the linker to assume a different
starting address, but that is a very cumbersome solution for
the users of the operating system.

## At which virtual address should the kernel then be placed?

Preferably, the kernel should be placed at a very high virtual memory address,
for example `0xC000000` (roughly 3 GB).
The user mode process is not likely to be 3 GB large, which is the now the only
way that it can conflict with the kernel.

## Is placing the kernel at `0xC000000` hard?
No, but it does require some thought. This is once again a linking problem.
When the linker resolves all indirect references, it will assume that our
kernel is loaded at physical memory location `0x00100000`, not `0x00000000`,
since we tell it so. However, we want the jumps to be resolved to `0xC0100000`
as base, since otherwise a kernel jump will jumps straight into the user mode
process code (remember that the user mode process is loaded at virtual memory
0x00000000). But, we can't simply tell the linker to assume that the kernel
starts at `0xC01000000`, since we want it to be placed at 1 MB. The
reason for having the kernel loaded at 1 MB is because it can't be loaded at
`0x00000000`, since there is BIOS code and GRUB code loaded below 1 MB. This
can be done by using both relocation (`.=0xC01000000`) and the `AT` instruction
in the linker script.

## Now everything is cool, right?
Not yet! When GRUB jumps to the kernel code, there is no paging table.
Therefore, all references to `0xC0100000 + X` won't be mapped to the correct
physical address, and will therefore cause a general protection exception (GPE)
at the very best, otherwise the OS will just crash.

Therefore, some assembly that doesn't use indirect jumps must set up the page
table and adding one entry for the first 4 MB of the virtual address space 
that maps to the first 4 MB of the physical memory as well as an entry for
`0xC0100000` that maps to `0x0010000`. If the identity mapping for the first 4
MB wasn't added, the CPU would case a page fault when fetching the next
instruction from memory. Then, when the table is created, an indirect jump can
be done to a label, like

    lea ebx, [higher_half] ; load the address of the label in ebx
    jmp ebx                ; jump to the label

Now `EIP` will point to a memory location somewhere right after `0xC0100000`
and the entry mapping the first 4 MB of virtual memory to the first 4 MB of
physical can now be removed from the page table.

## Phew, that was all?
Yes, that was all. However, one must now take care when using memory mapped I/O
that uses very specific memory location. For example, the framebuffer is
located `0x000B8000`, but since there is no entry in the page table for the
address `0x000B8000`, the address `0xC00B8000` must be used, since the address
`0xC0000000` maps to `0x00000000`.
