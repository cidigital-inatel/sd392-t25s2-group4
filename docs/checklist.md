# Checklist de execução do projeto

Ordem de execução do início ao fim, seguindo as metas do Plano de Trabalho (`Plano_de_Trabalho_RISC-V_RV32IMAC_Extensoes.docx`). Marque `[x]` ao concluir. Dentro de cada janela de semanas, os itens estão na ordem em que precisam ser feitos para o seguinte funcionar. As frentes correm em paralelo; as janelas se sobrepõem como no cronograma do plano.

As semanas deste checklist são as do cronograma replanejado, não as 24 do plano. A execução começa em 05/10/2026, a entrega continua em 28/02/2027 e há um recesso de 21/12/2026 a 03/01/2027: são 19 semanas de trabalho, numeradas no calendário abaixo. A ordem das metas e as dependências entre elas são as do plano.

Última atualização: 2026-11-16. Semana atual do projeto: 7 (16/11 a 22/11/2026); o projeto começou em 2026-10-05.

Etiquetas: `[GES]` gestão · `[RTL]` hardware · `[VER]` verificação e modelo de referência · `[SW]` software, toolchain e IA · `[DOC]` documentação.

## Calendário

| Semana | Período | Metas com prazo final na semana |
|---|---|---|
| 1 | 05/10 a 11/10/2026 | |
| 2 | 12/10 a 18/10/2026 | |
| 3 | 19/10 a 25/10/2026 | M1.2, M1.3, M5.1 |
| 4 | 26/10 a 01/11/2026 | M2.1, M4.1 |
| 5 | 02/11 a 08/11/2026 | |
| 6 | 09/11 a 15/11/2026 | M3.1 |
| 7 | 16/11 a 22/11/2026 | |
| 8 | 23/11 a 29/11/2026 | M2.2, M4.2 |
| 9 | 30/11 a 06/12/2026 | M3.2 |
| 10 | 07/12 a 13/12/2026 | |
| 11 | 14/12 a 20/12/2026 | M2.3, M4.3 |
| Recesso | 21/12/2026 a 03/01/2027 | |
| 12 | 04/01 a 10/01/2027 | |
| 13 | 11/01 a 17/01/2027 | M2.4 |
| 14 | 18/01 a 24/01/2027 | M3.3 |
| 15 | 25/01 a 31/01/2027 | |
| 16 | 01/02 a 07/02/2027 | M2.5 |
| 17 | 08/02 a 14/02/2027 | M3.4, M4.4 |
| 18 | 15/02 a 21/02/2027 | M5.3 |
| 19 | 22/02 a 28/02/2027 | M5.4, M5.5 |

## Equipe de 3 pessoas (proposta de alocação, a formalizar na M1.3)

| Pessoa | Frente | Responde por |
|---|---|---|
| A (Lucas Lares) | `[RTL]` núcleo | M2.1 a M2.5; líder técnico e responsável pela especificação; apoio nas asserções SVA |
| B (Pedro Piccichelli) | `[VER]` verificação | M3.1 a M3.4: ambiente UVM, ISS em C, integração DPI-C e cobertura |
| C (Lucas Teixeira) | `[SW]` plataforma, software e IA; `[DOC]` | M4.1 a M4.4; memórias, periféricos, `soc_top` e testbench Verilog local (itens da M2.2); consolidação da documentação (M5.1 a M5.5) |
| Todos | `[GES]` | M1.1 a M1.5: atas em rodízio, RDA individual, riscos e mudanças |

Regras da divisão:

- Cada um documenta a própria frente: A a especificação, B o plano de verificação, C os guias de ambiente e de software. C consolida o conjunto.
- Revisão cruzada: B revisa o RTL de A; C valida o ISS de B contra o Spike, com os `riscv-tests` adaptados na M4.1; A confere os intrínsecos de C contra a especificação.
- Quem implementa uma instrução no RTL não é quem a implementa no ISS nem na referência em C puro.

Ordem de prioridade se for preciso cortar escopo (plano, seção 7.3): (1) núcleo RV32I com as instruções custom verificado; (2) extensões M e A; (3) aplicação de demonstração comparativa; (4) extensão C e otimizações.

