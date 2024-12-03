#ifndef con_es_H
#define con_es_H

#include "es.h"

typedef struct con_es_t con_es_t;

con_es_t *con_es_cria(es_t *es);
void con_es_destroi(con_es_t *self);

int con_es_registra_dispositivo_es(con_es_t *self, dispositivo_id_t le, dispositivo_id_t le_ok, dispositivo_id_t escreve, dispositivo_id_t escreve_ok);

bool con_es_le_dispositivo_es(con_es_t *self, int id, int *pvalor);
bool con_es_escreve_dispositivo_es(con_es_t *self, int id, int valor);

void con_es_reserva_dispositivo_es(con_es_t *self, int id);
void con_es_libera_dispositivo_es(con_es_t *self, int id);

int con_es_dispositivo_es_disponivel(con_es_t *self);

#endif