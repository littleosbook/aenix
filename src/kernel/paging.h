#ifndef PAGING_H
#define PAGING_H

#include "stdint.h"

#define PAGING_READ_ONLY  0
#define PAGING_READ_WRITE 1
#define PAGING_PL0        0
#define PAGING_PL3        1

typedef struct pde pde_t;

uint32_t paging_init(uint32_t kernel_pdt_vaddr, uint32_t kernel_pt_vaddr);

uint32_t pdt_kernel_find_next_vaddr(uint32_t size);

uint32_t pdt_map_kernel_memory(uint32_t paddr,
                               uint32_t vaddr,
                               uint32_t size,
                               uint8_t rw,
                               uint8_t pl);
uint32_t pdt_map_memory(pde_t *pdt,
                        uint32_t paddr,
                        uint32_t vaddr,
                        uint32_t size,
                        uint8_t rw,
                        uint8_t pl);

uint32_t pdt_unmap_kernel_memory(uint32_t vaddr, uint32_t size);
uint32_t pdt_unmap_memory(pde_t *pdt, uint32_t vaddr, uint32_t size);

pde_t *pdt_create(uint32_t *out_paddr);
void pdt_delete(pde_t *pdt);

void pdt_load_process_pdt(pde_t *pdt, uint32_t pdt_paddr);

#endif /* PAGING_H */