## Entregas recorrentes (durante todo o projeto)

- [ ] `[GES]` M1.1 · Reunião semanal com ata, 19 atas ao todo (semanas 1 a 19). Definir dia, horário e modelo de ata na semana 1.
- [ ] `[GES]` M1.5 · Relatório de Atividades (RDA) individual, 10 ao todo: semanas 1, 3, 5, 7, 9, 11, 13, 15, 17 e 19.
- [ ] `[GES]` M1.4 · Revisão do registro de riscos e mudanças, 6 ao todo: semanas 3, 7, 11, 14, 17 e 19.
- [ ] `[DOC]` M5.2 · Revisão da documentação técnica, 3 ao todo: semanas 7, 11 e 14 (READMEs, guias, diagramas, especificação, plano de verificação, versões de ferramentas).
- [ ] `[GES]` Toda mudança de escopo registrada com descrição, motivação, impacto, alternativas e decisão (plano, seção 7.1). Mudanças na especificação exigem atualização simultânea de RTL, ISS, plano de verificação e biblioteca de software.

O plano prevê 24 atas e 13 RDAs, contados sobre 24 semanas. As quantidades acima são as que cabem em 19 semanas e precisam ser confirmadas com o orientador.

## Semana 1 a 2 (até 18/10/2026) — Base: ambiente, gestão e acesso ao Xcelium

Ambiente de cada membro (repetir em cada máquina seguindo `docs/ambiente.md`):

- [x] `[SW]` WSL 2 com Ubuntu 24.04, pacotes básicos (build-essential, git, python3, curl, xz).
- [x] `[SW]` Toolchain xPack GNU RISC-V Embedded GCC 15.2.0-1 em `~/opt`, no PATH, verificado com `-march=rv32imac_zicsr -mabi=ilp32`.
- [x] `[DOC]` `docs/ambiente.md` criado e sendo atualizado a cada instalação.
- [ ] `[SW]` Membros B e C instalam o mesmo ambiente nas suas máquinas.
- [ ] `[VER]` Confirmar o acesso à máquina remota do Xcelium: host, usuário, licença, versão do Xcelium e da UVM. Rodar um exemplo mínimo com `xrun` para provar que compila e simula SystemVerilog com UVM. Registrar em `docs/ambiente.md` (seção 8). O plano exige isso nas semanas 1 e 2.
- [ ] `[VER]` Definir como o projeto chega à máquina remota (repositório, `scp` ou `rsync`) e testar o caminho de ida e volta.
- [x] `[SW]` Instalar dependências de compilação do Spike no WSL (`sudo apt install autoconf automake libtool device-tree-compiler libboost-dev libboost-regex-dev libboost-system-dev`), clonar `riscv-isa-sim`, compilar e instalar em `~/opt/spike`; fixar o commit e registrar em `docs/ambiente.md`.
- [x] `[RTL]` Instalar Verilator e Icarus Verilog no WSL (`sudo apt install verilator iverilog gtkwave`) para simulação rápida local sem UVM; registrar versões.

Gestão e documentação inicial:

- [ ] `[GES]` Preencher o plano de trabalho: nomes dos três membros, orientador, colaboradores, parágrafo "situação preliminar já alcançada", link do repositório.
- [ ] `[GES]` M1.3 · Matriz de responsabilidades e dependências (a tabela acima, ajustada), aprovada pelos três.
- [x] `[GES]` Criar o repositório no GitHub, com as pastas `rtl/`, `tb/`, `model/`, `sw/`, `docs/`: `https://github.com/lucaslaress/riscv-rv32imac-ci-digital` (privado).
- [x] `[GES]` Convidar os membros B e C como colaboradores do repositório.
- [ ] `[GES]` Combinar entre os membros a convenção de commits, tags e issues (proposta em `docs/convencoes.md`).
- [x] `[DOC]` Criar `docs/decisoes.md` com três tabelas: decisões tomadas, riscos (copiar a tabela 7.4 do plano e acompanhar) e mudanças de escopo.
- [x] `[DOC]` M5.1 · README na raiz e um README curto em cada pasta (`rtl/`, `tb/`, `model/`, `sw/`) dizendo o que vai ali.

