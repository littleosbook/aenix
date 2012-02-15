#include "scheduler.h"
#include "stddef.h"
#include "tss.h"
#include "paging.h"
#include "constants.h"
#include "log.h"
#include "kmalloc.h"
#include "math.h"

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
void run_process(registers_t *registers);

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
    run_process(&ps->current);
}
