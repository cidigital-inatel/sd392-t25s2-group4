# model

Modelo de referência do processador: um simulador do conjunto de instruções (ISS) em C, com precisão de instrução, cobrindo RV32IMAC, os CSRs `mcycle` e `minstret` e as instruções customizadas `mac`, `sat` e `relu`, conforme `docs/especificacao.md`.

Começa a ser preenchida na meta M3.2, depois da especificação (M2.1).

Dois usos:

- biblioteca chamada pelo scoreboard UVM via DPI-C, executando uma instrução por chamada e devolvendo o estado esperado (PC, instrução, registrador destino e valor, acesso à memória);
- executável autônomo que roda um programa e gera um trace em texto, para depurar programas antes de existir RTL.

Regra da frente: o modelo também pode estar errado. Ele é validado contra o Spike e a suíte `riscv-tests` no subconjunto padrão, e contra implementações independentes em C e os vetores de referência da CNN nas instruções custom.