## Semana 1 a 4 (até 01/11/2026) — M4.1 Toolchain e fluxo bare-metal `[SW]`

- [ ] `sw/common/link.ld` com o mapa de memória da especificação, seção 2 (IMEM em `0x0000_0000` com 64 KiB, DMEM em `0x1000_0000` com 128 KiB) e os símbolos `__stack_top`, `__bss_start`, `__bss_end`, `__global_pointer$`.
- [ ] `sw/common/crt0.S`: código em `.text.init` no endereço 0 que carrega `gp` e `sp`, zera `.bss`, chama `main` e, no retorno, sinaliza término.
- [ ] `sw/common/hal.h` e `hal.c`: escrita de caractere no console (`0x2000_0000`), término pelo registrador `tohost` (`0x2000_0004`), leitura de `mcycle` e `minstret` via `csrr`. Protocolo de término proposto: escrever `(código << 1) | 1`, com `1` significando sucesso (mesma convenção do Spike e dos riscv-tests); confirmar na M2.1.
- [ ] `sw/hello/main.c`: imprime uma mensagem, lê os contadores e termina.
- [ ] `sw/common/common.mk` e `sw/hello/Makefile`: `CROSS=riscv-none-elf-`, `-march=rv32imac_zicsr -mabi=ilp32`, `-nostdlib -nostartfiles -T link.ld`, geração de `.elf`, `.map` e `.dis` (objdump).
- [ ] Conversão ELF → hex por região de memória, uma palavra de 32 bits por linha, para `$readmemh`: um arquivo para a IMEM e outro para a DMEM (`objcopy -O verilog` ou script Python próprio; conferir a ordem dos bytes).
- [ ] Definir como rodar os programas no Spike antes de existir RTL: o Spike reserva `0x1000` para o boot ROM, então ou o programa é religado em `0x8000_0000` para o Spike, ou o ISS próprio (M3.2) assume esse papel. Registrar a decisão.
- [ ] Adaptar os `riscv-tests` (`rv32ui`, `rv32um`, `rv32ua`, `rv32uc`): copiar a suíte fixando o commit, substituir o ambiente `env/p` por linker script e `riscv_test.h` próprios, com `RVTEST_PASS`/`RVTEST_FAIL` escrevendo no `tohost`; Makefile que gera `.elf` e `.hex` de cada teste.
- [ ] Registrar em `docs/ambiente.md` todas as versões (toolchain, Spike, Verilator, Icarus, Python) e os comandos do fluxo.
- [ ] Aceite M4.1 (plano): programa em C compilado, carregado e executado até o fim em simulação, com ambiente e versões registrados. A execução em RTL depende da M2.2; até lá, aceitar a execução no Spike ou no ISS.

## Semana 1 a 4 (até 01/11/2026) — M2.1 Especificação da arquitetura e da ISA estendida `[RTL]`, com todos

Este documento é o contrato entre RTL, ISS, testbench e software. Nada de RTL, ISS ou intrínseco definitivo antes dele.

