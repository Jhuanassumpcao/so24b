// so.c
// sistema operacional
// simulador de computador
// so24b

// INCLUDES {{{1
#include "so.h"
#include "dispositivos.h"
#include "irq.h"
#include "programa.h"
#include "instrucao.h"

#include "cones.h"
#include "esc.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

// CONSTANTES E TIPOS {{{1
// intervalo entre interrupções do relógio
#define INTERVALO_INTERRUPCAO 50   // em instruções executadas
#define INTERVALO_QUANTUM 5 // em interrupções de relógio

#define TAM_TABELA_PROC 16

#define escal_MODO escal_MODO_PRIORITARIO

typedef struct so_metricas_t so_metricas_t;

struct so_metricas_t {
  int t_exec;
  int t_ocioso;

  int n_preempcoes;

  int n_irqs[N_IRQ];
};

typedef struct {
    int quantum;        // duração do quantum para cada processo
    int restante;       // tempo restante do quantum atual
    int relogio_atual;  // tempo atual do relogio do sistema
} so_temporizador_t;


typedef struct {
    process_t **tabela;   // vetor dinâmico de processos
    int qtd;           // quantidade de processos ativos
    int capacidade;    // capacidade alocada atualmente
} tabela_process_t;

struct so_t {
  cpu_t *cpu;
  mem_t *mem;
  es_t *es;
  console_t *console;
  con_es_t *com;
  escal_t *esc;

  tabela_process_t processos;
  process_t *process_corrente;

  so_temporizador_t temporizador;

  so_metricas_t metricas;

  bool erro_interno;
  // t1: tabela de processos, processo corrente, pendências, etc
};


// função de tratamento de interrupção (entrada no SO)
static int so_trata_interrupcao(void *argC, int reg_A);

// função de gerenciamento de processos
static process_t *so_busca_proc(so_t *self, int pid);
static process_t *so_cria_proc(so_t *self, char *nome_do_executavel);
static void so_mata_proc(so_t *self, process_t *process);
static void so_executa_proc(so_t *self, process_t *process);
static void so_bloqueia_proc(so_t *self, process_t *process, process_bloq_motivo_t motivo, int arg);
static void so_desbloqueia_proc(so_t *self, process_t *process);
static void so_assegura_dispositivo_es_proc(so_t *self, process_t *process);
static bool so_deve_escalonar(so_t *self);
static bool so_tem_trabalho(so_t *self);

// funções para contabilização de métricas
static void so_atualiza_metricas(so_t *self, int delta);
static void so_imprime_metricas(so_t *self);

// funções auxiliares
// carrega o programa contido no arquivo na memória do processador; retorna end. inicial
static int so_carrega_programa(so_t *self, char *nome_do_executavel);
// copia para str da memória do processador, até copiar um 0 (retorna true) ou tam bytes
static bool copia_str_da_mem(int tam, char str[tam], mem_t *mem, int ender);

// CRIAÇÃO {{{1

void tabela_process_init(tabela_process_t *tabela, int capacidade_inicial) {
    tabela->tabela = malloc(capacidade_inicial * sizeof(process_t *));
    tabela->qtd = 0;
    tabela->capacidade = capacidade_inicial;
}
void tabela_process_expandir(tabela_process_t *tabela) {
    int nova_capacidade = tabela->capacidade * 2;
    tabela->tabela = realloc(tabela->tabela, nova_capacidade * sizeof(process_t *));
    tabela->capacidade = nova_capacidade;
}

void tabela_process_destruir(tabela_process_t *tabela) {
    free(tabela->tabela);
    tabela->tabela = NULL;
    tabela->qtd = 0;
    tabela->capacidade = 0;
}

