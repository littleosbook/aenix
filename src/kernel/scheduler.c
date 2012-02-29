#include "scheduler.h"
#include "stddef.h"
#include "tss.h"
#include "paging.h"
#include "constants.h"
#include "log.h"
#include "kmalloc.h"
#include "math.h"
#include "pit.h"
#include "interrupt.h"
#include "pic.h"
#include "common.h"

#define SCHEDULER_PIT_INTERVAL 2 /* in ms */
#define SCHEDULER_TIME_SLICE   (5 * SCHEDULER_PIT_INTERVAL) /* in ms */

uint32_t ms = 0;

struct ps_list_ele {
    struct ps_list_ele *next;
    ps_t *ps;
};
typedef struct ps_list_ele ps_list_ele_t;

struct ps_list {
    ps_list_ele_t *start;
    ps_list_ele_t *end;
};
typedef struct ps_list ps_list_t;

static ps_list_t runnable_pss = { NULL, NULL };
static ps_list_t zombie_pss = { NULL, NULL };

/* defined in scheduler_asm.s */
void run_process_in_user_mode(registers_t *registers);
void run_process_in_kernel_mode(registers_t *registers);

static uint32_t max_id_in_ps_list(ps_list_t *pss)
{
    uint32_t max = 0;
    ps_list_ele_t *p = pss->start;
    while (p != NULL) {
        if (p->ps != NULL && p->ps->id >= max) {
            max = p->ps->id;
        }
        p = p->next;
    }

    return max;
}

uint32_t scheduler_next_pid(void)
{
    uint32_t pid_runnable = max_id_in_ps_list(&runnable_pss);
    uint32_t pid_zombie = max_id_in_ps_list(&zombie_pss);

    return maxu(pid_runnable, pid_zombie) + 1;
}

static void scheduler_copy_common_registers(registers_t *r,
                                            cpu_state_t const *cpu,
                                            stack_state_t const *stack)
{
    r->eax = cpu->eax;
    r->ebx = cpu->ebx;
    r->ecx = cpu->ecx;
    r->edx = cpu->edx;
    r->ebp = cpu->ebp;
    r->esi = cpu->esi;
    r->edi = cpu->edi;
    r->eip = stack->eip;
    r->eflags = stack->eflags;
    r->cs = stack->cs;
}

static void scheduler_update_user_registers(ps_t *ps, cpu_state_t const *cpu,
                                            stack_state_t const *stack)
{
    scheduler_copy_common_registers(&ps->user_mode, cpu, stack);
    ps->user_mode.esp = stack->user_esp;
    ps->user_mode.ss = stack->user_ss;
    ps->current = ps->user_mode;
}

static void scheduler_update_kernel_registers(ps_t *ps, cpu_state_t const *cpu,
                                              stack_state_t const *stack)
{
    scheduler_copy_common_registers(&ps->current, cpu, stack);
    ps->current.esp = cpu->esp + 12; /* +12 to skip EIP, CS and EFLAGS */
    ps->current.ss = SEGSEL_KERNEL_DS;
}


static void scheduler_schedule_on_intterupt(cpu_state_t const *cpu,
                                            stack_state_t const *stack)
{
    disable_interrupts();
    ps_t *ps = scheduler_get_current_process();
    if (stack->cs == (SEGSEL_USER_SPACE_CS | 0x03)) {
        scheduler_update_user_registers(ps, cpu, stack);
    } else {
        scheduler_update_kernel_registers(ps, cpu, stack);
    }

    pic_acknowledge();

    scheduler_schedule();
}

static void scheduler_handle_pit_interrupt(cpu_state_t cpu, idt_info_t info,
                                           stack_state_t stack)
{
    UNUSED_ARGUMENT(info);
    ms += SCHEDULER_PIT_INTERVAL;

    if (ms >= SCHEDULER_TIME_SLICE) {
        ms = 0;
        scheduler_schedule_on_intterupt(&cpu, &stack);
    } else {
        pic_acknowledge();
    }
}

int scheduler_init(void)
{
    pit_set_interval(SCHEDULER_PIT_INTERVAL);
    return register_interrupt_handler(PIT_INT_IDX,
                                      &scheduler_handle_pit_interrupt);
}