- [x] Criar `docs/especificacao.md` com as seções: escopo, mapa de memória e periféricos, reset e término, instruções custom, pipeline, extensão M, extensão A, extensão C, CSRs e traps, interface de memória, interface de trace, parâmetros do topo, histórico de versões.
- [x] Mapa de memória: confirmar bases e tamanhos. Atenção: 100 imagens MNIST de 784 bytes ocupam cerca de 78 KB, mais do que os 64 KiB propostos para a DMEM. Decidir entre DMEM maior ou carga de uma imagem por vez pelo testbench. Decidido: DMEM de 128 KiB (D2 em `docs/decisoes.md`).
- [x] Periféricos: endereços e semântica do console, do `tohost` e dos espelhos dos contadores; protocolo de término. Definidos na especificação, seção 2; espelhos dos contadores retirados, o software lê os contadores por `csrr`.
- [x] Instruções custom: tabela de codificação (opcode `0001011`, funct3 `000`/`001`/`010`, funct7 `0000000`), semântica exata de `mac`, `sat` e `relu`, comportamento com `rd = x0`, com `funct3` e `funct7` reservados, e `sat` com `n = 0`. Definidos na especificação, seção 4 (D3 e D7); reservados geram instrução ilegal, a fechar com a seção 9.
- [x] Decisão sobre a leitura de `rd` na MAC: terceira porta de leitura no banco, acumulador dedicado, execução em dois ciclos ou acumulador em `rs1`. Registrar a escolha e o efeito no forwarding. Decidido: acumulador dedicado na unidade custom, exposto como CSR `0x7C0`; a `mac` copia o novo acumulado em `rd` (D3 em `docs/decisoes.md`; especificação, seção 4.2).
- [x] Decisão sobre a largura de saturação: registrador (`rs2[4:0]`, como proposto) ou imediato. Decidido: registrador, `n = rs2[4:0]`, com `n = 0` como identidade (D7; especificação, seção 4.3).
- [ ] Pipeline: estágios, registradores entre estágios, forwarding EX/MEM e MEM/WB, stall de load-use, resolução de desvio em EX com predição não tomado, flush. Diagrama de blocos do pipeline (exigido no aceite).
- [ ] Extensão M: multiplicação em 1 ou 2 ciclos, divisão multiciclo com stall, compartilhamento ou não do multiplicador com a MAC.
- [ ] Extensão A: reserva de `lr/sc` e sequência read-modify-write no MEM.
- [ ] Extensão C: alinhamento de 16 bits, busca que cruza palavra, expansão em ID, habilitação por parâmetro.
- [ ] CSRs: `mcycle`/`mcycleh`, `minstret`/`minstreth`; decidir se `mstatus`, `mtvec`, `mepc`, `mcause` entram e como `ecall`, `ebreak` e instrução ilegal são tratados.
- [x] Interface de memória: nome, direção e semântica de cada sinal das portas de instrução e de dados, com handshake que permita latência variável. Definida na especificação, seção 10: portas `ibus_` e `dbus_`, protocolo `valid`/`ready` de uma fase com resposta no mesmo ciclo (D5).
- [x] Interface de trace de retire (estilo RVFI): lista de sinais que o testbench e o ISS comparam (PC, instrução, `rd` e valor, endereço e dado de memória, trap). Definida na especificação, seção 11: subconjunto RVFI com fontes, destino, memória e porta genérica de escrita de CSR (D6).
- [ ] Parâmetros do módulo topo: vetor de reset, tamanhos de memória, habilitação individual de M, A, C e custom.
- [ ] Revisão pelos três membros e aprovação pelo orientador; versão 1.0 registrada.
- [ ] Aceite M2.1 (plano): especificação versionada, diagrama de blocos do pipeline e tabela de codificação aprovados.

## Semana 2 a 6 (até 15/11/2026) — M3.1 Ambiente UVM base e smoke test `[VER]`

Depende de: interface de memória e de trace definidas na M2.1; um núcleo mínimo da M2.2 (ver abaixo) ou, até lá, um DUT de fachada que só responda ao protocolo de memória.

- [ ] Estrutura de `tb/`: `uvm/`, `sva/`, `cov/`, `tests/`, `Makefile`.
- [ ] Interfaces SystemVerilog das portas de memória e do trace de retire, conforme a especificação.
- [ ] Testbench top: instancia o DUT (`soc_top` ou o núcleo com memórias), gera clock e reset, conecta as interfaces e chama `run_test`.
- [ ] Agente de memória: `sequence_item`, `driver`, `monitor`, `sequencer`; modelo de memória esparsa carregado a partir dos `.hex`; latência configurável (wait states) para exercitar stalls.
- [ ] Monitor de trace de retire: captura PC, instrução, `rd` e valor, endereço e dado a cada instrução retirada e publica em uma `analysis_port`.
- [ ] Scoreboard inicial: recebe os retires; comparação com o ISS entra na M3.2.
- [ ] `uvm_env` e `uvm_test` base: configuração por `plusargs` (`+PROGRAM=`, `+UVM_TESTNAME=`, `-svseed`, latência de memória).
- [ ] Console e `tohost` observados pelo testbench: imprimir caracteres e encerrar a simulação com o código de término.
- [ ] `tb/Makefile` para o Xcelium: alvos `compile`, `run`, `smoke`, `clean`, com dump de ondas desligável.
- [ ] Smoke test: carrega `hello.hex`, executa até o `tohost` e passa.
- [ ] Guia reprodutível em `tb/README.md`: como sincronizar com a máquina remota, compilar e rodar um teste.
- [ ] Aceite M3.1 (plano): guia reprodutível, scripts de compilação e execução, smoke test aprovado.