so_t *so_cria(cpu_t *cpu, mem_t *mem, es_t *es, console_t *console)
{
  so_t *self = malloc(sizeof(*self));
  if (self == NULL) return NULL;

  self->cpu = cpu;
  self->mem = mem;
  self->es = es;
  self->console = console;
  self->esc = escal_cria(escal_MODO);
  self->com = con_es_cria(es);


  self->process_corrente = NULL;

  tabela_process_init(&self->processos, TAM_TABELA_PROC);

  self->temporizador.quantum = INTERVALO_QUANTUM;
  self->temporizador.restante = 0;
  self->temporizador.relogio_atual = -1;

  self->erro_interno = false;

  con_es_registra_dispositivo_es(self->com, D_TERM_A_TECLADO, D_TERM_A_TECLADO_OK, D_TERM_A_TELA, D_TERM_A_TELA_OK);
  con_es_registra_dispositivo_es(self->com, D_TERM_B_TECLADO, D_TERM_B_TECLADO_OK, D_TERM_B_TELA, D_TERM_B_TELA_OK);
  con_es_registra_dispositivo_es(self->com, D_TERM_C_TECLADO, D_TERM_C_TECLADO_OK, D_TERM_C_TELA, D_TERM_C_TELA_OK);
  con_es_registra_dispositivo_es(self->com, D_TERM_D_TECLADO, D_TERM_D_TECLADO_OK, D_TERM_D_TELA, D_TERM_D_TELA_OK);

  // quando a CPU executar uma instrução CHAMAC, deve chamar a função
  //   so_trata_interrupcao, com primeiro argumento um ptr para o SO
  cpu_define_chamaC(self->cpu, so_trata_interrupcao, self);

  // coloca o tratador de interrupção na memória
  // quando a CPU aceita uma interrupção, passa para modo supervisor, 
  //   salva seu estado à partir do endereço 0, e desvia para o endereço
  //   IRQ_END_TRATADOR
  // colocamos no endereço IRQ_END_TRATADOR o programa de tratamento
  //   de interrupção (escrito em asm). esse programa deve conter a 
  //   instrução CHAMAC, que vai chamar so_trata_interrupcao (como
  //   foi definido acima)
  int ender = so_carrega_programa(self, "trata_int.maq");
  if (ender != IRQ_END_TRATADOR) {
    console_printf("SO: problema na carga do programa de tratamento de interrupção");
    self->erro_interno = true;
  }

  // programa o relógio para gerar uma interrupção após INTERVALO_INTERRUPCAO
  if (es_escreve(self->es, D_RELOGIO_TIMER, INTERVALO_INTERRUPCAO) != ERR_OK) {
    console_printf("SO: problema na programação do timer");
    self->erro_interno = true;
  }

  return self;
}

void so_destroi(so_t *self) {
    cpu_define_chamaC(self->cpu, NULL, NULL);
    con_es_destroi(self->com);
    escal_destroi(self->esc);

    // destroi todos os processos
    tabela_process_destruir(&self->processos);


    free(self->processos.tabela);
    free(self);
}


// TRATAMENTO DE INTERRUPÇÃO {{{1

// funções auxiliares para o tratamento de interrupção
static void so_salva_estado_da_cpu(so_t *self);
static void so_sincroniza(so_t *self);
static void so_trata_irq(so_t *self, int irq);
static void so_trata_bloq(so_t *self, process_t *process);
static void so_trata_pendencias(so_t *self);
static void so_escalona(so_t *self);
static int so_despacha(so_t *self);
static int so_desliga(so_t *self);

// função a ser chamada pela CPU quando executa a instrução CHAMAC, no tratador de
//   interrupção em assembly
// essa é a única forma de entrada no SO depois da inicialização
// na inicialização do SO, a CPU foi programada para chamar esta função para executar
//   a instrução CHAMAC
// a instrução CHAMAC só deve ser executada pelo tratador de interrupção
//
// o primeiro argumento é um ponteiro para o SO, o segundo é a identificação
//   da interrupção
// o valor retornado por esta função é colocado no registrador A, e pode ser
//   testado pelo código que está após o CHAMAC. No tratador de interrupção em
//   assembly esse valor é usado para decidir se a CPU deve retornar da interrupção
//   (e executar o código de usuário) ou executar PARA e ficar suspensa até receber
//   outra interrupção
static int so_trata_interrupcao(void *argC, int reg_A)
{
  so_t *self = argC;
  irq_t irq = reg_A;

  self->metricas.n_irqs[irq]++;

  // esse print polui bastante, recomendo tirar quando estiver com mais confiança
  // console_printf("SO: recebi IRQ %d (%s)", irq, irq_nome(irq));
  // salva o estado da cpu no descritor do processo que foi interrompido
  so_salva_estado_da_cpu(self);
  // sincroniza o tempo do sistema
  so_sincroniza(self);
  // faz o atendimento da interrupção
  so_trata_irq(self, irq);
  // faz o processamento independente da interrupção
  so_trata_pendencias(self);
  // escolhe o próximo processo a executar
  so_escalona(self);

  if (so_tem_trabalho(self)) {
    // recupera o estado do processo escolhido
    return so_despacha(self);
  } else {
    // para de executar o SO, desabilitando as interrupções de relógio
    return so_desliga(self);
  }
}

static void so_salva_estado_da_cpu(so_t *self)
{
  // t1: salva os registradores que compõem o estado da cpu no descritor do
  //   processo corrente. os valores dos registradores foram colocados pela
  //   CPU na memória, nos endereços IRQ_END_*
  // se não houver processo corrente, não faz nada

  process_t *process = self->process_corrente;

  if (process == NULL) {
    return;
  }
  
  int PC, A, X;
  mem_le(self->mem, IRQ_END_PC, &PC);
  mem_le(self->mem, IRQ_END_A, &A);
  mem_le(self->mem, IRQ_END_X, &X);

  process_define_PC(process, PC);
  process_define_A(process, A);
  process_define_X(process, X);
}

static void so_sincroniza(so_t *self)
{
  int t_relogio_anterior = self->temporizador.relogio_atual;

  if (es_le(self->es, D_RELOGIO_INSTRUCOES, &self->temporizador.relogio_atual) != ERR_OK) {
    console_printf("SO: problema na leitura do relógio");
    return;
  }

  if (t_relogio_anterior == -1) {
    return;
  }

  int delta = self->temporizador.relogio_atual - t_relogio_anterior;

  so_atualiza_metricas(self, delta);

}

