global pdt_set

section .text:

pdt_set:
	mov	eax, [esp+4]	; loads the address of the pdt into eax
	mov	cr3, eax		; loads the PDT
	mov	eax, cr0		; read current config from cr0
	or	eax, 0x80000000	; the highest bit controls paging
	mov cr0, eax		; enable paging by writing config to cr0
	ret