## Semana 3 a 8 (até 29/11/2026) — M2.2 Núcleo RV32I em pipeline `[RTL]`

Ordem interna pensada para ter um núcleo mínimo executando cedo (para a M3.1) e crescer sem retrabalho. Os itens marcados com (C) são da pessoa C, que cuida da plataforma em volta do núcleo; os demais são da pessoa A.

- [ ] Estrutura de `rtl/`: um módulo por arquivo, nomes conforme a especificação; `soc_top` (núcleo + memórias + periféricos), `core_top` (só o núcleo).
- [ ] (C) `imem` e `dmem` em Verilog com `$readmemh`, tamanhos parametrizados; `periph` com console e `tohost` (sem espelhos dos contadores, especificação seção 2); `soc_top` ligando o núcleo às memórias e aos periféricos.
- [ ] Banco de registradores (32 × 32 bits, `x0` fixo em zero, duas portas de leitura e uma de escrita; a `mac` não lê `rd`, D3).
- [ ] Gerador de imediatos, decodificador RV32I, ALU.
- [ ] Estágio IF: PC, busca, cálculo de PC + 4 (PC + 2 entra na M2.3).
- [ ] Estágio ID: decodificação, leitura do banco, imediatos.
- [ ] Estágio EX: ALU, comparação e alvo de desvios e saltos.
- [ ] Estágio MEM: `lb/lh/lw/lbu/lhu/sb/sh/sw` com byte enables e extensão de sinal.
- [ ] Estágio WB: escrita no banco.
- [ ] Núcleo mínimo funcionando sem hazards (programa com `nop` entre instruções dependentes) em simulação local com Verilator ou Icarus; entregar à frente de verificação para a M3.1.
- [ ] Registradores de pipeline e unidade de hazards: forwarding EX/MEM e MEM/WB para EX, stall de load-use, flush em desvio tomado.
- [ ] CSRs `mcycle`, `minstret` (e os demais decididos na M2.1) e o tratamento de `ecall`/`ebreak`/ilegal decidido.
- [ ] Interface de trace de retire (RVFI) no estágio WB, conforme a especificação.
- [ ] (C) Testbench Verilog simples (sem UVM) para simulação local: carrega os `.hex`, roda até o `tohost`, imprime o console.
- [ ] `hello` executa até o fim no RTL.
- [ ] Todos os `rv32ui` aprovados no RTL.
- [ ] Aceite M2.2 (plano): testes `rv32ui` aprovados em simulação e programa em C executado até o fim.

## Semana 4 a 9 (até 06/12/2026) — M3.2 Modelo de referência (ISS) integrado via DPI-C `[VER]`

