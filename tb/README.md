# tb

Ambiente de verificação funcional em SystemVerilog com UVM, executado no Cadence Xcelium na máquina remota (ver `docs/ambiente.md`, seção 8).

Começa a ser preenchida na meta M3.1, com o testbench base e um smoke test que carrega um programa e roda até o término.

Componentes previstos no plano:

- testbench top, interfaces de memória e de trace, `uvm_test` e `uvm_env`;
- agente de memória com modelo de memória esparsa e latência configurável;
- monitor de trace de retire;
- scoreboard em lock-step com o modelo de referência de `model/`, via DPI-C;
- sequências dirigidas e gerador de instruções aleatórias restritas, que só produzem programas válidos;
- asserções SVA de invariantes do pipeline e do protocolo de memória;
- covergroups por instrução, extensão, hazard, desvio e valores de borda das instruções custom;
- Makefile de regressão com múltiplas seeds e relatórios de cobertura.

Regra da frente: a primeira divergência entre RTL e modelo de referência interrompe o teste com o contexto completo.
