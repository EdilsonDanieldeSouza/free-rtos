# Controle de Potência e Espectroscopia de Impedância Eletroquímica (EIS) com FreeRTOS

Firmware para o DSP **TMS320F28379D** (Texas Instruments) que executa, simultaneamente, o controle em malha fechada de um conversor de potência e um algoritmo de varredura de frequência para diagnóstico eletroquímico (EIS). A aplicação é estruturada sobre o **FreeRTOS** e serve também como trabalho da disciplina de Sistemas Embarcados, atendendo aos requisitos de tarefas hard e soft real-time, shell de diagnóstico, sinalização visual e interação por botão físico.

---

## 1. O problema e as restrições temporais

O sistema precisa conciliar **três naturezas de tempo completamente distintas**, sem que as camadas de menor prioridade interfiram no determinismo das mais críticas — e, neste projeto, isso é resolvido distribuindo essas naturezas entre os dois núcleos do DSP:

1. **Malha de controle crítica — hard real-time, escala de microssegundos.** O controle PI de corrente do conversor precisa amostrar os sensores e atualizar o duty cycle do PWM rigidamente a cada **10 µs (100 kHz)**. Jitter nesta malha pode levar à saturação do indutor e, em casos extremos, dano ao hardware de potência.
2. **Demodulação síncrona do EIS — hard real-time, escala de milissegundos.** Para cada step de frequência do sweep, é necessário extrair magnitude e fase do sinal a partir das somas acumuladas de correlação seno/cosseno. Isso envolve `sqrt` e `atan2`, operações caras demais para caber na janela de 10 µs da ISR de controle.
3. **Interface do supervisório UART — soft real-time, escala de milissegundos/segundos.** A comunicação com o PC (telemetria e recepção de parâmetros como K_p/K_i) é, por natureza, lenta e irregular. Ela não pode, em hipótese alguma, bloquear ou atrasar a malha de potência.

A decisão arquitetural central do projeto é: **o que roda isolado na CPU1 (bare-metal), o que roda como tarefa hard real-time do RTOS na CPU2, e o que roda como tarefa soft real-time na CPU2** — e como esses mundos trocam dados sem nunca recorrer a polling.

---

## 2. Decisão de arquitetura: dual-core — CPU1 bare-metal, CPU2 sob FreeRTOS

O F28379D é dual-core (dois C28x independentes, CPU1 e CPU2). Este projeto usa essa característica de propósito: **a malha de controle vive inteiramente na CPU1, sem RTOS, e tudo o que envolve gerenciamento de tarefas, EIS de alto nível e interface com o usuário vive na CPU2, sob FreeRTOS.**

A justificativa é a mesma de qualquer decisão de particionamento hard/soft, levada ao extremo: em vez de discutir se o jitter do escalonador "cabe" dentro do orçamento de 10 µs, a malha crítica é movida para um núcleo que **nunca enxerga o RTOS**. Isso elimina de raiz qualquer fonte de jitter vinda de troca de contexto, preempção por outra ISR de prioridade similar, ou latência de despertar de task — o problema desaparece por construção, não por otimização.

Essa divisão também favorece a expansão do projeto: a CPU1 pode evoluir seu algoritmo de controle (trocar o PI por um controlador mais sofisticado, adicionar uma segunda malha, etc.) sem tocar em uma linha do FreeRTOS, e a CPU2 pode crescer em funcionalidades de supervisório, conectividade e diagnóstico sem qualquer risco de interferir no timing da potência.

### O que fica em cada núcleo

- **CPU1 (bare-metal, sem RTOS).** Roda exclusivamente a **ISR do ePWM/ADC a 100 kHz**. Essa ISR: lê os canais do ADC (`x1`, `x2`, `x3`); calcula o PI completo com anti-windup; escreve o novo duty cycle direto em `CMPA`; gera a referência senoidal de excitação (`i_ref`) do sweep de EIS; e acumula as somas de correlação seno/cosseno amostra a amostra. Essas duas últimas tarefas — geração de referência e acumulação — ficam na CPU1 (e não migram para a CPU2) porque **precisam estar sincronizadas ciclo a ciclo com a própria malha de controle**: se a geração de `i_ref` fosse feita em outro núcleo, sob RTOS, o atraso de comunicação entre núcleos reintroduziria exatamente o tipo de jitter de fase que motiva manter o controle isolado em primeiro lugar. A CPU1, portanto, não é "burra" — ela é o único lugar do sistema onde o tempo é absolutamente determinístico.
- **CPU2 (FreeRTOS).** Recebe, ao final de cada step de frequência do sweep, as somas acumuladas pela CPU1 via IPC. A partir daí, todo o processamento pesado (raiz quadrada, arco-tangente), o EIS de alto nível, a UART, o shell, o LED e o botão rodam como tarefas do RTOS — exatamente como descrito nas seções seguintes.

