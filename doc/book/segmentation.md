# Segmentation

## Virtual memory

In the x86 architecture, virtual memory can be accomplished in two ways:
Segmentation and paging. Paging is by far the most common and versatile
technique, and we'll implement it in the next chapter. Some use of segmentation
is still necessary (to allow for code to execute under different privilege
levels), so we'll use a minimal segmentation setup.

It might be interesting to note that in x86\_64, segmentation is almost
completely removed.

Segmentation and paging is described in [@intel3a], chapter 3.

## Segmentation

Segmentation in x86 means accessing the memory through segments. Segments are
portions of the address space, possibly overlapping, specified by a base
address and a limit. To address a byte in segmented memory, you use a 48-bit
**logical address**: 16 bits that specifies the segment, and 32-bits to specify
what offset within that segment you want. The offset is added to the base
address of the segment, and the resulting linear address is checked against the
segment's limit. If everything checks out fine, (including access-right-checks
ignored for now), this results in a **linear address**. If paging is disabled,
this linear address space is mapped 1:1 on the **physical address** space, and
the physical memory can be accessed.

![Figure: Translation of logical addresses to linear addresses.
](images/intel_3_5_logical_to_linear.png)

To enable segmentation you need to set up a table that describes each segment -
a segment descriptor table. In x86, there are two types of descriptor tables:
The global descriptor table (GDT) and local descriptor tables (LDT). An LDT is
set up and managed by user-space processes, and each process have their own LDT.
LDT's can be used if a more complex segmentation model is desired - we won't
use it. The GDT shared by everyone - it's global.

### Accessing memory

Most of the time when we access memory we don't have to explicitly specify the
segment we want to use. The processor has six 16-bit segment registers: `cs`,
`ss`, `ds`, `es`, `gs` and  `fs`. `cs` is the code segment register and
specifies the segment to use when fetching instructions. `ss` is used whenever
the accessing the stack (through the stack pointer `esp`), and `ds` is used for
other data accesses. `es`, `gs` and `fs` are free to use however we or the user
processes wish (we will set them, but not use them).

Implicit use of segment registers:

    func:
        mov eax, [esp+4]
        mov ebx, [eax]
        add ebx, 8
        mov [eax], ebx
        ret

Explicit version:

    func:
        mov eax, [ss:esp+4]
        mov ebx, [ds:eax]
        add ebx, 8
        mov [ds:eax], ebx
        ret

(You don't need to use `ss` when accessing the stack, or `ds` when accessing
other memory. But it is convenient, and makes it possible to use the implicit
style above.)

[Figure 3-8, describing a segment descriptor]

### The Global Descriptor Table (GDT)

A GDT/LDT is an array of 8-byte segment descriptors. The first descriptor in
the GDT is always a null descriptor, and can never be used to access memory. We
need at least two segment descriptors (plus the null descriptor) for our GDT
(and two more later when we enter user mode, see ??????????????). This is
because the descriptor contains more information than just the base and limit
fields. The two most relevant fields here are the Type field and the Descriptor
Privilege Level (DPL) field.

The table 3-1, chapter 3, in [@intel3a] specifies the values for the Type field,
and it is because of this that we need at least two descriptors: One to execute
code (to put in `cs`) (Execute-only or Execute-Read) and one to read and write
data (Read/Write) (to put in the other segment registers).

The DPL specifies the privilege levels required to execute in this segment.
Since we are now executing in kernel mode (PL0), the DPL should be 0.

Segment descriptors needed:

  index   offset   name                 address range             type   DPL
 ------  -------   -------------------  ------------------------- -----  ----
      0   `0x00`   null descriptor
      1   `0x08`   kernel code segment  `0x00000000 - 0xFFFFFFFF` RX     PL0
      2   `0x10`   kernel data segment  `0x00000000 - 0xFFFFFFFF` RW     PL0

Note that the segments overlap - they both encompass the entire linear address
space. In our minimal setup we'll only use segmentation to get privilege levels.
See the [@intel3a], chapter 3, for details on the other descriptor fields.

### Creating and loading the GDT

Creating the GDT can easily be done both in C and assembler. A static
fixed-size array should do the trick.

To load the GDT into the processor we use the `lgdt` instruction, which takes
the address of a struct that specifies the start and size of the GDT:

    32-bit: start of GDT
    16-bit: size of GDT (8 bytes * num entries)

We have three entries.

If `eax` has an address to such a struct, we can just to the following:

    lgdt [eax]

Now that the processor knows where to look for the segment descriptors we need
to load the segment registers. Segment selectors look like this:

[Figure 3-6]

The offset is added to the start of the GDT to get the address of the segment
descriptor: `0x08` for the first descriptor and `0x10` for the second, since
each descriptor is 8 bytes. The Requested Privilege Level (RPL) should be `0`,
since we want to remain in PL0.

Loading the segment selector registers is easy for the data registers - just
copy the correct offsets into the registers:

    mov ds, 0x10
    mov ss, 0x10
    mov es, 0x10
    ; ...

To load `cs` we have to do a "far jump":

        ; using previous cs
        jmp 0x08:flush_cs

    flush_cs:
        ; now we've changed cs to 0x08

A far jump is a jump where we explicitly specify the full 48-bit logical
address: The segment selector to use, and the absolute address to jump to. It
will first set `cs` to `0x08`, and then jump to `.flush_cs` using its absolute
address.

Whenever we load a new segment selector into a segment register, the processor
reads the entire descriptor and stores it in shadow registers within the
processor.

## Why segmentation?

The reason we want segmentation is because the `cs` segment selector specifies
what privilege level the processor executes as. Without segments there is no
concept of different privilege levels, and privilege levels are needed to make
sure that user-space processes (PL3) cannot read/execute kernel-space (PL0)
data. The actual memory protection is done through paging, but to set up the
different privilege levels x86 requires us to use segmentation.
