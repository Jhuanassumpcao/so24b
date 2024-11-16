#include "esc.h"

#include <stdlib.h>
#include <assert.h>

typedef struct escal_no_t {
  process_t *process;
  struct escal_no_t *prox;  // proximo no na lista
} escal_no_t;

struct escal_t {
  escal_modo_t modo;       // modo de operacao do escalonador
  escal_no_t *primeiro;    // primeiro no da fila
  escal_no_t *melhor;      // cache para o no de maior prioridade
  escal_no_t *atual;       // ponteiro de estado para modos
};

// funcoes auxiliares
static void escal_atualiza_melhor(escal_t *self);
static process_t *escal_proximo_modo_simples(escal_t *self);
static process_t *escal_proximo_modo_circular(escal_t *self);
static process_t *escal_proximo_modo_prioritario(escal_t *self);

// cria um escalonador
escal_t *escal_cria(escal_modo_t modo) {
  escal_t *self = (escal_t *)malloc(sizeof(escal_t));
  assert(self != NULL);
  self->modo = modo;
  self->primeiro = NULL;
  self->melhor = NULL;
  self->atual = NULL; // inicializamos o ponteiro para modos
  return self;
}

// destroi o escalonador
void escal_destroi(escal_t *self) {
  if (self->primeiro) {
    escal_no_t *atual = self->primeiro;
    do {
      escal_no_t *prox = atual->prox;
      free(atual);
      atual = prox;
    } while (atual != self->primeiro);
  }
  free(self);
}

// insere um processo na fila
void escal_insere_proc(escal_t *self, process_t *process) {
  escal_no_t *novo_no = (escal_no_t *)malloc(sizeof(escal_no_t));
  assert(novo_no != NULL);
  novo_no->process = process;

  if (self->primeiro == NULL) {
    novo_no->prox = novo_no;
    self->primeiro = novo_no;
    self->melhor = novo_no;  // atualiza o cache do melhor
    self->atual = novo_no;   // atualiza o ponteiro de estado
  } else {
    // insere o no no final da lista circular
    escal_no_t *ultimo = self->primeiro;
    while (ultimo->prox != self->primeiro) {
      ultimo = ultimo->prox;
    }
    ultimo->prox = novo_no;
    novo_no->prox = self->primeiro;

    // atualiza o cache do melhor se necessario
    if (process_prioridade(process) < process_prioridade(self->melhor->process)) {
      self->melhor = novo_no;
    }
  }
}

// remove um processo da fila
void escal_remove_proc(escal_t *self, process_t *process) {
  if (self->primeiro == NULL) return;

  escal_no_t *atual = self->primeiro;
  escal_no_t *anterior = NULL;

  do {
    if (atual->process == process) {
      if (anterior == NULL) { // primeiro no
        if (atual->prox == atual) { // unico no
          self->primeiro = NULL;
          self->melhor = NULL;
          self->atual = NULL;
        } else {
          escal_no_t *ultimo = self->primeiro;
          while (ultimo->prox != self->primeiro) {
            ultimo = ultimo->prox;
          }
          self->primeiro = atual->prox;
          ultimo->prox = self->primeiro;
          if (self->atual == atual) self->atual = self->primeiro;
          if (self->melhor == atual) escal_atualiza_melhor(self);
        }
      } else {
        anterior->prox = atual->prox;
        if (self->atual == atual) self->atual = atual->prox;
        if (self->melhor == atual) escal_atualiza_melhor(self);
      }
      free(atual);
      return;
    }
    anterior = atual;
    atual = atual->prox;
  } while (atual != self->primeiro);
}

// retorna o proximo processo com base no modo
process_t *escal_proximo(escal_t *self) {
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

// atualiza o cache do melhor processo
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

// modo simples: Sempre retorna o processo armazenado em `self->atual`
static process_t *escal_proximo_modo_simples(escal_t *self) {
  return self->atual ? self->atual->process : NULL;
}

// modo circular: Retorna o processo armazenado em `self->atual` e avança o ponteiro
static process_t *escal_proximo_modo_circular(escal_t *self) {
  if (self->atual == NULL) return NULL;

  process_t *process = self->atual->process;
  self->atual = self->atual->prox; // avança para o próximo
  return process;
}

// modo prioritario: Retorna o processo com maior prioridade (cache)
static process_t *escal_proximo_modo_prioritario(escal_t *self) {
  return self->melhor ? self->melhor->process : NULL;
}
