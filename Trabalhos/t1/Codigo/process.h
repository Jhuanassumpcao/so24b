#ifndef process_H
#define process_H

typedef enum {
    ESTADO_EXECUTANDO,
    ESTADO_PRONTO,
    ESTADO_BLOQUEADO,
    MORTO,
    N_ESTADO
} process_estado_t;



typedef enum {
    BLOQ_LEITURA,
    BLOQ_ESCRITA,
    BLOQ_ESPERA_PROCESS,
    N_BLOQ
} process_bloq_motivo_t;

typedef struct process_t process_t;
typedef struct process_estado_metricas_t process_estado_metricas_t;
typedef struct process_metricas_t process_metricas_t;

struct process_estado_metricas_t
{
    int n_vezes;
    int t_total;
};

struct process_metricas_t
{
    int n_preempcoes;
    
    int t_retorno;
    int t_resposta;

    process_estado_metricas_t estados[N_ESTADO];
};

struct process_t {
    int id;                         // identificador unico do processo
    double prioridade;              // prioridade do processo
    process_estado_t estado;           // estado atual do processo (pronto, executando, etc.)
    
    // detalhes do bloqueio do processo
    struct {
        int motivo;                 // motivo do bloqueio
        int argumento;              // argumento associado ao motivo do bloqueio
    } bloqueio;
    
    // identificador de dispositivo_es do processo
    int dispositivo_es;
    
    // registradores do processo
    struct {
        int PC;                    
        int A;                     
        int X;                     
    } registradores;
    
    // metricas associadas ao processo
    process_metricas_t metricas;
};

process_t *process_cria(int id, int end);
void process_destroi(process_t *self);

int process_id(process_t *self);
process_estado_t process_estado(process_t *self);

void process_executa(process_t *self);
void process_para(process_t *self);
void process_bloqueia(process_t *self, process_bloq_motivo_t motivo, int arg);
void process_desbloqueia(process_t *self);
void process_encerra(process_t *self);

void process_atualiza_metricas(process_t *self, int delta);

double process_prioridade(process_t *self);
double process_calcula_prioridade(process_t *process, int t_exec, int quantum);
void process_define_prioridade(process_t *self, double prioridade);

process_bloq_motivo_t process_bloq_motivo(process_t *self);
int process_bloq_arg(process_t *self);

int process_dispositivo_es(process_t *self);
void process_atribui_dispositivo_es(process_t *self, int dispositivo_es);
void process_desatribui_dispositivo_es(process_t *self);

int process_PC(process_t *self);
int process_A(process_t *self);
int process_X(process_t *self);

void process_define_PC(process_t *self, int valor);
void process_define_A(process_t *self, int valor);
void process_define_X(process_t *self, int valor);

process_metricas_t process_metricas(process_t *self);

char *process_estado_nome(process_estado_t estado);

#endif