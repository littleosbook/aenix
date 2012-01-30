global pdt_set
global invalidate_page_table_entry

section .text:

pdt_set:
    mov eax, [esp+4]    ; loads the address of the pdt into eax
    and eax, 0xFFFFF000 ; we only care about the highest 20 bits
    or  eax, 0x08       ; we wan't page write through! PWT FTW!
    mov	cr3, eax        ; loads the PDT
    ret

invalidate_page_table_entry:
    mov eax, [esp+4]    ; loads the virtual address which page table entry
                        ; will be flushed
    invlpg [eax]
    ret