static void so_trata_pendencias(so_t *self)
{
    // t1: realiza ações que não são diretamente ligadas com a interrupção que
    //   está sendo atendida:
    // - E/S pendente
    // - desbloqueio de processos
    // - contabilidades

    process_t **tabela = self->processos.tabela; // cache do ponteiro para a tabela de processos
    int qtd = self->processos.qtd;            // cache do número de processos para evitar acessos repetidos

    for (int i = 0; i < qtd; i++) {
        process_t *process = tabela[i]; // acesso direto a tabela
        if (process_estado(process) == ESTADO_BLOQUEADO) {
            so_trata_bloq(self, process); // trata o processo bloqueado
        }
    }
}


static void so_escalona(so_t *self)
{
  // escolhe o próximo processo a executar, que passa a ser o processo
  //   corrente; pode continuar sendo o mesmo de antes ou não
  // t1: na primeira versão, escolhe um processo caso o processo corrente não possa continuar
  //   executando. depois, implementar escalonador melhor
  if (!so_deve_escalonar(self)) {
    return;
  }

     if (self->process_corrente != NULL) {
        int t_exec = self->temporizador.quantum - self->temporizador.restante;

        double nova_prioridade = process_calcula_prioridade(self->process_corrente, t_exec, self->temporizador.quantum);

        console_printf(
            "SO: processo %d - atualiza prioridade %lf -> %lf",
            process_id(self->process_corrente),
            process_prioridade(self->process_corrente),
            nova_prioridade
        );

        process_define_prioridade(self->process_corrente, nova_prioridade);
    }

    process_t *process = escal_proximo(self->esc);

    if (process != NULL) {
        console_printf("SO: escalona processo %d", process_id(process));
    } else {
        console_printf("SO: nenhum processo para escalonar");
    }

    so_executa_proc(self, process);
}

static int so_despacha(so_t *self)
{
  // t1: se houver processo corrente, coloca o estado desse processo onde ele
  //   será recuperado pela CPU (em IRQ_END_*) e retorna 0, senão retorna 1
  // o valor retornado será o valor de retorno de CHAMAC
  if (self->erro_interno) return 1;

  process_t *process = self->process_corrente;

  if (process == NULL) {
    return 1;
  }
  
  int PC = process_PC(process);
  int A = process_A(process);
  int X = process_X(process);

  mem_escreve(self->mem, IRQ_END_PC, PC);
  mem_escreve(self->mem, IRQ_END_A, A);
  mem_escreve(self->mem, IRQ_END_X, X);

  return 0;
}

static int so_desliga(so_t *self)
{
  err_t e1, e2;
  e1 = es_escreve(self->es, D_RELOGIO_INTERRUPCAO, 0);
  e2 = es_escreve(self->es, D_RELOGIO_TIMER, 0);

  if (e1 != ERR_OK || e2 != ERR_OK) {
    console_printf("SO: problema de desarme do timer");
    self->erro_interno = true;
  }
  so_imprime_metricas(self);

  return 1;
}

// TRATAMENTO DE UM BLOQUEIO {{{1
static void so_trata_bloq_leitura(so_t *self, process_t *process);
static void so_trata_bloq_escrita(so_t *self, process_t *process);
static void so_trata_bloq_espera_proc(so_t *self, process_t *process);
static void so_trata_bloq_desconhecido(so_t *self, process_t *process);

static void so_trata_bloq(so_t *self, process_t *process)
{
  process_bloq_motivo_t motivo = process_bloq_motivo(process);

  switch (motivo)
  {
  case BLOQ_LEITURA:
    so_trata_bloq_leitura(self, process);
    break;
  case BLOQ_ESCRITA:
    so_trata_bloq_escrita(self, process);
    break;
  case BLOQ_ESPERA_PROCESS:
    so_trata_bloq_espera_proc(self, process);
    break;
  default:
    so_trata_bloq_desconhecido(self, process);
    break;
  }
}

static void so_trata_bloq_leitura(so_t *self, process_t *process)
{
    int dispositivo_es = process_dispositivo_es(process);

    // retorna imediatamente se a dispositivo_es for invalida
    if (dispositivo_es == -1) {
        return;
    }

    int dado;

    // le da dispositivo_es e so prossegue se deu certo
    if (!con_es_le_dispositivo_es(self->com, dispositivo_es, &dado)) {
        return;
    }

    // atualiza o registrador e desbloqueia o processo
    process_define_A(process, dado);
    so_desbloqueia_proc(self, process);

    console_printf("SO: processo %d - desbloqueia de leitura", process_id(process));
}