- [ ] Estrutura de `model/`: código C do ISS, executável standalone, biblioteca para DPI-C, Makefile.
- [ ] Memória do ISS com mapa configurável (para rodar tanto no mapa do projeto quanto no do Spike), carga de `.hex` e de ELF.
- [ ] Decodificação e execução de RV32I; CSRs `mcycle`/`minstret`; periféricos (console, `tohost`).
- [ ] Executável standalone com trace em texto: uma linha por instrução retirada com os mesmos campos da interface de trace.
- [ ] Extensão M, extensão A (com reserva de `lr/sc`), extensão C (expansão das comprimidas, PC + 2).
- [ ] Instruções `mac`, `sat` e `relu` exatamente como na especificação.
- [ ] Validação cruzada: rodar os `riscv-tests` (`rv32ui/um/ua/uc`) no Spike e no ISS e comparar os traces; resultado idêntico.
- [ ] Validação das instruções custom contra implementação independente em C puro e, depois da M4.3, contra os vetores de referência da CNN.
- [ ] Interface DPI-C: `iss_init`, `iss_load`, `iss_reset`, `iss_step` devolvendo a tupla de retire esperada.
- [ ] Integração ao scoreboard: comparação em lock-step de PC, instrução, `rd` e valor, endereço e dado de memória; parada na primeira divergência com contexto completo (últimas N instruções, estado dos registradores).
- [ ] Teste de detecção: injetar de propósito um erro no RTL ou no ISS e comprovar que o scoreboard acusa.
- [ ] Aceite M3.2 (plano): resultados idênticos ao Spike nos `riscv-tests`; scoreboard detecta divergências injetadas.

## Semana 5 a 8 (até 29/11/2026) — M4.2 Biblioteca de intrínsecos `[SW]`

- [ ] `sw/include/ci_ext.h`: `mac`, `sat` e `relu` via `asm volatile` com `.insn r`, restrições `"=r"` em `rd` e `"r"` nos fontes, mais funções para zerar, ler e carregar o acumulador com `csrwi`, `csrr` e `csrw` no CSR `0x7C0`, conforme a especificação, seção 4.2.
- [ ] `sw/include/ci_ref.h`: as mesmas três funções em C puro, sem UB, usadas como referência e na versão `baseline`.
- [ ] Script que roda `objdump -d`, localiza as palavras com opcode `0x0B` e decodifica os campos, para conferir a codificação de cada ocorrência.
- [ ] `sw/tests/custom/`: teste unitário que compara intrínseco e referência em valores de borda (máximos, mínimos, zero, negativos, overflow da MAC, limites da SAT com n = 8 e 16) e em valores aleatórios determinísticos.
- [ ] Teste executado no ISS (assim que a M3.2 tiver as instruções custom) e no RTL (após a M2.4).
- [ ] Aceite M4.2 (plano): biblioteca versionada, código gerado inspecionado com objdump, testes unitários aprovados.

## Semana 5 a 11 (até 20/12/2026) — M4.3 Pacote do modelo de IA quantizado `[SW]`

- [ ] Ambiente Python no WSL: `python3 -m venv`, PyTorch, torchvision, NumPy, Jupyter; versões registradas em `docs/ambiente.md`.
- [ ] Notebook: download do MNIST, CNN compacta (na ordem de 2 convoluções com pooling e 1 camada totalmente conectada, poucos milhares de parâmetros), treino em ponto flutuante, acurácia no conjunto de teste (meta acima de 95%).
- [ ] Quantização pós-treino para int8 com escalas em potência de dois, de modo que a requantização seja um deslocamento aritmético; acurácia int8 registrada.
- [ ] Referência bit-exata em NumPy da inferência int8: mesma ordem de operações que o C vai usar (acumulação em int32, deslocamento, saturação, ReLU).
- [ ] Geração dos vetores de referência para um subconjunto de imagens de teste (por exemplo 100): entrada, saída de cada camada e saída final.
- [ ] Exportação de pesos, vieses, escalas e vetores como arrays C (`.h`) e como arquivos de dados versionados.
- [ ] Verificar que a saída final do NumPy bate com a classe prevista pelo PyTorch quantizado nas mesmas imagens.
- [ ] Aceite M4.3 (plano): notebook reprodutível, acurácias float e int8 registradas, vetores de referência versionados.

## Semana 7 a 11 (até 20/12/2026) — M2.3 Extensões M, A e C `[RTL]`

A janela termina na última semana antes do recesso. Na revisão de riscos da semana 11, se a extensão C ainda não estiver aprovada no `rv32uc`, decidir entre concluí-la depois do recesso ou registrar a redução de escopo (risco R2 em `docs/decisoes.md`).

