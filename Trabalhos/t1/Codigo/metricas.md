# Introdução
Este documento descreve a implementação de um sistema operacional simples com suporte a múltiplos processos. A implementação é dividida em três partes principais:

1. **Parte I: Implementação Inicial de Processos**
    - Criação de uma estrutura de dados para representar processos.
    - Inicialização de uma tabela de processos.
    - Implementação de funções para salvar e recuperar o estado do processador.
    - Implementação de um escalonador básico.
    - Implementação de chamadas de criação e término de processos.
    - Modificação das chamadas de E/S para usar terminais diferentes com base no PID do processo.

2. **Parte II: Bloqueio de Processos e Eliminação de Espera Ocupada**
    - Implementação de bloqueio de processos nas chamadas de E/S.
    - Verificação do estado dos dispositivos e desbloqueio de processos conforme necessário.
    - Implementação da chamada de sistema `SO_ESPERA_PROC` para bloquear processos até que outro processo termine.

3. **Parte III: Escalonador Preemptivo Round-Robin e Escalonador com Prioridade**
    - Implementação de um escalonador round-robin preemptivo.
    - Implementação de um escalonador com prioridade, onde a prioridade é ajustada com base no tempo de execução e no quantum.


    As métricas a seguir foram obtidas utilizando diferentes escalonadores (Simples, Round-Robin e Prioritário) e variando o intervalo de relógio entre 50 e 100, e o quantum entre 5 e 10:

    1. **SIMPLES**
        - **INTERVALO_RELOGIO = 50 e INTERVALO_QUANTUM = 5**
        - **INTERVALO_RELOGIO = 100 e INTERVALO_QUANTUM = 10**

    2. **CIRCULAR (Round-Robin)**
        - **INTERVALO_RELOGIO = 50 e INTERVALO_QUANTUM = 5**
        - **INTERVALO_RELOGIO = 100 e INTERVALO_QUANTUM = 10**

    3. **PRIORITÁRIO**
        - **INTERVALO_RELOGIO = 50 e INTERVALO_QUANTUM = 5**
        - **INTERVALO_RELOGIO = 100 e INTERVALO_QUANTUM = 10**

# Métricas do Sistema Operacional SIMPLES INTERVALO_RELOGIO = 50 e INTERVALO_QUANTUM = 5

# Métricas do Sistema Operacional

## Informações Gerais
| Número de Processos | Tempo de Execução | Tempo Ocioso | Número de Preempções |
|---------------------|-------------------|--------------|----------------------|
| 4 | 27885 | 8503 | 0 |

## Interrupções
| IRQ | Número de Ocorrências |
|-----|------------------------|
| 0 | 1 |
| 1 | 0 |
| 2 | 462 |
| 3 | 556 |
| 4 | 0 |
| 5 | 0 |

## Métricas dos Processos
| PID | Número de Preempções | Tempo de Retorno | Tempo de Resposta |
|-----|-----------------------|------------------|-------------------|
| 1 | 0 | 27885 | 10 |
| 2 | 0 | 13414 | 552 |
| 3 | 0 | 15775 | 503 |
| 4 | 0 | 27167 | 95 |

## Número de Vezes por Estado
| PID | N_Pronto | N_Executando | N_Bloqueado | N_Morto |
|-----|------|------|------|------|
| 1 | 4 | 4 | 3 | 1 |
| 2 | 7 | 7 | 6 | 1 |
| 3 | 21 | 21 | 20 | 1 |
| 4 | 131 | 131 | 130 | 1 |

## Tempo por Estado
| PID | T_Pronto | T_Executando | T_Bloqueado | T_Morto |
|-----|------|------|------|------|
| 1 | 743 | 43 | 27099 | 0 |
| 2 | 9156 | 3864 | 394 | 14088 |
| 3 | 3774 | 10575 | 1426 | 11719 |
| 4 | 5709 | 12449 | 9009 | 319 |


# Métricas do Sistema Operacional SIMPLES INTERVALO_RELOGIO = 100 e INTERVALO_QUANTUM = 10