static void so_trata_bloq_escrita(so_t *self, process_t *process)
{
    // garante que o processo tenha a dispositivo_es certa
    so_assegura_dispositivo_es_proc(self, process);

    // pega a dispositivo_es e o argumento para escrita
    int dispositivo_es = process_dispositivo_es(process);

    // retorna se a dispositivo_es for invalida
    if (dispositivo_es == -1) {
        return;
    }

    int dado = process_bloq_arg(process);

    // tenta escrever na dispositivo_es e retorna se falhar
    if (!con_es_escreve_dispositivo_es(self->com, dispositivo_es, dado)) {
        return;
    }

    // desbloqueia o processo e atualiza o registrador A
    process_define_A(process, 0);
    so_desbloqueia_proc(self, process);

    console_printf("SO: processo %d - desbloqueia de escrita", process_id(process));
}


static void so_trata_bloq_espera_proc(so_t *self, process_t *process)
{
    // pega o PID do processo alvo
    int pid_alvo = process_bloq_arg(process);

    // busca o processo alvo
    process_t *process_alvo = so_busca_proc(self, pid_alvo);

    // verifica se o processo alvo nao existe ou esta morto
    if (!process_alvo || process_estado(process_alvo) == MORTO) {
        process_define_A(process, 0);  // marca o processo atual como concluido
        so_desbloqueia_proc(self, process);  // desbloqueia o processo atual

        console_printf(
            "SO: processo %d - desbloqueia de espera de processo",
            process_id(process)
        );
    }
}

static void so_trata_bloq_desconhecido(so_t *self, process_t *process)
{
  console_printf("SO: não sei tratar bloqueio por motivo %d", process_bloq_motivo(process));
  self->erro_interno = true;
}

// TRATAMENTO DE UMA IRQ {{{1

// funções auxiliares para tratar cada tipo de interrupção
static void so_trata_irq_reset(so_t *self);
static void so_trata_irq_chamada_sistema(so_t *self);
static void so_trata_irq_err_cpu(so_t *self);
static void so_trata_irq_relogio(so_t *self);
static void so_trata_irq_desconhecida(so_t *self, int irq);

static void so_trata_irq(so_t *self, int irq)
{
  // verifica o tipo de interrupção que está acontecendo, e atende de acordo
  switch (irq) {
    case IRQ_RESET:
      so_trata_irq_reset(self);
      break;
    case IRQ_SISTEMA:
      so_trata_irq_chamada_sistema(self);
      break;
    case IRQ_ERR_CPU:
      so_trata_irq_err_cpu(self);
      break;
    case IRQ_RELOGIO:
      so_trata_irq_relogio(self);
      break;
    default:
      so_trata_irq_desconhecida(self, irq);
  }
}

// interrupção gerada uma única vez, quando a CPU inicializa
static void so_trata_irq_reset(so_t *self)
{
  // t1: deveria criar um processo para o init, e inicializar o estado do
  //   processador para esse processo com os registradores zerados, exceto
  //   o PC e o modo.
  // como não tem suporte a processos, está carregando os valores dos
  //   registradores diretamente para a memória, de onde a CPU vai carregar
  //   para os seus registradores quando executar a instrução RETI

  // coloca o programa init na memória
  process_t *process = so_cria_proc(self, "init.maq");

  if (process_PC(process) != 100) {
    console_printf("SO: problema na carga do programa inicial");
    self->erro_interno = true;
    return;
  }

  // altera o PC para o endereço de carga
  // mem_escreve(self->mem, IRQ_END_PC, ender);
  // passa o processador para modo usuário
  mem_escreve(self->mem, IRQ_END_modo, usuario);
}

// interrupção gerada quando a CPU identifica um erro
static void so_trata_irq_err_cpu(so_t *self)
{
  // Ocorreu um erro interno na CPU
  // O erro está codificado em IRQ_END_erro
  // Em geral, causa a morte do processo que causou o erro
  // Ainda não temos processos, causa a parada da CPU
  int err_int;
  // t1: com suporte a processos, deveria pegar o valor do registrador erro
  //   no descritor do processo corrente, e reagir de acordo com esse erro
  //   (em geral, matando o processo)
  mem_le(self->mem, IRQ_END_erro, &err_int);
  err_t err = err_int;
  console_printf("SO: IRQ não tratada -- erro na CPU: %s", err_nome(err));
  self->erro_interno = true;
}

// interrupção gerada quando o timer expira
static void so_trata_irq_relogio(so_t *self)
{
  // rearma o interruptor do relógio e reinicializa o timer para a próxima interrupção
  err_t e1, e2;
  e1 = es_escreve(self->es, D_RELOGIO_INTERRUPCAO, 0); // desliga o sinalizador de interrupção
  e2 = es_escreve(self->es, D_RELOGIO_TIMER, INTERVALO_INTERRUPCAO);
  if (e1 != ERR_OK || e2 != ERR_OK) {
    console_printf("SO: problema da reinicialização do timer");
    self->erro_interno = true;
  }
  // t1: deveria tratar a interrupção
  //   por exemplo, decrementa o quantum do processo corrente, quando se tem
  //   um escalonador com quantum

  console_printf("SO: interrupção do relógio");

  if (self->temporizador.restante > 0) {
    self->temporizador.restante--;
  }
}

