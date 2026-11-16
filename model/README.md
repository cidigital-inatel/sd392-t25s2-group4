# model

Modelo de referência do processador: um simulador do conjunto de instruções (ISS) em C, com precisão de instrução, cobrindo RV32IMAC, os CSRs `mcycle` e `minstret` e as instruções customizadas `mac`, `sat` e `relu`, conforme `docs/especificacao.md`.

Dois usos:

- biblioteca chamada pelo scoreboard UVM via DPI-C, executando uma instrução por chamada e devolvendo o estado esperado (PC, instrução, registrador destino e valor, acesso à memória);
- executável autônomo que roda um programa e gera um trace em texto, para depurar programas antes de existir RTL.

Regra da frente: o modelo também pode estar errado. Ele é validado contra o Spike e a suíte `riscv-tests` no subconjunto padrão, e contra implementações independentes em C e os vetores de referência da CNN nas instruções custom.

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `iss.h` | API: `iss_init`, `iss_load_elf`, `iss_load_hex`, `iss_reset`, `iss_step` e a tupla de retire `iss_retire_t` |
| `iss.c` | Memória (IMEM, DMEM, periféricos), expansão das comprimidas, execução e formatação do trace |
| `iss_main.c` | Executável autônomo |
| `tests/test_iss.c` | Autoteste com mini-montador em C: roda sem o toolchain RISC-V |
| `Makefile` | Alvos `all`, `test`, `lib` (biblioteca para DPI-C) e `clean` |

## Como compilar e rodar

Precisa só de um compilador C (gcc no WSL/Linux; no Windows também funciona com MinGW e `mingw32-make`).

```sh
cd model
make test                                   # compila e roda o autoteste
./build/iss programa.elf                    # executa um ELF até o tohost
./build/iss --imem imem.hex --dmem dmem.hex # ou os .hex do $readmemh
./build/iss programa.elf --trace trace.log --stats
```

Opções: `--trace ARQ` (`-` para stderr), `--max-insns N`, `--reset-pc END`, `--stats`. O console sai na saída padrão. Código de saída: o do `tohost` (0 é sucesso), 2 em trap, 3 se estourar o limite de instruções.

O trace usa o formato do `--log-commits` do Spike, para comparar os dois com `diff`:

```
core   0: 3 0x0000000c (0x00b50533) x10 0x00000001
core   0: 3 0x00000104 (0xc188) mem 0x10000000 0x0000002a
```

Instruções comprimidas aparecem com os 16 bits originais, como no Spike.

## Exemplo em C

`exemplos/hello/` compila um programa em C com o toolchain xPack e roda no ISS: imprime pelo console, lê `minstret` e compara um produto escalar int8 em C puro com o mesmo cálculo pela instrução `mac`. O `crt0.S` e o `link.ld` dali são provisórios, só para o exemplo; os oficiais são da M4.1, em `sw/common/`.

```sh
cd model && make                 # compila o ISS
cd exemplos/hello && make run    # compila o hello.elf e roda no ISS
```

Saída esperada:

```
Ola do RISC-V! Programa em C rodando no ISS do grupo 4.
produto escalar int8, 64 elementos
  C puro : resultado 70816, 452 instrucoes
  com mac: resultado 70816, 386 instrucoes
resultados iguais
iss: PASSOU (tohost = 1)
```

## Estado (M3.2)

Feito:

- [x] RV32I, extensões M, A (com reserva de `lr/sc`) e C (expansão para a instrução de 32 bits, PC + 2).
- [x] `mac` com acumulador no CSR `macacc` (`0x7C0`), `sat` e `relu`.
- [x] CSRs `mcycle`/`mcycleh`, `minstret`/`minstreth` e os apelidos de leitura `cycle`/`instret`.
- [x] Mapa de memória Harvard da especificação, seção 2; `console` e `tohost`.
- [x] Carga de ELF32 e de `.hex` no formato do `$readmemh`.
- [x] Executável autônomo com trace no formato do Spike.
- [x] Autoteste: 12 grupos, 71 verificações.

Falta:

- [ ] Mapa de memória configurável, para rodar programas ligados no mapa do Spike (`0x8000_0000`).
- [ ] Validação cruzada com o Spike nos `riscv-tests` (`rv32ui/um/ua/uc`), depois da M4.1.
- [ ] Camada DPI-C (`import "DPI-C"`) e integração ao scoreboard.
- [ ] Ajustar os pontos provisórios abaixo quando a especificação os fechar.

## Escolhas provisórias (a confirmar na especificação)

Onde a especificação ainda tem "A decidir", o ISS adota a escolha abaixo, marcada com `PROVISÓRIO` no código:

| Ponto | Seção | Comportamento atual do ISS |
|---|---|---|
| `ecall`, `ebreak`, instrução ilegal | 9 | Param a simulação com trap; não desviam para `mtvec` |
| Load, store ou AMO desalinhado | 10 | Para a simulação com trap |
| Acesso fora do mapa | 2 | Leitura devolve 0, escrita é ignorada, aviso no stderr |
| `sat` com `n = 0` | 4.3 | Não satura (devolve `rs1`) |
| `relu` com `rs2 != x0` | 4.4 | Ignora `rs2` |
| `funct3`/`funct7` reservados no custom-0 | 4 | Instrução ilegal |
| `mcycle` | 9 | Um ciclo por instrução; o scoreboard não compara ciclos |
