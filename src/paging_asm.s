global pdt_set

section .text:

pdt_set:
	mov	eax, [esp+4]	; loads the address of the pdt into eax
	mov	cr3, eax		; loads the PDT
	ret