## Informações Gerais
| Número de Processos | Tempo de Execução | Tempo Ocioso | Número de Preempções |
|---------------------|-------------------|--------------|----------------------|
| 4 | 27541 | 8753 | 0 |

## Interrupções
| IRQ | Número de Ocorrências |
|-----|------------------------|
| 0 | 1 |
| 1 | 0 |
| 2 | 462 |
| 3 | 275 |
| 4 | 0 |
| 5 | 0 |

## Métricas dos Processos
| PID | Número de Preempções | Tempo de Retorno | Tempo de Resposta |
|-----|-----------------------|------------------|-------------------|
| 1 | 0 | 27541 | 18 |
| 2 | 0 | 13001 | 601 |
| 3 | 0 | 15352 | 489 |
| 4 | 0 | 26844 | 92 |

## Número de Vezes por Estado
| PID | N_Pronto | N_Executando | N_Bloqueado | N_Morto |
|-----|------|------|------|------|
| 1 | 4 | 4 | 3 | 1 |
| 2 | 6 | 6 | 5 | 1 |
| 3 | 21 | 21 | 20 | 1 |
| 4 | 131 | 131 | 130 | 1 |

## Tempo por Estado
| PID | T_Pronto | T_Executando | T_Bloqueado | T_Morto |
|-----|------|------|------|------|
| 1 | 719 | 73 | 26749 | 0 |
| 2 | 8877 | 3611 | 513 | 14169 |
| 3 | 3648 | 10274 | 1430 | 11810 |
| 4 | 5544 | 12078 | 9222 | 310 |


# Métricas do Sistema Operacional CIRCULAR INTERVALO_RELOGIO = 50 e INTERVALO_QUANTUM = 5
# Métricas do Sistema Operacional

## Informações Gerais
| Número de Processos | Tempo de Execução | Tempo Ocioso | Número de Preempções |
|---------------------|-------------------|--------------|----------------------|
| 4 | 25355 | 5931 | 43 |

## Interrupções
| IRQ | Número de Ocorrências |
|-----|------------------------|
| 0 | 1 |
| 1 | 0 |
| 2 | 462 |
| 3 | 505 |
| 4 | 0 |
| 5 | 0 |

## Métricas dos Processos
| PID | Número de Preempções | Tempo de Retorno | Tempo de Resposta |
|-----|-----------------------|------------------|-------------------|
| 1 | 0 | 25355 | 5 |
| 2 | 27 | 16353 | 184 |
| 3 | 11 | 12311 | 324 |
| 4 | 5 | 24637 | 94 |

## Número de Vezes por Estado
| PID | N_Pronto | N_Executando | N_Bloqueado | N_Morto |
|-----|------|------|------|------|
| 1 | 3 | 3 | 2 | 1 |
| 2 | 36 | 36 | 8 | 1 |
| 3 | 24 | 24 | 12 | 1 |
| 4 | 119 | 119 | 113 | 1 |

## Tempo por Estado
| PID | T_Pronto | T_Executando | T_Bloqueado | T_Morto |
|-----|------|------|------|------|
| 1 | 743 | 17 | 24595 | 0 |
| 2 | 9180 | 6624 | 549 | 8619 |
| 3 | 3753 | 7793 | 765 | 12653 |
| 4 | 5748 | 11260 | 7629 | 319 |


# Métricas do Sistema Operacional CIRCULAR INTERVALO_RELOGIO = 100 e INTERVALO_QUANTUM = 10

# Métricas do Sistema Operacional

## Informações Gerais
| Número de Processos | Tempo de Execução | Tempo Ocioso | Número de Preempções |
|---------------------|-------------------|--------------|----------------------|
| 4 | 27037 | 8237 | 9 |

## Interrupções
| IRQ | Número de Ocorrências |
|-----|------------------------|
| 0 | 1 |
| 1 | 0 |
| 2 | 462 |
| 3 | 270 |
| 4 | 0 |
| 5 | 0 |