// foi gerada uma interrupção para a qual o SO não está preparado
static void so_trata_irq_desconhecida(so_t *self, int irq)
{
  console_printf("SO: não sei tratar IRQ %d (%s)", irq, irq_nome(irq));
  self->erro_interno = true;
}

// CHAMADAS DE SISTEMA {{{1

// funções auxiliares para cada chamada de sistema
static void so_chamada_le(so_t *self);
static void so_chamada_escr(so_t *self);
static void so_chamada_cria_proc(so_t *self);
static void so_chamada_mata_proc(so_t *self);
static void so_chamada_espera_proc(so_t *self);

static void so_trata_irq_chamada_sistema(so_t *self)
{
  // a identificação da chamada está no registrador A
  // t1: com processos, o reg A tá no descritor do processo corrente
  int id_chamada;
  if (mem_le(self->mem, IRQ_END_A, &id_chamada) != ERR_OK) {
    console_printf("SO: erro no acesso ao id da chamada de sistema");
    self->erro_interno = true;
    return;
  }
  // console_printf("SO: chamada de sistema %d", id_chamada);
  switch (id_chamada) {
    case SO_LE:
      so_chamada_le(self);
      break;
    case SO_ESCR:
      so_chamada_escr(self);
      break;
    case SO_CRIA_PROC:
      so_chamada_cria_proc(self);
      break;
    case SO_MATA_PROC:
      so_chamada_mata_proc(self);
      break;
    case SO_ESPERA_PROC:
      so_chamada_espera_proc(self);
      break;
    default:
      console_printf("SO: chamada de sistema desconhecida (%d)", id_chamada);
      // t1: deveria matar o processo
      self->erro_interno = true;
  }
}

// implementação da chamada se sistema SO_LE
// faz a leitura de um dado da entrada corrente do processo, coloca o dado no reg A
static void so_chamada_le(so_t *self)
{
    // implementação com espera ocupada
    //   T1: deveria realizar a leitura somente se a entrada estiver disponível,
    //     senão, deveria bloquear o processo.
    //   no caso de bloqueio do processo, a leitura (e desbloqueio) deverá
    //     ser feita mais tarde, em tratamentos pendentes em outra interrupção,
    //     ou diretamente em uma interrupção específica do dispositivo, se for
    //     o caso
    // implementação lendo direto do terminal A
    //   T1: deveria usar dispositivo de entrada corrente do processo

    // pega o processo corrente
    process_t *process = self->process_corrente;

    // retorna se nao houver processo corrente
    if (!process) {
        return;
    }

    // assegura que o processo esta associado a uma dispositivo_es valida
    so_assegura_dispositivo_es_proc(self, process);

    // pega a dispositivo_es associada ao processo
    int dispositivo_es = process_dispositivo_es(process);

    // verifica se a dispositivo_es é valida
    if (dispositivo_es == -1) {
        console_printf("SO: processo %d - sem dispositivo_es válida para leitura", process_id(process));
        return;
    }

    // tenta realizar a leitura diretamente
    int dado;
    if (con_es_le_dispositivo_es(self->com, dispositivo_es, &dado)) {
        // leitura certa: armazena o dado no registrador A do processo
        process_define_A(process, dado);
    } else {
        // Leitura nao disponivel: bloqueia o processo para leitura futura
        console_printf("SO: processo %d - bloqueia para leitura", process_id(process));
        so_bloqueia_proc(self, process, BLOQ_LEITURA, 0);
    }
}

// implementação da chamada se sistema SO_ESCR
// escreve o valor do reg X na saída corrente do processo
static void so_chamada_escr(so_t *self)
{
    // implementação com espera ocupada
    //   T1: deveria bloquear o processo se dispositivo ocupado
    // implementação escrevendo direto do terminal A
    //   T1: deveria usar o dispositivo de saída corrente do processo

    // pega o processo corrente
    process_t *process = self->process_corrente;

    // retorna imediatamente se nao houver processo corrente
    if (!process) {
        return;
    }

    // assegura que o processo esta associado a uma dispositivo_es valida
    so_assegura_dispositivo_es_proc(self, process);

    // pega a dispositivo_es associada ao processo
    int dispositivo_es = process_dispositivo_es(process);

    // verifica se a dispositivo_es é valida antes de tentar escrever
    if (dispositivo_es == -1) {
        console_printf("SO: processo %d - sem dispositivo_es válida para escrita", process_id(process));
        return;
    }

    // pega o valor a ser escrito do registrador X do processo
    int dado = process_X(process);

    // tenta realizar a escrita na dispositivo_es
    if (con_es_escreve_dispositivo_es(self->com, dispositivo_es, dado)) {
        // escrita certa: armazena 0 no registrador A do processo
        process_define_A(process, 0);
    } else {
        // escrita nao disponivel: bloqueia o processo para escrita futura
        console_printf("SO: processo %d - bloqueia para escrita", process_id(process));
        so_bloqueia_proc(self, process, BLOQ_ESCRITA, dado);
    }
}


