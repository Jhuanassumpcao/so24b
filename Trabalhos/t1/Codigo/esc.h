#ifndef escal_H
#define escal_H

#include <stdbool.h>
#include "process.h"

typedef enum {
    escal_MODO_SIMPLES,
    escal_MODO_CIRCULAR,
    escal_MODO_PRIORITARIO,
    N_escal_MODO
} escal_modo_t;

typedef struct escal_t escal_t;

escal_t *escal_cria(escal_modo_t modo);
void escal_destroi(escal_t *self);

void escal_insere_proc(escal_t *self, process_t *process);
void escal_remove_proc(escal_t *self, process_t *process);

process_t *escal_proximo(escal_t *self);

#endif