## Métricas dos Processos
| PID | Número de Preempções | Tempo de Retorno | Tempo de Resposta |
|-----|-----------------------|------------------|-------------------|
| 1 | 0 | 27037 | 9 |
| 2 | 7 | 14256 | 348 |
| 3 | 1 | 14948 | 559 |
| 4 | 1 | 26340 | 89 |

## Número de Vezes por Estado
| PID | N_Pronto | N_Executando | N_Bloqueado | N_Morto |
|-----|------|------|------|------|
| 1 | 4 | 4 | 3 | 1 |
| 2 | 14 | 14 | 6 | 1 |
| 3 | 18 | 18 | 16 | 1 |
| 4 | 130 | 130 | 128 | 1 |

## Tempo por Estado
| PID | T_Pronto | T_Executando | T_Bloqueado | T_Morto |
|-----|------|------|------|------|
| 1 | 719 | 37 | 26281 | 0 |
| 2 | 8892 | 4884 | 480 | 12410 |
| 3 | 3645 | 10078 | 1225 | 11710 |
| 4 | 5544 | 11685 | 9111 | 310 |


# Métricas do Sistema Operacional Prioritário INTERVALO_RELOGIO = 50 e INTERVALO_QUANTUM = 5
# Métricas do Sistema Operacional

## Informações Gerais
| Número de Processos | Tempo de Execução | Tempo Ocioso | Número de Preempções |
|---------------------|-------------------|--------------|----------------------|
| 4 | 25283 | 5856 | 22 |

## Interrupções
| IRQ | Número de Ocorrências |
|-----|------------------------|
| 0 | 1 |
| 1 | 0 |
| 2 | 462 |
| 3 | 504 |
| 4 | 0 |
| 5 | 0 |

## Métricas dos Processos
| PID | Número de Preempções | Tempo de Retorno | Tempo de Resposta |
|-----|-----------------------|------------------|-------------------|
| 1 | 0 | 25283 | 0 |
| 2 | 16 | 16437 | 279 |
| 3 | 6 | 14568 | 459 |
| 4 | 0 | 24565 | 99 |

## Número de Vezes por Estado
| PID | N_Pronto | N_Executando | N_Bloqueado | N_Morto |
|-----|------|------|------|------|
| 1 | 3 | 3 | 2 | 1 |
| 2 | 24 | 24 | 7 | 1 |
| 3 | 21 | 21 | 14 | 1 |
| 4 | 114 | 114 | 113 | 1 |

## Tempo por Estado
| PID | T_Pronto | T_Executando | T_Bloqueado | T_Morto |
|-----|------|------|------|------|
| 1 | 743 | 0 | 24540 | 0 |
| 2 | 9174 | 6704 | 559 | 8463 |
| 3 | 3795 | 9644 | 1129 | 10324 |
| 4 | 5715 | 11341 | 7509 | 319 |

# Métricas do Sistema Operacional Prioritário INTERVALO_RELOGIO = 100 e INTERVALO_QUANTUM = 10

# Métricas do Sistema Operacional

## Informações Gerais
| Número de Processos | Tempo de Execução | Tempo Ocioso | Número de Preempções |
|---------------------|-------------------|--------------|----------------------|
| 4 | 27243 | 8452 | 0 |

## Interrupções
| IRQ | Número de Ocorrências |
|-----|------------------------|
| 0 | 1 |
| 1 | 0 |
| 2 | 462 |
| 3 | 272 |
| 4 | 0 |
| 5 | 0 |

## Métricas dos Processos
| PID | Número de Preempções | Tempo de Retorno | Tempo de Resposta |
|-----|-----------------------|------------------|-------------------|
| 1 | 0 | 27243 | 0 |
| 2 | 0 | 13119 | 628 |
| 3 | 0 | 15474 | 495 |
| 4 | 0 | 26546 | 89 |