- [ ] Extensão M: multiplicador (`mul`, `mulh`, `mulhsu`, `mulhu`) em 1 ou 2 ciclos conforme a M2.1; divisor multiciclo (`div`, `divu`, `rem`, `remu`) com stall e casos de divisão por zero e overflow conforme a especificação RISC-V; `rv32um` aprovado.
- [ ] Extensão A: `lr.w` e `sc.w` com reserva; `amoswap/add/xor/and/or/min/max/minu/maxu.w` como read-modify-write no MEM com stall; `rv32ua` aprovado.
- [ ] Extensão C, por último: busca alinhada a 16 bits, remontagem de instruções de 32 bits que cruzam a fronteira de palavra, expansor de comprimidas em ID, PC + 2, habilitação por parâmetro; `rv32uc` aprovado.
- [ ] Programa em C compilado com `-march=rv32imac_zicsr` (com instruções comprimidas) executado corretamente.
- [ ] Aceite M2.3 (plano): `rv32um`, `rv32ua` e `rv32uc` aprovados; código compilado com `-march=rv32imac` executado corretamente.

## Semana 8 a 14 (até 24/01/2027) — M3.3 Suíte de testes, asserções e plano de cobertura `[VER]`

- [ ] `docs/plano-verificacao.md`: cada requisito da especificação ligado a testes, asserções e coverpoints; metas de cobertura funcional e de código.
- [ ] Sequência de carga de programa (`.hex`) e de testes dirigidos (riscv-tests, testes custom, hello).
- [ ] Gerador de instruções aleatórias restritas: só programas válidos (alvos de desvio dentro do programa, endereços dentro da memória, término controlado pelo `tohost`), com pesos para forçar hazards RAW a distância 1, 2 e 3, load-use, desvios tomados e não tomados, `lr/sc`, comprimidas e valores de borda das instruções custom.
- [ ] Asserções SVA: `x0` nunca escrito; PC alinhado a 2 bytes; cada instrução retirada exatamente uma vez; nenhuma escrita durante flush; protocolo da interface de memória.
- [ ] Covergroups: por instrução e por extensão; hazards; desvios; sucesso e falha de `lr/sc`; comprimidas e desalinhadas; MAC com overflow; SAT dentro e fora dos limites com n = 8 e 16; RELU negativo, zero e positivo.
- [ ] `tb/Makefile`: alvos `regress SEEDS=...`, `cov` (relatórios de cobertura funcional e de código do Xcelium), `riscv-tests`.
- [ ] Todos os testes dirigidos aprovados em regressão.
- [ ] Aceite M3.3 (plano): plano de cobertura versionado, suíte executável em regressão, testes dirigidos aprovados.

## Semana 10 a 13 (até 17/01/2027) — M2.4 Unidade de extensões customizadas `[RTL]`

- [ ] Decodificação do opcode `custom-0` e dos campos `funct3`/`funct7`; tratamento dos valores reservados conforme a M2.1.
- [ ] Registrador `ACC` na unidade custom, exposto como CSR `0x7C0` (D3): reset, escrita condicionada ao descarte por flush ou trap, acesso por `csrr`, `csrw` e `csrwi` integrado à unidade de CSRs.
- [ ] `custom_unit` no estágio EX: MAC (multiplicador próprio ou compartilhado), SAT, RELU; resultado pelo caminho normal de WB.
- [ ] Testes dirigidos das três instruções (os de M4.2 e programas em assembly) aprovados contra o ISS, incluindo casos de borda e sequências envolvendo o `ACC`: `csrw` seguida de `mac`, `mac` seguida de `csrr`, `mac` com `rd = x0`.
- [ ] Covergroups das instruções custom atingindo as bordas previstas.
- [ ] Aceite M2.4 (plano): testes dirigidos das três instruções aprovados contra o modelo de referência, incluindo casos de borda.

## Semana 11 a 17 (até 14/02/2027) — M4.4 Aplicação de demonstração e relatório comparativo `[SW]`