### Comunicação entre núcleos (IPC)

A troca de dados entre CPU1 e CPU2 usa o mecanismo **nativo de IPC do F2837xD**: registradores de IPC (`IPCFLG`/`IPCACK`/`IPCSET`/`IPCCLR`) como sinalização de "mensagem pronta", e uma área de **RAM compartilhada (GSx RAM)** como payload — a CPU1 escreve a struct de somas acumuladas na RAM compartilhada e seta um flag de IPC; a CPU2 trata isso através de uma interrupção de IPC, que dá um semáforo para a task de cálculo do RTOS (nunca há polling do flag de IPC em nenhum dos dois lados). Isso evita qualquer necessidade de protocolo de software adicional — o IPC já resolve atomicidade e sincronismo entre os dois núcleos no nível de hardware.

Em resumo, a separação hard/soft do trabalho não acontece mais "dentro de um único núcleo" — ela acontece **entre núcleos**: tudo que precisa de determinismo em microssegundos fica isolado na CPU1; tudo que precisa de determinismo em milissegundos (ainda que computacionalmente caro) ou que é soft real-time vive na CPU2, sob FreeRTOS.

---

## 3. Arquitetura de tarefas

### Nível 0 — CPU1, bare-metal (acima e fora do RTOS)

| Bloco | Disparo | Responsabilidade |
|---|---|---|
| ISR ePWM/ADC (CPU1) | Timer de hardware, 100 kHz | Lê `x1`, `x2`, `x3`; calcula PI; escreve `CMPA`; gera `i_ref`; acumula somas sin/cos; ao fim de cada step de frequência, escreve a struct de somas na RAM compartilhada e sinaliza a CPU2 via IPC |

### Ponte entre núcleos — IPC (hardware)

| Mecanismo | Direção | Payload |
|---|---|---|
| IPC registers + GSx RAM compartilhada | CPU1 → CPU2 | Struct com as somas acumuladas de um step de frequência completo |
| IPC registers | CPU2 → CPU1 *(opcional, evolução futura)* | Ex.: novos K_p/K_i vindos da UART, se a CPU1 vier a expor esses parâmetros como ajustáveis em runtime |

### Nível 1 — CPU2, hard real-time (tarefas do RTOS, prioridade alta)

| Tarefa | Prioridade | Desbloqueio | Responsabilidade |
|---|---|---|---|
| `Task_Calculo_EIS` | Alta | Semáforo dado pela interrupção de IPC (dados novos disponíveis) | Calcula `sqrt` e `atan2` para extrair V_mag, I_mag e fase a partir das somas brutas recebidas da CPU1. Usa a **TMU** (Trigonometric Math Unit, hardware nativo do FPU do F28379D) via intrinsics do compilador para acelerar essas operações |

### Nível 2 — CPU2, soft real-time (tarefas do RTOS, prioridade média/baixa)

| Tarefa | Prioridade | Desbloqueio | Responsabilidade |
|---|---|---|---|
| `Task_Supervisorio_UART` | Média | `xQueueReceive` (Fila 2) + interrupção de RX da UART | Transmite telemetria (struct binária) ao PC; recebe comandos (novo K_p/K_i, start/stop, faixa de frequência) |
| `Task_Shell_Console` | Média-baixa | Interrupção de RX da UART (mesmo canal ou canal dedicado) | Shell de diagnóstico: `vTaskList`, `vTaskGetRunTimeStats`, heap livre, comando dedicado para inspecionar jitter/runtime das tarefas hard |
| `Task_Blink_LED` | Baixa | `vTaskDelay` | Heartbeat visual — pisca em intervalo fixo; qualquer engasgo perceptível indica estouro de tempo em tarefa de prioridade maior na CPU2 |
| `Task_Botao` | Baixa | Semáforo binário dado por interrupção GPIO | Inicia/encerra o sweep de EIS a qualquer momento, de forma segura |

