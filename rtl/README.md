# rtl

Núcleo RISC-V RV32IMAC em pipeline de cinco estágios, em Verilog (IEEE 1364-2005), com a unidade das instruções customizadas `mac`, `sat` e `relu`, mais as memórias de instruções e de dados e os periféricos simulados (console e registrador de término; os contadores são CSRs lidos com `csrr`, conforme `docs/especificacao.md`, seção 2).

Começa a ser preenchida na meta M2.2, depois que a especificação (`docs/especificacao.md`, meta M2.1) estiver aprovada. Nada de RTL antes disso.

Regras da frente:

- Um módulo por arquivo: um por estágio (IF, ID, EX, MEM, WB) e um por unidade funcional (ALU, multiplicador e divisor, unidade custom, decodificador de instruções comprimidas, banco de registradores, CSRs, unidade de hazards e forwarding).
- Parâmetros no módulo topo: vetor de reset, tamanhos de memória e habilitação individual das extensões M, A, C e custom.
- Verilog puro; SystemVerilog só em `tb/`.
- Interface de trace de retire (estilo RVFI) no estágio WB, conforme a especificação: é o que o testbench compara com o modelo de referência.
