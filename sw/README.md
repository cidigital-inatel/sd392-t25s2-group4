# sw

Tudo o que roda no processador ou gera dados para ele.

Começa a ser preenchida na meta M4.1, com o fluxo bare-metal: linker script com o mapa de memória, código de inicialização (`crt0`), programa mínimo e conversão de ELF para os arquivos hex que carregam as memórias simuladas.

Conteúdo previsto no plano, por meta:

- M4.1: fluxo bare-metal, Makefiles e a suíte `riscv-tests` adaptada ao mapa de memória e ao mecanismo de término;
- M4.2: biblioteca de intrínsecos em C que expõe `mac`, `sat` e `relu` via `asm` inline e diretiva `.insn`, com testes unitários contra implementações em C puro;
- M4.3: pacote do modelo de IA: notebook de treino e quantização da CNN para o MNIST, pesos int8, referência bit-exata em NumPy e vetores de referência por camada;
- M4.4: micro-benchmarks e a aplicação MNIST em duas versões, `baseline` (só RV32IMAC) e `ext` (com os intrínsecos), com medição de ciclos e instruções.

Compilação: toolchain xPack `riscv-none-elf-gcc` com `-march=rv32imac_zicsr -mabi=ilp32` (instalação em `docs/ambiente.md`, seção 4). A codificação gerada para as instruções custom é sempre conferida com `objdump -d`.

Subpastas: a definir na M4.1.