// implementação da chamada se sistema SO_CRIA_PROC
// cria um processo
static void so_chamada_cria_proc(so_t *self)
{
    // ainda sem suporte a processos, carrega programa e passa a executar ele
    // quem chamou o sistema não vai mais ser executado, coitado!
    // T1: deveria criar um novo processo

    // pega o processo corrente
    process_t *process = self->process_corrente;

    // retorna se nao houver processo corrente
    if (!process) {
        return;
    }

    // T1: deveria ler o X do descritor do processo criador
    int ender_proc = process_X(process);
    char nome[100];

    // tenta copiar o nome do arquivo da memoria do processo
    if (!copia_str_da_mem(100, nome, self->mem, ender_proc)) {
        // erro ao copiar o nome do arquivo
        console_printf(
            "SO: processo %d - erro ao tentar criar processo (endereço inválido ou nome muito longo)",
            process_id(process)
        );
        process_define_A(process, -1); // indica erro no registrador A
        return;
    }

    // exibe log indicando tentativa de criacao do processo
    console_printf(
        "SO: processo %d - tenta criar processo (nome: %s)",
        process_id(process),
        nome
    );

    // T1: deveria gerar um novo processo
    process_t *process_alvo = so_cria_proc(self, nome);

    if (process_alvo) {
        // processo criado com sucesso
        process_define_A(process, process_id(process_alvo)); // escreve o PID do novo processo
        return;
    }

    // caso não tenha sido possível criar o processo
    console_printf(
        "SO: processo %d - falha ao criar processo (nome: %s)",
        process_id(process),
        nome
    );
    process_define_A(process, -1); // indica erro no registrador A
}


// implementação da chamada se sistema SO_MATA_PROC
// mata o processo com pid X (ou o processo corrente se X é 0)
static void so_chamada_mata_proc(so_t *self)
{
    // pega o processo corrente
    process_t *process = self->process_corrente;

    // retorna se nao houver processo corrente
    if (!process) {
        return;
    }

    // T1: deveria matar um processo
    int pid_alvo = process_X(process);

    // determina o processo alvo; PID 0 refere ao processo corrente
    process_t *process_alvo = (pid_alvo == 0) ? process : so_busca_proc(self, pid_alvo);

    console_printf(
        "SO: processo %d - tenta matar processo (PID: %d)",
        process_id(process),
        pid_alvo
    );

    // verifica se o processo alvo existe
    if (process_alvo) {
        so_mata_proc(self, process_alvo);  // mata o processo alvo
        process_define_A(process, 0);        // indica sucesso no registrador A
        console_printf(
            "SO: processo %d - processo (PID: %d) morto com sucesso",
            process_id(process),
            pid_alvo
        );
    } else {
        process_define_A(process, -1);       // indica erro no registrador A
        console_printf(
            "SO: processo %d - falha ao tentar matar processo (PID: %d, não encontrado)",
            process_id(process),
            pid_alvo
        );
    }
}


// implementação da chamada se sistema SO_ESPERA_PROC
// espera o fim do processo com pid X
static void so_chamada_espera_proc(so_t *self)
{
  process_t *process = self->process_corrente;

  if (!process) {
        return;
    }

  // T1: deveria bloquear o processo se for o caso (e desbloquear na morte do esperado)
  // ainda sem suporte a processos, retorna erro -1
  int pid_alvo = process_X(process);
  process_t *process_alvo = so_busca_proc(self, pid_alvo);

  console_printf(
    "SO: processo %d - espera processo (PID: %d)",
    process_id(process),
    pid_alvo
  );

  // caso de erro: processo alvo inexistente ou é o proprio processo
  if (process_alvo == NULL || process_alvo == process) {
    process_define_A(process, -1);
    return;
  }

  // verifica se o processo alvo ja esta morto
  if (process_estado(process_alvo) == MORTO) {
    process_define_A(process, 0);
    return;
  }

  // caso padrao: bloqueia o processo atual ate que o alvo termine
  console_printf(
    "SO: processo %d - bloqueia para espera de processo",
    process_id(process)
  );

  so_bloqueia_proc(self, process, BLOQ_ESPERA_PROCESS, pid_alvo);
}


// GERENCIAMENTO DE PROCESSOS{{{1
static process_t *so_busca_proc(so_t *self, int pid) {
    if (pid <= 0) {
        return NULL; // IDs invalidos
    }

    // busca o processo diretamente pela tabela
    for (int i = 0; i < self->processos.qtd; i++) {
        process_t *process = self->processos.tabela[i];
        if (process_id(process) == pid) {
            return process;
        }
    }

    return NULL; // nao encontrado
}


