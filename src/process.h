#ifndef PROCESS_H
#define PROCESS_H

typedef struct ps ps_t;

ps_t *create_process(char *path);
void start_process(ps_t *ps);

#endif /* PROCESS_H */