## Número de Vezes por Estado
| PID | N_Pronto | N_Executando | N_Bloqueado | N_Morto |
|-----|------|------|------|------|
| 1 | 4 | 4 | 3 | 1 |
| 2 | 6 | 6 | 5 | 1 |
| 3 | 21 | 21 | 20 | 1 |
| 4 | 132 | 132 | 131 | 1 |

## Tempo por Estado
| PID | T_Pronto | T_Executando | T_Bloqueado | T_Morto |
|-----|------|------|------|------|
| 1 | 719 | 0 | 26524 | 0 |
| 2 | 8874 | 3769 | 476 | 13753 |
| 3 | 3654 | 10398 | 1422 | 11390 |
| 4 | 5544 | 11772 | 9230 | 310 |


# **Análise e Comparação dos Sistemas Operacionais Simulados**

---

## **Comparação Geral**

### **Tempo de Execução e Tempo Ocioso**
- **Intervalo Relógio SIMPLES (50, Quantum 5):**
  - Tempo de execução: 27541 ms
  - Tempo ocioso: 8753 ms  
- **Intervalo Relógio SIMPLES (100, Quantum 10):**
  - Tempo de execução: 27885 ms  
  - Tempo ocioso: 8503 ms  
- **Circular (50, Quantum 5):**
  - Tempo de execução: 25355 ms  
  - Tempo ocioso: 5931 ms  
- **Circular (100, Quantum 10):**
  - Tempo de execução: 27037 ms  
  - Tempo ocioso: 8237 ms  
- **Prioritário (50, Quantum 5):**
  - Tempo de execução: 25283 ms  
  - Tempo ocioso: 5856 ms  
- **Prioritário (100, Quantum 10):**
  - Tempo de execução: 27243 ms  
  - Tempo ocioso: 8452 ms  

**Conclusão Parcial:**  
Os sistemas circulares e prioritários com intervalo de relógio menor (50) demonstraram menor tempo de execução e ocioso. Isso indica maior eficiência no uso de CPU para esses casos, embora possam causar mais interrupções.

---

### **Número de Preempções**
- **SIMPLES (100, Quantum 10 e 5):** 0 preempções em ambos os casos.
- **Circular (50, Quantum 5):** 43 preempções.
- **Circular (100, Quantum 10):** 9 preempções.
- **Prioritário (50, Quantum 5):** 22 preempções.
- **Prioritário (100, Quantum 10):** 0 preempções.

**Conclusão Parcial:**  
Os algoritmos que utilizam abordagem circular apresentaram maior número de preempções, devido à alternância frequente entre os processos.

---

### **Tempo de Retorno e Tempo de Resposta**
#### Melhor tempo médio de retorno:
- **Circular (50, Quantum 5):**  
  Média: (25355 + 16353 + 12311 + 24637) / 4 = **19664 ms**  

#### Melhor tempo médio de resposta:
- **Circular (50, Quantum 5):**  
  Média: (5 + 184 + 324 + 94) / 4 = **151 ms**

**Conclusão Parcial:**  
O sistema circular com intervalos menores oferece os melhores tempos médios de retorno e resposta, beneficiando processos interativos.

---

### **Interrupções**
- **IRQ 3 (Timer):**  
  - O número de ocorrências varia diretamente com o intervalo de relógio e o quantum. Intervalos menores aumentam a quantidade de interrupções.  

**Conclusão Parcial:**  
Intervalos menores de relógio resultam em maior sobrecarga no sistema devido ao número mais elevado de interrupções, impactando a eficiência global.

---

## **Conclusões Finais**

1. **Sistemas circulares com menor intervalo (50 ms) e menor quantum (5 ms)** mostraram-se os mais eficientes para retorno e resposta de processos, embora com maior número de interrupções e preempções.
2. **Sistemas simples (100 ms)** apresentaram menor sobrecarga no sistema (0 preempções) e são mais adequados para cargas mais estáveis.
3. **Sistemas prioritários** equilibram eficiência e desempenho, sendo indicados para ambientes onde processos com maior prioridade devem ser favorecidos.
