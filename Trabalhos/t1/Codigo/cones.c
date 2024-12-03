#include "cones.h"
#include <stdlib.h>
#include <assert.h>

typedef struct dispositivo_es_livre_t {
    int indice;
    struct dispositivo_es_livre_t *prox;
} dispositivo_es_livre_t;

typedef struct dispositivo_es_t {
    bool reservada;
    struct dispositivo_es_t *prox;  // aponta para a proxima dispositivo_es na lista encadeada
    dispositivo_id_t escreve;
    dispositivo_id_t escreve_ok;
    dispositivo_id_t le;
    dispositivo_id_t le_ok;
} dispositivo_es_t;

struct con_es_t {
    es_t *es;
    dispositivo_es_t *dispositivo_es;          // lista encadeada de dispositivo_es
    dispositivo_es_livre_t *livres;     // lista encadeada de dispositivo_es livres
    int next_id;               // proximo índice unico para novas dispositivo_es
};

con_es_t *con_es_cria(es_t *es) {
    con_es_t *self = (con_es_t *)malloc(sizeof(con_es_t));
    assert(self != NULL);

    self->es = es;
    self->dispositivo_es = NULL;
    self->livres = NULL;
    self->next_id = 0;

    return self;
}

void con_es_destroi(con_es_t *self) {
    dispositivo_es_t *dispositivo_es_atual = self->dispositivo_es;
    while (dispositivo_es_atual) {
        dispositivo_es_t *tmp = dispositivo_es_atual;
        dispositivo_es_atual = dispositivo_es_atual->prox;
        free(tmp);
    }

    // libera a lista de dispositivo_es livres
    while (self->livres) {
        dispositivo_es_livre_t *tmp = self->livres;
        self->livres = self->livres->prox;
        free(tmp);
    }

    free(self);
}

int con_es_registra_dispositivo_es(con_es_t *self, dispositivo_id_t le, dispositivo_id_t le_ok, dispositivo_id_t escreve, dispositivo_id_t escreve_ok) {
    dispositivo_es_t *nova_dispositivo_es = (dispositivo_es_t *)malloc(sizeof(dispositivo_es_t));
    assert(nova_dispositivo_es != NULL);

    nova_dispositivo_es->le = le;
    nova_dispositivo_es->le_ok = le_ok;
    nova_dispositivo_es->escreve = escreve;
    nova_dispositivo_es->escreve_ok = escreve_ok;
    nova_dispositivo_es->reservada = false;
    nova_dispositivo_es->prox = self->dispositivo_es;

    self->dispositivo_es = nova_dispositivo_es;

    // adiciona o indice da dispositivo_es como livre na lista
    dispositivo_es_livre_t *nova_livre = (dispositivo_es_livre_t *)malloc(sizeof(dispositivo_es_livre_t));
    nova_livre->indice = self->next_id++;
    nova_livre->prox = self->livres;
    self->livres = nova_livre;

    return nova_livre->indice;
}

bool con_es_le_dispositivo_es(con_es_t *self, int id, int *pvalor) {
    dispositivo_es_t *dispositivo_es_atual = self->dispositivo_es;
    int i = 0;

    // procura pela dispositivo_es correspondente ao id
    while (dispositivo_es_atual && i++ != id) {
        dispositivo_es_atual = dispositivo_es_atual->prox;
    }
    assert(dispositivo_es_atual != NULL);

    int ok;
    if (es_le(self->es, dispositivo_es_atual->le_ok, &ok) != ERR_OK || !ok) {
        return false;
    }

    return es_le(self->es, dispositivo_es_atual->le, pvalor) == ERR_OK;
}

bool con_es_escreve_dispositivo_es(con_es_t *self, int id, int valor) {
    dispositivo_es_t *dispositivo_es_atual = self->dispositivo_es;
    int i = 0;

    // procura pela dispositivo_es correspondente ao id
    while (dispositivo_es_atual && i++ != id) {
        dispositivo_es_atual = dispositivo_es_atual->prox;
    }
    assert(dispositivo_es_atual != NULL);

    int ok;
    if (es_le(self->es, dispositivo_es_atual->escreve_ok, &ok) != ERR_OK || !ok) {
        return false;
    }

    return es_escreve(self->es, dispositivo_es_atual->escreve, valor) == ERR_OK;
}

void con_es_reserva_dispositivo_es(con_es_t *self, int id) {
    dispositivo_es_t *dispositivo_es_atual = self->dispositivo_es;
    int i = 0;

    // procura pela dispositivo_es correspondente ao id
    while (dispositivo_es_atual && i++ != id) {
        dispositivo_es_atual = dispositivo_es_atual->prox;
    }
    assert(dispositivo_es_atual != NULL);

    if (!dispositivo_es_atual->reservada) {
        dispositivo_es_atual->reservada = true;

        // remove a dispositivo_es da lista de dispositivo_es livres
        dispositivo_es_livre_t **ptr = &self->livres;
        while (*ptr && (*ptr)->indice != id) {
            ptr = &(*ptr)->prox;
        }
        if (*ptr) {
            dispositivo_es_livre_t *tmp = *ptr;
            *ptr = (*ptr)->prox;
            free(tmp);
        }
    }
}

void con_es_libera_dispositivo_es(con_es_t *self, int id) {
    dispositivo_es_t *dispositivo_es_atual = self->dispositivo_es;
    int i = 0;

    // procura pela dispositivo_es correspondente ao id
    while (dispositivo_es_atual && i++ != id) {
        dispositivo_es_atual = dispositivo_es_atual->prox;
    }
    assert(dispositivo_es_atual != NULL);

    if (dispositivo_es_atual->reservada) {
        dispositivo_es_atual->reservada = false;

        // adiciona a dispositivo_es a lista de dispositivo_es livres
        dispositivo_es_livre_t *nova_livre = (dispositivo_es_livre_t *)malloc(sizeof(dispositivo_es_livre_t));
        nova_livre->indice = id;
        nova_livre->prox = self->livres;
        self->livres = nova_livre;
    }
}

int con_es_dispositivo_es_disponivel(con_es_t *self) {
    if (self->livres == NULL) {
        return -1;
    }

    int id = self->livres->indice;
    con_es_reserva_dispositivo_es(self, id);
    return id;
}