Nenhuma tarefa de aplicação executa laços de espera ociosa (`while` checando flag) ou temporização por software. Todo bloqueio/desbloqueio depende de objetos de sincronismo do kernel — filas ou semáforos — inclusive na fronteira entre núcleos via IPC.

---

## 4. Mapeamento com os requisitos da disciplina

| Requisito do trabalho | Onde está atendido |
|---|---|
| Pelo menos uma tarefa hard real-time | `Task_Calculo_EIS` na CPU2 (e, em sentido mais amplo, a ISR de controle na CPU1, que é a parte de timing mais crítico do sistema, ainda que fora do RTOS) |
| Pelo menos uma tarefa soft real-time (mínimo 2 se usar a opção do enunciado) | `Task_Supervisorio_UART`, `Task_Shell_Console`, `Task_Blink_LED`, `Task_Botao` — quatro tarefas soft na CPU2, acima do mínimo exigido |
| Shell/console com info de tarefas, heap e runtime, de forma não intrusiva | `Task_Shell_Console`; a impressão nunca ocorre dentro de ISR ou tarefa hard — só consome dados já prontos via fila |
| LED de heartbeat para validar temporização | `Task_Blink_LED` |
| Tarefa disparada por botão físico | `Task_Botao`, via interrupção GPIO + semáforo binário |
| Nenhum polling em código de aplicação | Toda sincronização usa filas/semáforos do FreeRTOS (CPU2) e IPC de hardware (entre núcleos); nenhuma tarefa verifica flags em loop |

---

## 5. Estrutura de periféricos e toolchain

- **Processador:** TMS320F28379D (C28x, FPU64 + TMU), operando em **dual-core**: CPU1 dedicada ao controle (bare-metal), CPU2 dedicada ao FreeRTOS.
- **Aceleração matemática:** TMU nativa do FPU (presente em ambos os núcleos), acessada via intrinsics do compilador (ex.: `__sqrt`, suporte a `atan2f` acelerado) habilitada com `--fp_mode=relaxed` e a TMU ligada no link. Usada na CPU2, dentro de `Task_Calculo_EIS`. Não há uso do CLA nesta versão do projeto.
- **Comunicação entre núcleos:** IPC nativo do F2837xD (registradores `IPCFLG`/`IPCACK`/`IPCSET`/`IPCCLR` + RAM compartilhada GSx), inicializado nos dois projetos (CPU1 e CPU2) do CCS.
- **Evolução futura (fora do escopo atual):** mover parte do cálculo de controle para o **CLA (Control Law Accelerator)** dentro da própria CPU1, ou explorar comunicação CPU2→CPU1 para parâmetros ajustáveis em runtime. Mantido como próximo passo, não implementado nesta versão.
- **Ambiente de desenvolvimento:** Code Composer Studio (CCS), com **dois projetos separados** (um por núcleo) dentro do mesmo workspace, conforme o padrão dual-core do C2000Ware.
- **Bibliotecas base:** C2000Ware (drivers de periféricos via driverlib) + FPU/TMU intrinsics do compilador TI + driverlib de IPC.
- **Kernel:** FreeRTOS, rodando exclusivamente na CPU2, configurado no padrão moderno do compilador TI (EABI).

### Pinos e periféricos principais

| Periférico | Função no projeto |
|---|---|
| ePWM7 | Geração do PWM de potência (duty cycle = saída do PI) |
| ePWM1 | Trigger do ADC, sincronizado com a malha de controle |
| ADCINA0 / ADCIN2 | Leitura de corrente do indutor (`x1`) e tensão/corrente de carga (`x2`, `x3`) |
| GPIO (botão da placa) | Interrupção externa para `Task_Botao` |
| GPIO (LED da placa) | Saída controlada por `Task_Blink_LED` |
| SCI/UART | Canal de comunicação com o supervisório Python |

---

