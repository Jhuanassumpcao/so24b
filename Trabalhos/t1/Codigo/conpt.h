#ifndef con_pt_H
#define con_pt_H

#include "es.h"

typedef struct con_pt_t con_pt_t;

con_pt_t *con_pt_cria(es_t *es);
void con_pt_destroi(con_pt_t *self);

int con_pt_registra_porta(con_pt_t *self, dispositivo_id_t le, dispositivo_id_t le_ok, dispositivo_id_t escreve, dispositivo_id_t escreve_ok);

bool con_pt_le_porta(con_pt_t *self, int id, int *pvalor);
bool con_pt_escreve_porta(con_pt_t *self, int id, int valor);

void con_pt_reserva_porta(con_pt_t *self, int id);
void con_pt_libera_porta(con_pt_t *self, int id);

int con_pt_porta_disponivel(con_pt_t *self);

#endif