- [ ] Micro-benchmarks em `sw/bench/`: produto interno int8, convolução 3×3, requantização (deslocamento + saturação + ReLU), cada um em `baseline` (C puro) e `ext` (intrínsecos); medição de `mcycle`/`minstret` no ISS.
- [ ] `sw/mnist/baseline/`: inferência completa em C só com RV32IMAC (convoluções, pooling, camada totalmente conectada, requantização, argmax), usando os pesos exportados na M4.3; comparação camada a camada com os vetores de referência no ISS.
- [ ] `sw/mnist/ext/`: a mesma inferência com os intrínsecos; mesma comparação camada a camada.
- [ ] Verificar que a DMEM comporta pesos, ativações e imagens conforme a decisão da M2.1; ajustar a carga das imagens se necessário.
- [ ] Execução das duas versões no RTL sobre o subconjunto de imagens (dimensionar pelo tempo de simulação; dump de ondas desligado), registrando por imagem: classe prevista, igualdade bit a bit com o NumPy, `mcycle` e `minstret`.
- [ ] Análise estática do código gerado: número de instruções no laço interno de cada núcleo nas duas versões (objdump).
- [ ] Relatório comparativo: ciclos, instruções, acurácia, razão `baseline/ext`, tabela por imagem, e a validação nos quatro níveis (PyTorch, NumPy, ISS, RTL).
- [ ] Aceite M4.4 (plano): saídas bit a bit iguais à referência nas duas versões e relatório comparativo de desempenho.

## Semana 13 a 16 (até 07/02/2027) — M2.5 RTL consolidado `[RTL]`

- [ ] Corrigir todos os defeitos apontados pela verificação, registrando cada um (descrição, causa, correção, teste que o detectou).
- [ ] Lint do RTL (Verilator `--lint-only` e o lint do Xcelium) sem avisos relevantes; relatório salvo em `docs/`.
- [ ] Opcional (plano): síntese lógica genérica (por exemplo Yosys) para estimativa de recursos.
- [ ] Regressão completa sem falhas.
- [ ] Versão congelada e etiquetada no repositório.
- [ ] Aceite M2.5 (plano): regressão completa sem falhas, relatório de lint, versão etiquetada.

## Semana 13 a 17 (até 14/02/2027) — M3.4 Regressão e cobertura finais `[VER]`

- [ ] Regressões com múltiplas seeds (Makefile), incluindo latências de memória variadas.
- [ ] Análise de cobertura funcional e de código; novos testes para fechar lacunas; exceções justificadas por escrito no plano de verificação.
- [ ] Registro dos defeitos encontrados e corrigidos ao longo do projeto.
- [ ] Relatório final de regressão e cobertura.
- [ ] Aceite M3.4 (plano): relatório de cobertura com metas atingidas ou exceções justificadas; regressão final sem falhas.

## Semana 15 a 18 (até 21/02/2027) — M5.3 Dossiê de evidências `[DOC]`

- [ ] Logs de simulação das regressões finais e dos riscv-tests.
- [ ] Relatórios de cobertura e de lint.
- [ ] Medições de desempenho e comparação com o modelo de referência (M4.4).
- [ ] Procedimentos de reprodução passo a passo (ambiente, compilação, simulação, geração dos relatórios).
- [ ] Limitações conhecidas e itens cortados do escopo, com referência ao registro de mudanças.
- [ ] Aceite M5.3: dossiê consolidado em `docs/evidencias/`.

## Semana 17 a 19 (até 28/02/2027) — M5.4 Revisão final `[DOC]`, com todos

- [ ] Versão final do documento do TCC, coerente com objetivos, metodologia, resultados e cronograma.
- [ ] Planilha de metas atualizada (planejado × realizado).
- [ ] Repositório, READMEs, referências e links conferidos.
- [ ] Terminologia unificada entre especificação, código, testbench e texto.

## Semana 18 a 19 (até 28/02/2027) — M5.5 Apresentação e pacote final `[DOC]`, com todos

- [ ] Apresentação final.
- [ ] Roteiro da demonstração e gravação em vídeo.
- [ ] Repositório etiquetado com a versão final.
- [ ] Pacote de entrega: documento, repositório, roteiro, gravação.
- [ ] Último RDA, última ata e última revisão de riscos (semana 19).