static bool tabela_process_adiciona(tabela_process_t *tabela, process_t *process) {
    if (tabela->qtd == tabela->capacidade) {
        int nova_capacidade = tabela->capacidade > 0 ? tabela->capacidade + (tabela->capacidade / 2) : 4;

        // reraloca a tabela com alinhamento apropriado para evitar fragmentacao de cache
        process_t **novo_tabela = realloc(tabela->tabela, nova_capacidade * sizeof(process_t *));
        if (novo_tabela == NULL) {
            return false;
        }

        tabela->tabela = novo_tabela;
        tabela->capacidade = nova_capacidade;
    }

    tabela->tabela[tabela->qtd++] = process;
    return true;
}


static process_t *so_cria_proc(so_t *self, char *nome_do_executavel) {
    int end = so_carrega_programa(self, nome_do_executavel);
    if (end <= 0) {
        return NULL;
    }

    // criar o processo
    process_t *process = process_cria(self->processos.qtd + 1, end);
    if (process == NULL) {
        return NULL; // falha na criacao do processo
    }

    if (!tabela_process_adiciona(&self->processos, process)) {
        process_destroi(process); // limpa memoria em caso de falha
        return NULL;
    }

    // inserir no escalonador
    escal_insere_proc(self->esc, process);

    return process;
}




static void so_mata_proc(so_t *self, process_t *process)
{
    if (!process) return; // nenhuma operacao com ponteiro nulo

    int dispositivo_es = process_dispositivo_es(process);
    if (dispositivo_es != -1) {
        con_es_libera_dispositivo_es(self->com, dispositivo_es);
        process_desatribui_dispositivo_es(process);
    }

    // remove o processo da estrutura e encerra diretamente
    escal_remove_proc(self->esc, process);
    process_encerra(process);
}

static void so_executa_proc(so_t *self, process_t *process)
{
    // processo ja é o atual, apenas reseta o temporizador e sai
    if (self->process_corrente == process) {
        self->temporizador.restante = self->temporizador.quantum;
        return;
    }

    // se ha um processo em execucao, pare e registre a preempcao
    if (self->process_corrente && process_estado(self->process_corrente) == ESTADO_EXECUTANDO) {
        process_para(self->process_corrente);
        self->metricas.n_preempcoes++;
    }

    // se o novo processo nao esta executando, inicie sua execucao
    if (process && process_estado(process) != ESTADO_EXECUTANDO) {
        process_executa(process);
    }

    // atualiza os estados compartilhados
    self->process_corrente = process;
    self->temporizador.restante = self->temporizador.quantum;
}


static void so_bloqueia_proc(so_t *self, process_t *process, process_bloq_motivo_t motivo, int arg)
{
    if (process != NULL && process_estado(process) != ESTADO_BLOQUEADO) {
        escal_remove_proc(self->esc, process);
        process_bloqueia(process, motivo, arg);
    }
}

static void so_desbloqueia_proc(so_t *self, process_t *process)
{
    if (process != NULL && process_estado(process) == ESTADO_BLOQUEADO) {
        process_desbloqueia(process);
        escal_insere_proc(self->esc, process);
    }
}

static void so_assegura_dispositivo_es_proc(so_t *self, process_t *process)
{
    if (process != NULL && process_dispositivo_es(process) == -1) {
        int dispositivo_es = con_es_dispositivo_es_disponivel(self->com);
        if (dispositivo_es != -1) {
            process_atribui_dispositivo_es(process, dispositivo_es);
            con_es_reserva_dispositivo_es(self->com, dispositivo_es);
        }
    }
}

static bool so_deve_escalonar(so_t *self)
{
    if (!self->process_corrente ||
        process_estado(self->process_corrente) != ESTADO_EXECUTANDO ||
        self->temporizador.restante <= 0)
    {
        return true;
    }
    return false;
}

static bool so_tem_trabalho(so_t *self)
{
    process_t **tabela = self->processos.tabela;
    for (int i = 0, qtd = self->processos.qtd; i < qtd; i++) {
        if (process_estado(tabela[i]) != MORTO) {
            return true;
        }
    }
    return false;
}


// MÉTRICAS {{1

static void so_atualiza_metricas(so_t *self, int delta)
{
  self->metricas.t_exec += delta;
  
  if (self->process_corrente == NULL) {
    self->metricas.t_ocioso += delta;
  }

  for (int i = 0; i < self->processos.qtd; i++) {
    process_atualiza_metricas(self->processos.tabela[i], delta);
  }
}


