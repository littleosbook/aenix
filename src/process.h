#ifndef PROCESS_H
#define PROCESS_H

typedef struct ps ps_t;

ps_t *process_create(char *path);
void process_enter_user_mode(ps_t *init);

#endif /* PROCESS_H */