## 6. Protocolo de comunicação UART

A comunicação usa **pacotes binários com struct + framing**, em vez de struct binária crua. O motivo é robustez: uma struct enviada sem delimitador, se perder um único byte na linha (ruído, buffer overrun), desalinha o parser permanentemente — o receptor nunca mais encontra o início de um pacote válido. O framing abaixo resolve isso com um byte de início fixo e um checksum, permitindo que o lado Python re-sincronize sozinho após qualquer erro de transmissão.

### Formato do pacote (DSP → PC, telemetria)

```c
#define UART_START_BYTE 0xAA

typedef struct {
    uint8_t  start;       // sempre 0xAA
    uint8_t  msg_type;    // 0x01 = telemetria EIS
    float    freq;        // y2 - frequência ativa (Hz), recebida da CPU1 via IPC
    float    i_mag;       // y3 - magnitude de corrente
    float    v_mag;       // y4 - magnitude de tensão
    float    phase;       // y5 - fase (graus)
    float    duty;        // y1 - duty cycle atual
    uint8_t  sweep_done;  // y6 - flag de sweep finalizado
    uint8_t  checksum;    // XOR de todos os bytes anteriores
} __attribute__((packed)) telemetry_packet_t;
```

### Formato do pacote (PC → DSP, comandos)

```c
typedef struct {
    uint8_t  start;       // sempre 0xAA
    uint8_t  msg_type;    // 0x10 = novo Kp/Ki, 0x11 = start sweep,
                           // 0x12 = stop sweep, 0x13 = nova faixa de frequência
    float    param1;       // ex.: novo Kp, ou f_start
    float    param2;       // ex.: novo Ki, ou f_stop
    uint8_t  checksum;
} __attribute__((packed)) command_packet_t;
```

### Tipos de mensagem (`msg_type`)

| Código | Direção | Significado |
|---|---|---|
| `0x01` | DSP → PC | Telemetria de um step de frequência do EIS |
| `0x10` | PC → DSP | Atualiza K_p / K_i do controlador PI |
| `0x11` | PC → DSP | Inicia o sweep de EIS |
| `0x12` | PC → DSP | Interrompe o sweep de EIS |
| `0x13` | PC → DSP | Atualiza f_start / f_stop do sweep |

O lado Python (supervisório) é responsável por: enviar comandos via `pyserial`, validar o checksum de cada pacote recebido, descartar e re-sincronizar em caso de erro de framing, e plotar os dados de impedância (Bode e/ou Nyquist) em tempo real.

---

## 7. Como compilar e rodar

> *(Seção a completar conforme o setup final do CCS — caminhos de projeto, versão do C2000Ware e do FreeRTOS utilizada.)*

1. Importar **os dois projetos** no Code Composer Studio: o projeto bare-metal da CPU1 (controle) e o projeto FreeRTOS da CPU2 (EIS/UART/shell/LED/botão).
2. Verificar se a TMU está habilitada nas opções do linker/compilador (`--fp_mode=relaxed`) no projeto da CPU2.
3. Verificar a configuração de IPC e da RAM compartilhada (GSx) em ambos os projetos, garantindo que os endereços batem entre CPU1 e CPU2.
4. Compilar e gravar o F28379D — primeiro a CPU1, depois a CPU2 (a CPU1 precisa liberar/inicializar a CPU2 via boot mode ou `Device_bootCPU2()`, conforme o fluxo padrão dual-core do C2000Ware).
5. Conectar o supervisório Python na porta serial correspondente.
6. Usar `Task_Botao` ou um comando `0x11` via UART para iniciar o sweep de EIS.

---

## 8. Status do projeto

- [x] Decomposição do bloco C original (PSIM) em CPU1 (controle) + CPU2 (RTOS)
- [x] Definição do protocolo UART (struct + framing)
- [ ] Implementação da ISR de controle na CPU1
- [ ] Configuração do IPC e RAM compartilhada entre CPU1 e CPU2
- [ ] Implementação de `Task_Calculo_EIS` com TMU (CPU2)
- [ ] Implementação das tarefas soft real-time (CPU2)
- [ ] Supervisório Python (recepção, validação de checksum, plotagem)
- [ ] Validação experimental do EIS em bancada