ps_t *scheduler_get_current_process()
{
    return runnable_pss.start == NULL ? NULL : runnable_pss.start->ps;
}

static int scheduler_add_process(ps_list_t *pss, ps_t *ps)
{
    ps_list_ele_t *ele = kmalloc(sizeof(ps_list_ele_t));
    if (ele == NULL) {
        log_error("scheduler_add_process",
                  "Couldn't allocate memory for ps_list_t struct\n");
        return -1;
    }

    ele->ps = ps;
    ele->next = NULL;

    if (pss->start == NULL) {
        pss->start = ele;
    } else {
        pss->end->next = ele;
    }

    pss->end = ele;

    return 0;
}

int scheduler_add_runnable_process(ps_t *ps)
{
    return scheduler_add_process(&runnable_pss, ps);
}

static int scheduler_remove_process(ps_list_t *pss, uint32_t pid,
                                    uint32_t should_delete)
{
    ps_list_ele_t *current = pss->start;
    ps_list_ele_t *prev = NULL;
    while (current != NULL) {
        if (current->ps != NULL && current->ps->id == pid) {
            if (prev == NULL) {
                pss->start= current->next;
            } else {
                prev->next = current->next;
            }

            if (pss->end == current) {
                pss->end = prev;
                prev->next = NULL;
            }

            if (should_delete) {
                process_delete_resources(current->ps);
                kfree(current->ps);
            }

            kfree(current);

            return 0;
        }
        prev = current;
        current = current->next;
    }

    return -1;
}

void scheduler_terminate_process(ps_t *ps)
{
    scheduler_remove_process(&runnable_pss, ps->id, 0);
    process_delete_resources(ps);
    scheduler_add_process(&zombie_pss, ps);
}

int scheduler_has_any_child_terminated(ps_t *parent)
{
    ps_list_ele_t *e = zombie_pss.start;
    while (e != NULL) {
        if (e->ps->parent_id == parent->id) {
            return 1;
        }
        e = e->next;
    }

    return 0;
}

static int scheduler_count_children_in_list(ps_list_t *pss, uint32_t pid)
{
    int num = 0;
    ps_list_ele_t *e = pss->start;
    while (e != NULL) {
        if (e->ps->parent_id == pid) {
            ++num;
        }
        e = e->next;
    }

    return num;
}

int scheduler_num_children(uint32_t pid)
{
    int num_zombies = scheduler_count_children_in_list(&zombie_pss, pid);
    int num_running = scheduler_count_children_in_list(&runnable_pss, pid);

    return num_zombies + num_running;
}

int scheduler_replace_process(ps_t *old, ps_t *new)
{
    int error;

    error = scheduler_remove_process(&runnable_pss, old->id, 1);
    if (error) {
        log_error("scheduler_replace_process",
                  "Couldn't remove old process %u\n", old->id);
        return -1;
    }

    return scheduler_add_runnable_process(new);
}

static ps_t *scheduler_get_and_rotate_runnable_process(void)
{
    if (runnable_pss.start == NULL || runnable_pss.start->ps == NULL) {
        log_error("scheduler_get_and_rotate_runnable_process",
                  "There are no processes to schedule\n");
        return NULL;
    }

    ps_list_ele_t *start = runnable_pss.start;
    if (start->next != NULL) {
        /* more than one element in the list */
        /* move current process (head of list) to end of list */
        runnable_pss.start = start->next;
        start->next = NULL;
        runnable_pss.end->next = start;
        runnable_pss.end = start;
    }

    return runnable_pss.start->ps;
}

void scheduler_schedule(void)
{
    ps_t *ps = scheduler_get_and_rotate_runnable_process();
    if (ps == NULL) {
        log_error("scheduler_schedule", "Can't schedule processes\n");
        return;
    }

    tss_set_kernel_stack(SEGSEL_KERNEL_DS, ps->kernel_stack_start_vaddr);
    pdt_load_process_pdt(ps->pdt, ps->pdt_paddr);

    if (ps->current.cs == SEGSEL_KERNEL_CS) {
        run_process_in_kernel_mode(&ps->current);
    } else {
        run_process_in_user_mode(&ps->current);
    }
}