static void so_imprime_metricas(so_t *self) {
    FILE *file = fopen("metricas.md", "a"); // Modo de abertura em append
    if (file == NULL) {
        perror("Erro ao abrir o arquivo para escrita");
        return;
    }

    fprintf(file, "\n# Métricas do Sistema Operacional\n\n");

    // Informações do sistema operacional
    fprintf(file, "## Informações Gerais\n");
    fprintf(file, "| Número de Processos | Tempo de Execução | Tempo Ocioso | Número de Preempções |\n");
    fprintf(file, "|---------------------|-------------------|--------------|----------------------|\n");
    fprintf(file, "| %d | %d | %d | %d |\n",
            self->processos.qtd,
            self->metricas.t_exec,
            self->metricas.t_ocioso,
            self->metricas.n_preempcoes);

    // Informações de interrupções
    fprintf(file, "\n## Interrupções\n");
    fprintf(file, "| IRQ | Número de Ocorrências |\n");
    fprintf(file, "|-----|------------------------|\n");
    for (int i = 0; i < N_IRQ; i++) {
        fprintf(file, "| %d | %d |\n", i, self->metricas.n_irqs[i]);
    }

    // Métricas detalhadas dos processos
    fprintf(file, "\n## Métricas dos Processos\n");
    fprintf(file, "| PID | Número de Preempções | Tempo de Retorno | Tempo de Resposta |\n");
    fprintf(file, "|-----|-----------------------|------------------|-------------------|\n");
    for (int i = 0; i < self->processos.qtd; i++) {
        process_t *proc = self->processos.tabela[i];

        process_metricas_t proc_metricas_atual = process_metricas(proc);

        int pid = process_id(proc);                  // ID do processo
        int n_preempcoes = proc_metricas_atual.n_preempcoes; // Número de preempções
        int tempo_retorno = proc_metricas_atual.t_retorno;   // Tempo de retorno
        int t_resposta = proc_metricas_atual.t_resposta;     // Tempo de resposta

        fprintf(file, "| %d | %d | %d | %d |\n", pid, n_preempcoes, tempo_retorno, t_resposta);
    }

    // Métricas dos estados dos processos (vezes)
    fprintf(file, "\n## Número de Vezes por Estado\n");
    fprintf(file, "| PID ");
    for (int i = 0; i < N_ESTADO; i++) {
        fprintf(file, "| N_%s ", process_estado_nome(i));
    }
    fprintf(file, "|\n");

    fprintf(file, "|-----");
    for (int i = 0; i < N_ESTADO; i++) {
        fprintf(file, "|------");
    }
    fprintf(file, "|\n");

    for (int i = 0; i < self->processos.qtd; i++) {
        process_t *proc = self->processos.tabela[i];
        process_metricas_t proc_metricas_atual = process_metricas(proc);

        fprintf(file, "| %d ", process_id(proc));
        for (int j = 0; j < N_ESTADO; j++) {
            fprintf(file, "| %d ", proc_metricas_atual.estados[j].n_vezes);
        }
        fprintf(file, "|\n");
    }

    // Métricas dos estados dos processos (tempo)
    fprintf(file, "\n## Tempo por Estado\n");
    fprintf(file, "| PID ");
    for (int i = 0; i < N_ESTADO; i++) {
        fprintf(file, "| T_%s ", process_estado_nome(i));
    }
    fprintf(file, "|\n");

    fprintf(file, "|-----");
    for (int i = 0; i < N_ESTADO; i++) {
        fprintf(file, "|------");
    }
    fprintf(file, "|\n");

    for (int i = 0; i < self->processos.qtd; i++) {
        process_t *proc = self->processos.tabela[i];
        process_metricas_t proc_metricas_atual = process_metricas(proc);

        fprintf(file, "| %d ", process_id(proc));
        for (int j = 0; j < N_ESTADO; j++) {
            fprintf(file, "| %d ", proc_metricas_atual.estados[j].t_total);
        }
        fprintf(file, "|\n");
    }

    fclose(file);
    printf("Métricas adicionadas ao arquivo 'metricas.md'.\n");
}



// CARGA DE PROGRAMA {{{1

// carrega o programa na memória
// retorna o endereço de carga ou -1
static int so_carrega_programa(so_t *self, char *nome_do_executavel)
{
  // programa para executar na nossa CPU
  programa_t *prog = prog_cria(nome_do_executavel);
  if (prog == NULL) {
    console_printf("Erro na leitura do programa '%s'\n", nome_do_executavel);
    return -1;
  }

  int end_ini = prog_end_carga(prog);
  int end_fim = end_ini + prog_tamanho(prog);

  for (int end = end_ini; end < end_fim; end++) {
    if (mem_escreve(self->mem, end, prog_dado(prog, end)) != ERR_OK) {
      console_printf("Erro na carga da memória, endereco %d\n", end);
      return -1;
    }
  }

  prog_destroi(prog);
  console_printf("SO: carga de '%s' em %d-%d", nome_do_executavel, end_ini, end_fim);
  return end_ini;
}

// ACESSO À MEMÓRIA DOS PROCESSOS {{{1

// copia uma string da memória do simulador para o vetor str.
// retorna false se erro (string maior que vetor, valor não char na memória,
//   erro de acesso à memória)
// T1: deveria verificar se a memória pertence ao processo
static bool copia_str_da_mem(int tam, char str[tam], mem_t *mem, int ender)
{
  for (int indice_str = 0; indice_str < tam; indice_str++) {
    int caractere;
    if (mem_le(mem, ender + indice_str, &caractere) != ERR_OK) {
      return false;
    }
    if (caractere < 0 || caractere > 255) {
      return false;
    }
    str[indice_str] = caractere;
    if (caractere == 0) {
      return true;
    }
  }
  // estourou o tamanho de str
  return false;
}

// vim: foldmethod=marker
