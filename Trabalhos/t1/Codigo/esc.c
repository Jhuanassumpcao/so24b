#include "esc.h"
#include <stdlib.h>
#include <assert.h>

typedef struct escal_no_t {
  process_t *process;
  struct escal_no_t *prox;
  struct escal_no_t *ant;  // Adicionamos um ponteiro para o nó anterior
} escal_no_t;

struct escal_t {
  escal_modo_t modo;
  escal_no_t *primeiro;
  escal_no_t *ultimo;     // Adicionamos um ponteiro para o último nó
  escal_no_t *melhor;
  escal_no_t *atual;
  int tamanho;            // Adicionamos um contador de processos
};

// Funções auxiliares
static void escal_atualiza_melhor(escal_t *self);
static process_t *escal_proximo_modo_simples(escal_t *self);
static process_t *escal_proximo_modo_circular(escal_t *self);
static process_t *escal_proximo_modo_prioritario(escal_t *self);
static void escal_remove_no(escal_t *self, escal_no_t *no);

escal_t *escal_cria(escal_modo_t modo) {
  escal_t *self = (escal_t *)malloc(sizeof(escal_t));
  assert(self != NULL);
  self->modo = modo;
  self->primeiro = NULL;
  self->ultimo = NULL;
  self->melhor = NULL;
  self->atual = NULL;
  self->tamanho = 0;
  return self;
}

void escal_destroi(escal_t *self) {
  escal_no_t *atual = self->primeiro;
  while (atual != NULL) {
    escal_no_t *prox = atual->prox;
    free(atual);
    atual = prox;
  }
  free(self);
}

void escal_insere_proc(escal_t *self, process_t *process) {
  escal_no_t *novo_no = (escal_no_t *)malloc(sizeof(escal_no_t));
  assert(novo_no != NULL);
  novo_no->process = process;

  if (self->primeiro == NULL) {
    novo_no->prox = novo_no;
    novo_no->ant = novo_no;
    self->primeiro = novo_no;
    self->ultimo = novo_no;
    self->melhor = novo_no;
    self->atual = novo_no;
  } else {
    novo_no->prox = self->primeiro;
    novo_no->ant = self->ultimo;
    self->ultimo->prox = novo_no;
    self->primeiro->ant = novo_no;
    self->ultimo = novo_no;

    if (process_prioridade(process) < process_prioridade(self->melhor->process)) {
      self->melhor = novo_no;
    }
  }
  self->tamanho++;
}

void escal_remove_proc(escal_t *self, process_t *process) {
  if (self->primeiro == NULL) return;

  escal_no_t *atual = self->primeiro;
  do {
    if (atual->process == process) {
      escal_remove_no(self, atual);
      return;
    }
    atual = atual->prox;
  } while (atual != self->primeiro);
}

static void escal_remove_no(escal_t *self, escal_no_t *no) {
  if (self->tamanho == 1) {
    self->primeiro = NULL;
    self->ultimo = NULL;
    self->melhor = NULL;
    self->atual = NULL;
  } else {
    no->ant->prox = no->prox;
    no->prox->ant = no->ant;
    if (no == self->primeiro) self->primeiro = no->prox;
    if (no == self->ultimo) self->ultimo = no->ant;
    if (no == self->atual) self->atual = no->prox;
    if (no == self->melhor) escal_atualiza_melhor(self);
  }
  free(no);
  self->tamanho--;
}

process_t *escal_proximo(escal_t *self) {
  if (self->tamanho == 0) return NULL;

  switch (self->modo) {
    case escal_MODO_SIMPLES:
      return escal_proximo_modo_simples(self);
    case escal_MODO_CIRCULAR:
      return escal_proximo_modo_circular(self);
    case escal_MODO_PRIORITARIO:
      return escal_proximo_modo_prioritario(self);
    default:
      return NULL;
  }
}

static void escal_atualiza_melhor(escal_t *self) {
  if (self->primeiro == NULL) {
    self->melhor = NULL;
    return;
  }

  escal_no_t *atual = self->primeiro;
  escal_no_t *melhor_no = atual;

  do {
    if (process_prioridade(atual->process) < process_prioridade(melhor_no->process)) {
      melhor_no = atual;
    }
    atual = atual->prox;
  } while (atual != self->primeiro);

  self->melhor = melhor_no;
}

static process_t *escal_proximo_modo_simples(escal_t *self) {
  return self->atual ? self->atual->process : NULL;
}

static process_t *escal_proximo_modo_circular(escal_t *self) {
  if (self->atual == NULL) return NULL;

  process_t *process = self->atual->process;
  self->atual = self->atual->prox;
  return process;
}

static process_t *escal_proximo_modo_prioritario(escal_t *self) {
  return self->melhor ? self->melhor->process : NULL;
}