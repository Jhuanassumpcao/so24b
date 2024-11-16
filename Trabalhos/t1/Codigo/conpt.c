#include "conpt.h"
#include <stdlib.h>
#include <assert.h>

typedef struct porta_livre_t {
    int indice;
    struct porta_livre_t *prox;
} porta_livre_t;

typedef struct porta_t {
    dispositivo_id_t escreve;
    dispositivo_id_t escreve_ok;
    dispositivo_id_t le;
    dispositivo_id_t le_ok;
    bool reservada;
    struct porta_t *prox;  // aponta para a proxima porta na lista encadeada
} porta_t;

struct con_pt_t {
    es_t *es;
    porta_t *portas;          // lista encadeada de portas
    porta_livre_t *livres;     // lista encadeada de portas livres
    int next_id;               // proximo índice unico para novas portas
};

con_pt_t *con_pt_cria(es_t *es) {
    con_pt_t *self = (con_pt_t *)malloc(sizeof(con_pt_t));
    assert(self != NULL);

    self->es = es;
    self->portas = NULL;
    self->livres = NULL;
    self->next_id = 0;

    return self;
}

void con_pt_destroi(con_pt_t *self) {
    porta_t *porta_atual = self->portas;
    while (porta_atual) {
        porta_t *tmp = porta_atual;
        porta_atual = porta_atual->prox;
        free(tmp);
    }

    // libera a lista de portas livres
    while (self->livres) {
        porta_livre_t *tmp = self->livres;
        self->livres = self->livres->prox;
        free(tmp);
    }

    free(self);
}

int con_pt_registra_porta(con_pt_t *self, dispositivo_id_t le, dispositivo_id_t le_ok, dispositivo_id_t escreve, dispositivo_id_t escreve_ok) {
    porta_t *nova_porta = (porta_t *)malloc(sizeof(porta_t));
    assert(nova_porta != NULL);

    nova_porta->le = le;
    nova_porta->le_ok = le_ok;
    nova_porta->escreve = escreve;
    nova_porta->escreve_ok = escreve_ok;
    nova_porta->reservada = false;
    nova_porta->prox = self->portas;

    self->portas = nova_porta;

    // adiciona o indice da porta como livre na lista
    porta_livre_t *nova_livre = (porta_livre_t *)malloc(sizeof(porta_livre_t));
    nova_livre->indice = self->next_id++;
    nova_livre->prox = self->livres;
    self->livres = nova_livre;

    return nova_livre->indice;
}

bool con_pt_le_porta(con_pt_t *self, int id, int *pvalor) {
    porta_t *porta_atual = self->portas;
    int i = 0;

    // procura pela porta correspondente ao id
    while (porta_atual && i++ != id) {
        porta_atual = porta_atual->prox;
    }
    assert(porta_atual != NULL);

    int ok;
    if (es_le(self->es, porta_atual->le_ok, &ok) != ERR_OK || !ok) {
        return false;
    }

    return es_le(self->es, porta_atual->le, pvalor) == ERR_OK;
}

bool con_pt_escreve_porta(con_pt_t *self, int id, int valor) {
    porta_t *porta_atual = self->portas;
    int i = 0;

    // procura pela porta correspondente ao id
    while (porta_atual && i++ != id) {
        porta_atual = porta_atual->prox;
    }
    assert(porta_atual != NULL);

    int ok;
    if (es_le(self->es, porta_atual->escreve_ok, &ok) != ERR_OK || !ok) {
        return false;
    }

    return es_escreve(self->es, porta_atual->escreve, valor) == ERR_OK;
}

void con_pt_reserva_porta(con_pt_t *self, int id) {
    porta_t *porta_atual = self->portas;
    int i = 0;

    // procura pela porta correspondente ao id
    while (porta_atual && i++ != id) {
        porta_atual = porta_atual->prox;
    }
    assert(porta_atual != NULL);

    if (!porta_atual->reservada) {
        porta_atual->reservada = true;

        // remove a porta da lista de portas livres
        porta_livre_t **ptr = &self->livres;
        while (*ptr && (*ptr)->indice != id) {
            ptr = &(*ptr)->prox;
        }
        if (*ptr) {
            porta_livre_t *tmp = *ptr;
            *ptr = (*ptr)->prox;
            free(tmp);
        }
    }
}

void con_pt_libera_porta(con_pt_t *self, int id) {
    porta_t *porta_atual = self->portas;
    int i = 0;

    // procura pela porta correspondente ao id
    while (porta_atual && i++ != id) {
        porta_atual = porta_atual->prox;
    }
    assert(porta_atual != NULL);

    if (porta_atual->reservada) {
        porta_atual->reservada = false;

        // adiciona a porta a lista de portas livres
        porta_livre_t *nova_livre = (porta_livre_t *)malloc(sizeof(porta_livre_t));
        nova_livre->indice = id;
        nova_livre->prox = self->livres;
        self->livres = nova_livre;
    }
}

int con_pt_porta_disponivel(con_pt_t *self) {
    if (self->livres == NULL) {
        return -1;
    }

    int id = self->livres->indice;
    con_pt_reserva_porta(self, id);
    return id;
}
