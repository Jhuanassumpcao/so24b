#include "process.h"
#include <stdlib.h>
#include <assert.h>


// macro para registrar transições de estado
#define MUDAR_ESTADO(process, novo_estado) \
    do { \
        if ((process)->estado == ESTADO_EXECUTANDO && (novo_estado) == ESTADO_PRONTO) { \
            (process)->metricas.n_preempcoes++; \
        } \
        (process)->metricas.estados[(novo_estado)].n_vezes++; \
        (process)->estado = (novo_estado); \
    } while (0)

// funcao para inicializar as metricas
static inline void inicializa_metricas(process_metricas_t *metricas) {
    metricas->t_retorno = 0;
    metricas->n_preempcoes = 0;
    for (int i = 0; i < N_ESTADO; ++i) {
        metricas->estados[i].n_vezes = 0;
        metricas->estados[i].t_total = 0;
    }
    metricas->estados[ESTADO_PRONTO].n_vezes = 1;
}

process_t *process_cria(int id, int end) {
    process_t *process = (process_t *)malloc(sizeof(process_t));
    if (!process) abort();

    process->id = id;
    process->prioridade = 0.5;
    process->estado = ESTADO_PRONTO;
    process->bloqueio.motivo = 0;
    process->bloqueio.argumento = 0;
    process->dispositivo_es = -1;
    process->registradores.PC = end;
    process->registradores.A = 0;
    process->registradores.X = 0;

    inicializa_metricas(&process->metricas);
    return process;
}

void process_destroi(process_t *process) {
    if (process) free(process);
}

int process_id(process_t *process) { return process->id; }

process_estado_t process_estado(process_t *process) { return process->estado; }

void process_executa(process_t *process) {
    if (process->estado == ESTADO_PRONTO) MUDAR_ESTADO(process, ESTADO_EXECUTANDO);
}

void process_para(process_t *process) {
    if (process->estado == ESTADO_EXECUTANDO) MUDAR_ESTADO(process, ESTADO_PRONTO);
}

void process_bloqueia(process_t *process, process_bloq_motivo_t motivo, int arg) {
    if (process->estado == ESTADO_EXECUTANDO || process->estado == ESTADO_PRONTO) {
        MUDAR_ESTADO(process, ESTADO_BLOQUEADO);
        process->bloqueio.motivo = motivo;
        process->bloqueio.argumento = arg;
    }
}

double process_calcula_prioridade(process_t *process, int t_exec, int quantum)
{
    double prioridade_atual = process_prioridade(process);
    double fator_execucao = (double)t_exec / quantum;

    // calcular prioridade com formula otimizada
    return (prioridade_atual + fator_execucao) * 0.5;
}

void process_desbloqueia(process_t *process) {
    if (process->estado == ESTADO_BLOQUEADO) MUDAR_ESTADO(process, ESTADO_PRONTO);
}

void process_encerra(process_t *process) {
    MUDAR_ESTADO(process, MORTO);
}

void process_atualiza_metricas(process_t *process, int delta) {
    if (process->estado != MORTO) process->metricas.t_retorno += delta;
    process->metricas.estados[process->estado].t_total += delta;

    if (process->metricas.estados[ESTADO_PRONTO].n_vezes > 0) {
        process->metricas.t_resposta = process->metricas.estados[ESTADO_PRONTO].t_total / 
                                    process->metricas.estados[ESTADO_PRONTO].n_vezes;
    }
}

double process_prioridade(process_t *process) { return process->prioridade; }

void process_define_prioridade(process_t *process, double prioridade) { process->prioridade = prioridade; }

int process_PC(process_t *process) { return process->registradores.PC; }

int process_A(process_t *process) { return process->registradores.A; }

int process_X(process_t *process) { return process->registradores.X; }

void process_define_PC(process_t *process, int valor) { process->registradores.PC = valor; }

void process_define_A(process_t *process, int valor) { process->registradores.A = valor; }

void process_define_X(process_t *process, int valor) { process->registradores.X = valor; }

process_bloq_motivo_t process_bloq_motivo(process_t *process) { return process->bloqueio.motivo; }

int process_bloq_arg(process_t *process) { return process->bloqueio.argumento; }

int process_dispositivo_es(process_t *process) { return process->dispositivo_es; }

void process_atribui_dispositivo_es(process_t *process, int dispositivo_es) {
    if (dispositivo_es != -1) process->dispositivo_es = dispositivo_es;
}

void process_desatribui_dispositivo_es(process_t *process) { process->dispositivo_es = -1; }

process_metricas_t process_metricas(process_t *process) { return process->metricas; }

char *process_estado_nome(process_estado_t estado) {
    static const char *nomes_estados[] = {"Pronto", "Executando", "Bloqueado", "Morto"};
    return (estado >= 0 && estado < N_ESTADO) ? (char *)nomes_estados[estado] : "Desconhecido";
}
