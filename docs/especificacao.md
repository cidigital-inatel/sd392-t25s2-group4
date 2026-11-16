# Especificação da arquitetura e da ISA estendida

Contrato entre o núcleo em RTL (`rtl/`), o modelo de referência (`model/`), o ambiente de verificação (`tb/`) e a biblioteca de software (`sw/`). Tudo o que esses quatro precisam concordar entre si está escrito aqui e em nenhum outro lugar. Nada de RTL, ISS ou intrínseco definitivo antes da versão 1.0 aprovada (meta M2.1, checklist em `docs/checklist.md`).

Responsável: Lucas Lares (A). Revisão: Pedro Piccichelli (B) e Lucas Teixeira (C). Aprovação: os três integrantes e o orientador. A cada versão aprovada é criada a tag `spec-vX.Y` na `main` (ver `docs/convencoes.md`, seção 5). Mudanças depois da 1.0 seguem a seção 7.1 do plano de trabalho e são registradas em `docs/decisoes.md`.

Última atualização: 2026-11-16. Situação: rascunho, versão 0.8, não aprovada.

Convenção deste documento: cada seção começa dizendo o que define e termina com a lista "A decidir", com os pontos ainda abertos. Quando uma seção não tiver mais nada em "A decidir", ela está pronta para revisão.

## 1. Escopo

Define o que o processador implementa, o que fica fora, o que é parametrizável, a referência de conformidade e a ordem de corte de escopo. Decisão D9 em `docs/decisoes.md`.

### 1.1 O que está dentro

Um núcleo RISC-V de 32 bits, único (um hart), em ordem, em pipeline de cinco estágios (seção 5), sem cache, com memória Harvard (seção 2), que implementa:

| Extensão | Conteúdo | Habilitação |
|---|---|---|
| RV32I | As 40 instruções do conjunto base de inteiros, versão 2.1, incluindo `fence`, `ecall` e `ebreak`. | sempre |
| M | `mul`, `mulh`, `mulhsu`, `mulhu`, `div`, `divu`, `rem`, `remu` (seção 6). | parâmetro |
| A | `lr.w`, `sc.w` e as nove `amo*.w` (seção 7). | parâmetro |
| C | Todas as instruções RV32C que não dependem de ponto flutuante (seção 8). | parâmetro |
| Zicsr | `csrrw`, `csrrs`, `csrrc`, `csrrwi`, `csrrsi`, `csrrci`, sobre os CSRs da seção 9.1. | sempre |
| `Xqnn` | `mac`, `sat` e `relu` sobre o opcode `custom-0`, mais o CSR `macacc` (seção 4). | parâmetro |
| Privilegiada | Modo máquina apenas, com o mecanismo de trap, `mret`, `wfi` e os CSRs da seção 9. | sempre |

`Xqnn`, de quantized neural network, é o nome da extensão custom, no formato que a ISA reserva para extensões não padrão: `X` seguido de letras minúsculas. Neste documento, "extensão custom", "unidade custom" e `Xqnn` são a mesma coisa. O nome aparece no bit `X` do `misa` (seção 9.1), no parâmetro de habilitação (seção 12), no nome do módulo em Verilog e na documentação. A toolchain não o conhece: o software emite as instruções com `.insn` (seção 4.1).

### 1.2 O que está fora

Fora do escopo de propósito, não por esquecimento. Cada item traz o motivo ou a seção que o justifica:

- Interrupções e temporizador (`mtime`, `mtimecmp`): não há fonte de interrupção no sistema simulado; `mie` e `mip` leem zero (seção 9).
- Modos supervisor e usuário, e o que vem com eles: `medeleg`, `mideleg`, `mcounteren`, PMP, memória virtual, `sret`, `sfence.vma`. Só modo máquina (seção 9).
- Modo de depuração (`dret`, Debug Module).
- Ponto flutuante (F, D), extensão B, extensão V e qualquer outra extensão padrão fora de M, A, C e Zicsr.
- Zifencei (`fence.i`): a memória de instruções não é escrita pelo programa (seção 2); `fence.i` é instrução ilegal (seção 9).
- Zicntr e Zihpm: as sombras `cycle`, `time` e `instret` e os contadores de desempenho; o software usa `mcycle` e `minstret` (seção 9).
- Acesso a dados desalinhado atendido pelo hardware: gera exceção (seções 9 e 10).
- Caches, buffer de escrita, predição dinâmica de desvio, execução fora de ordem.
- Barramento padrão (AXI, OBI): a interface é a da seção 10; a adaptação é trabalho futuro.
- Prototipação em FPGA e síntese para silício: toda a validação é em simulação. A síntese lógica genérica da M2.5 é opcional e só para estimativa de recursos.
- Mais de um núcleo: a atomicidade da extensão A vem de haver um só mestre (seção 10).

### 1.3 O que é parametrizável

Vetor de reset, tamanho da IMEM e da DMEM, habilitação individual de M, A, C e `Xqnn`, e os wait states das memórias padrão. Nomes, tipos e valores padrão na seção 12. Com uma extensão desligada, suas instruções são ilegais e o bit correspondente do `misa` lê zero (seção 9).

### 1.4 Referência e conformidade

A ISA de referência é a do RISC-V Instruction Set Manual: Volume I, não privilegiado, versão 20191213, e Volume II, privilegiado, versão 20211203, as mesmas citadas no plano de trabalho. Onde a ISA deixa escolha ao projetista (acesso desalinhado, `mtval`, `wfi`, CSRs opcionais, codificações do `custom-0`), a escolha está nesta especificação.

Conformidade do subconjunto padrão: a suíte `riscv-tests`, grupos `rv32ui`, `rv32um`, `rv32ua` e `rv32uc`, adaptada ao mapa de memória e ao `tohost` (M4.1), aprovada no RTL (M2.2 e M2.3) e no ISS (M3.2), com o ISS também comparado ao Spike. Ressalvas:

- `fence_i` fica fora: Zifencei fora do escopo (seção 2).
- `ma_data`, do `rv32ui`, exercita acessos desalinhados. Como aqui eles geram exceção, a M4.1 verifica se o teste aceita o trap com os códigos 4 e 6 (seção 9.2); se aceitar, entra; se não, fica fora como o `fence_i`, com registro em `docs/decisoes.md`.

As instruções `Xqnn` não têm suíte externa: são verificadas contra o ISS, contra a implementação independente em C (seção 4) e contra os vetores de referência da CNN (M4.3), conforme o plano de verificação da M3.3.

### 1.5 Ordem de corte de escopo

Se for preciso cortar, a ordem de prioridade é a da seção 7.3 do plano de trabalho, do que fica até o fim ao que sai primeiro:

1. Núcleo RV32I com a extensão `Xqnn`, verificado. É o essencial e não é cortado.
2. Extensões M e A.
3. Aplicação de demonstração comparativa (M4.4).
4. Extensão C e otimizações.

Todo corte é uma mudança de escopo: issue, linha na seção 3 de `docs/decisoes.md` e aprovação do orientador (plano, seção 7.1). A extensão C já tem o ponto de decisão marcado para a revisão de riscos da semana 11 (risco R2).

A decidir: nada. Seção pronta para revisão.

## 2. Mapa de memória e periféricos

O endereço de 32 bits é decodificado pelos quatro bits mais altos, `addr[31:28]`, que escolhem a região. Dentro da região, os bits baixos endereçam a palavra; quantos bits são usados depende do tamanho configurado (seção 12).

| Região | `addr[31:28]` | Base | Tamanho padrão | Porta | Acesso |
|---|---|---|---|---|---|
| IMEM | `0x0` | `0x0000_0000` | 64 KiB | Instruções | Só busca de instruções |
| DMEM | `0x1` | `0x1000_0000` | 128 KiB | Dados | Leitura e escrita de 8, 16 e 32 bits |
| Periféricos | `0x2` | `0x2000_0000` | 4 palavras | Dados | Leitura e escrita de 32 bits (ver abaixo) |
| Demais | `0x3` a `0xF` | | | | Fora do mapa |

Regras:

- **Harvard estrita.** A porta de instruções enxerga só a IMEM; a porta de dados enxerga só a DMEM e os periféricos. Para o software (M4.1), o linker script põe `.text` na IMEM e `.rodata`, `.data` e `.bss` na DMEM. Como o testbench e o ISS carregam os dois `.hex` antes do reset, o `crt0` não copia `.data`; só zera `.bss`. Consequência para a verificação: o teste `fence_i` dos `rv32ui` escreve na memória de instruções e fica fora da suíte (Zifencei não está no escopo).
- **Tamanhos parametrizáveis.** IMEM e DMEM são parâmetros do topo (seção 12) com os padrões da tabela, iguais no RTL e no ISS. O tamanho é sempre potência de dois.
- **DMEM de 128 KiB.** O plano propôs 64 KiB; 128 KiB é a menor potência de dois que comporta a demonstração com 100 imagens residentes, conforme o orçamento abaixo. Decisão D2 em `docs/decisoes.md`.
- **Fora do mapa.** Acesso a uma região não mapeada, ou além do tamanho configurado da IMEM, da DMEM ou dos periféricos: a leitura devolve 0 e a escrita é ignorada, no RTL e no ISS, sem exceção, porque a interface de memória não tem sinal de erro (seção 10) e a seção 9 não define exceções de acesso. O testbench acusa qualquer acesso desses como erro do teste. Uma busca de instrução fora do mapa devolve a palavra zero, que é instrução ilegal (seção 9): um salto para o nada vira trap.

Orçamento da DMEM para a demonstração MNIST (estimativas; a M4.3 confirma os valores da rede):

| Conteúdo | Tamanho |
|---|---|
| 100 imagens de 28 × 28 em int8 | 78.400 B |
| Pesos e vieses da CNN (poucos milhares de parâmetros) | ~6 KiB |
| Ativações em int8, requantizadas a cada saída (sem buffer int32) | ~10 KiB |
| `.data`, `.bss` e pilha | ~8 KiB |
| Total | ~102 KiB de 128 KiB |

Os vetores de referência camada a camada (cerca de 9 KiB por imagem) não cabem para as 100 imagens; a comparação camada a camada da M4.4 é feita em poucas imagens ou fora do processador, a partir da memória lida pelo testbench.

Periféricos. Cada registrador se comporta como uma palavra de memória de 32 bits com byte enables, e o efeito colateral acontece em toda escrita que habilite o byte 0:

| Endereço | Nome | Leitura | Escrita |
|---|---|---|---|
| `0x2000_0000` | `console` | Retorna 0 | O byte `[7:0]` é impresso como um caractere pelo testbench e pelo ISS |
| `0x2000_0004` | `tohost` | Retorna o último valor escrito (0 após o reset) | Com bit 0 = 1, encerra a simulação com código `valor >> 1`; o valor `1` é sucesso. Com bit 0 = 0, só armazena |
| `0x2000_0008`, `0x2000_000C` | reservados | Retorna 0 | Ignorada |

O protocolo do `tohost` é o dos `riscv-tests` e do Spike: `RVTEST_PASS` escreve `1` e `RVTEST_FAIL` escreve `(número_do_teste << 1) | 1`. Os testes rodam sem alteração no RTL, no ISS e no Spike, com o mesmo código de término. O Spike não tem o `console` neste endereço; a M4.1 decide como os programas imprimem lá.

Não há espelhos dos contadores nos periféricos: o software lê `mcycle` e `minstret` com `csrr` (seção 9), como a HAL da M4.1 já prevê. Decisão D4 em `docs/decisoes.md`.

A decidir: nada. Seção pronta para revisão.

## 3. Reset e término

Define o estado do processador após o reset e como um programa sinaliza que terminou, para o testbench e o ISS começarem iguais e encerrarem na mesma instrução, com o mesmo código. Decisão D9 em `docs/decisoes.md`.

### 3.1 Reset

O reset é o `rst_n` da seção 10: síncrono, ativo em nível baixo. O núcleo entra em reset em qualquer borda de subida de `clk` com `rst_n = 0`; uma borda basta, mas o testbench padrão segura o reset por vários ciclos, e o núcleo não depende de quantos. Enquanto `rst_n = 0`, `ibus_valid` e `dbus_valid` ficam em 0, nenhuma instrução está em voo e nenhum estado muda. No primeiro ciclo com `rst_n = 1`, o núcleo emite a busca no vetor de reset e `mcycle` vale 0; `mcycle` incrementa ao fim de cada ciclo daí em diante.

Estado após o reset:

| Estado | Valor | Definido em |
|---|---|---|
| PC | vetor de reset: `0x0000_0000` por padrão, parâmetro | seção 12 |
| `x1` a `x31` | 0 | esta seção |
| CSRs | 0, inclusive `mstatus` (`MIE = 0`, `MPIE = 0`), `mtvec`, `mepc`, `mcause`, `mtval`, `mscratch`, `mcycle`/`mcycleh` e `minstret`/`minstreth`; `misa` e os de identificação são constantes | seção 9.1 |
| `ACC` (`macacc`) | 0 | seção 4.2 |
| Reserva de `lr.w`/`sc.w` | inválida | seção 7 |
| Pipeline e buffer de busca | vazios | seções 5 e 8 |
| IMEM e DMEM | não são afetadas pelo reset. O testbench e o ISS as carregam dos `.hex` antes de soltar o reset; `.rodata` e `.data` já estão na DMEM quando o programa começa | seção 2 |

O banco de registradores é zerado no reset, e o ISS faz o mesmo. Uma SRAM real acordaria com conteúdo indefinido, mas em simulação registrador não inicializado vale X, enquanto no ISS vale 0: qualquer leitura antes de escrita, por exemplo num programa aleatório da M3.3, divergiria no scoreboard sem haver defeito, e o X poderia se espalhar pelo pipeline e poluir o trace. Zerar custa um reset em 31 registradores e elimina essa fonte de ruído. Com `mtvec = 0`, um trap antes de o software programá-lo salta para o vetor de reset; por isso o `crt0` escreve `mtvec` antes de tudo (seção 9.3).

### 3.2 Término

Um programa termina escrevendo no `tohost` (`0x2000_0004`, seção 2) um valor com o bit 0 em 1: `1` é sucesso, `(código << 1) | 1` é falha, e o código de saída da simulação é `valor >> 1`. É o protocolo dos `riscv-tests` e do Spike. O `console` (`0x2000_0000`) imprime e não encerra nada.

- O núcleo não participa do término. O `tohost` é um periférico do `soc_top`; o núcleo não sabe que ele existe e continua buscando e executando depois da escrita.
- O programa, logo depois da escrita, entra num laço infinito (`1: j 1b`). É o que os `riscv-tests` fazem e o que o `crt0` da M4.1 faz no retorno do `main`, usando o valor de retorno como código. Assim o comportamento continua definido e sem efeitos colaterais enquanto a simulação não parar.
- O testbench e o ISS encerram no retire da instrução de store que escreveu no `tohost`. Esse retire aparece no trace com `rvfi_mem_addr = 0x2000_0004` e `rvfi_mem_wmask` incluindo o byte 0 (seção 11), é comparado normalmente e é o último comparado. Como os dois param na mesma instrução, não há "depois" para divergir.
- Nenhuma outra coisa encerra a simulação. `ebreak`, `ecall` e instrução ilegal geram trap (seção 9), e é o tratador que decide, normalmente escrevendo no `tohost` com código de falha. Um limite de ciclos do testbench, para programa que travou, é falha do teste, não término.

A decidir: nada. Seção pronta para revisão.

## 4. Instruções custom

As três instruções para redes neurais quantizadas: `mac`, `sat` e `relu`. Esta seção é a base para o RTL (M2.4), o ISS (M3.2), os intrínsecos (M4.2) e os covergroups (M3.3). A codificação e a semântica partem da tabela 4.1.3 do plano de trabalho; o que o plano deixou em aberto é fechado aqui.

### 4.1 Codificação

Formato R-type sobre o opcode `custom-0`, que a especificação RISC-V reserva para extensões do projetista e garante que nenhuma extensão padrão usa. Os campos `funct3` e `funct7` distinguem as instruções; `funct7` é zero nas três.

| Bits | 31:25 | 24:20 | 19:15 | 14:12 | 11:7 | 6:0 |
|---|---|---|---|---|---|---|
| Campo | `funct7` | `rs2` | `rs1` | `funct3` | `rd` | `opcode` |

| Mnemônico | Sintaxe | `funct3` | `funct7` | `opcode` | Equivalente em `.insn` |
|---|---|---|---|---|---|
| `mac` | `mac rd, rs1, rs2` | `000` | `0000000` | `0001011` (`0x0B`) | `.insn r 0x0B, 0x0, 0x00, rd, rs1, rs2` |
| `sat` | `sat rd, rs1, rs2` | `001` | `0000000` | `0001011` | `.insn r 0x0B, 0x1, 0x00, rd, rs1, rs2` |
| `relu` | `relu rd, rs1` | `010` | `0000000` | `0001011` | `.insn r 0x0B, 0x2, 0x00, rd, rs1, x0` |

Regras comuns às três:

- Os mnemônicos existem nesta especificação, no trace do ISS e na documentação. O assembler GNU não os conhece; o software emite as instruções com a diretiva `.insn r` (M4.2) e o `objdump` as mostra como palavras cruas, decodificadas pelo script da M4.2.
- `rd = x0`: a escrita em `rd` é descartada, como em qualquer R-type. Para `sat` e `relu` a instrução não tem efeito arquitetural; para `mac` o acumulador é atualizado mesmo assim (4.2). Em todos os casos a instrução é retirada e contada em `minstret`.
- `funct3` de `011` a `111` e qualquer `funct7` diferente de zero são reservados. Uma instrução com opcode `custom-0` e campos reservados é instrução ilegal e gera a exceção correspondente (seção 9), nunca `nop`: um `.insn` digitado errado no software falha na hora, em vez de passar em silêncio, e o espaço de codificação fica livre para extensões futuras, porque nenhum programa pode depender de um `nop`. Decisão D7 em `docs/decisoes.md`.

### 4.2 `mac`: multiplicação e acumulação

A unidade custom tem um registrador interno de 32 bits, o acumulador `ACC`, zerado no reset. O software o enxerga como o CSR `macacc`, endereço `0x7C0`, dentro da faixa `0x7C0` a `0x7FF` que a especificação RISC-V reserva para CSRs do projetista em modo máquina, com leitura e escrita pelas instruções Zicsr padrão.

```
ACC ← (ACC + rs1 × rs2) mod 2^32
rd  ← ACC                         (valor novo)
```

- `rs1` e `rs2` são palavras de 32 bits; a `mac` não lê `rd`. O resultado são os 32 bits menos significativos da soma. Os 32 bits baixos de um produto não dependem de os fatores serem lidos com ou sem sinal, então não há variante com e sem sinal.
- Overflow: wrap-around, sem flag e sem exceção. Com o `ACC` valendo `a`, `mac rd, rs1, rs2` deixa em `rd` e no `ACC` exatamente o valor de `mul t, rs1, rs2` seguido de `add rd, a, t` da extensão M. É essa equivalência que permite comparar bit a bit as versões `baseline` e `ext` da aplicação: a saturação nunca é implícita, só pela `sat`.
- `rd = x0`: a cópia para `rd` é descartada, mas o `ACC` é atualizado mesmo assim. É o uso normal no laço interno, quando só o acumulado importa.
- Zerar, ler e carregar o acumulador: `csrwi 0x7C0, 0`, `csrr rd, 0x7C0` e `csrw 0x7C0, rs1`. São instruções Zicsr que o assembler GNU já aceita com o endereço numérico; não há instrução custom nova.
- Um só acumulador: uma acumulação por vez. A aplicação calcula um elemento de saída de cada vez; para intercalar duas acumulações, o software salva e restaura o `ACC` com `csrr` e `csrw`.
- O compilador não conhece o `ACC`. O software zera o `ACC` no início de cada acumulação e não supõe que ele sobreviva a chamadas de função. Os intrínsecos da M4.2 usam `asm volatile` na `mac` e nos acessos ao `ACC`, para o compilador não os reordenar, duplicar nem eliminar (risco R7).
- Referência em C (M4.2), com `acc` representando o `ACC`: `acc = (uint32_t)acc + (uint32_t)a * (uint32_t)b; rd = (int32_t)acc;`. Aritmética sem sinal para o overflow não ser comportamento indefinido em C.
- Na aplicação, a soma de produtos int8 × int8 fica longe de 2^31: um filtro 3 × 3 sobre 16 canais soma 144 produtos de módulo até 2^14, menos de 2^22, mais um viés. O wrap-around existe para a semântica ser completa e determinística, não porque ocorra em uso normal.

Efeitos no pipeline. O plano (seção 4.1.3) deixou em aberto como a `mac` acessa o acumulador. Decisão: **acumulador dedicado na unidade custom, exposto como CSR**, registrada como D3 em `docs/decisoes.md`.

- O banco de registradores continua com duas portas de leitura e uma de escrita. A `mac` lê `rs1` e `rs2` como qualquer R-type e escreve `rd` pelo caminho normal de WB, participando do forwarding como produtora.
- O `ACC` fica na unidade custom, no estágio EX. A `mac` lê o `ACC` e o escreve na borda de relógio do mesmo ciclo em EX, então `mac` consecutivas se encadeiam sem forwarding nem stall.
- Os acessos ao `ACC` pelas instruções Zicsr também executam em EX, em ordem de programa, logo não há hazard entre `csrw`, `csrr` e `mac` além do que a ordem do pipeline já garante.
- A escrita do `ACC` obedece ao mesmo sinal de descarte que as demais escritas de estado: se a instrução em EX for anulada por flush ou por trap de uma instrução mais antiga (seção 9), o `ACC` não muda.
- Requisito para a seção 6: a latência da `mac` é a da `mul`, e `mac` consecutivas nunca param. Com multiplicador de dois ciclos, a `mac` faz o produto no primeiro e a soma no `ACC` no segundo, em pipeline; nesse caso o valor de `rd` fica pronto um estágio depois, como um load, e a unidade de hazards o trata como load-use.
- Motivo da escolha e alternativas descartadas: D3.

### 4.3 `sat`: saturação

Força `rs1` a caber em `n` bits com sinal, com `n = rs2[4:0]`. Decisão D7 em `docs/decisoes.md`.

```
n ← rs2[4:0]
se n = 0:   rd ← rs1                                    sem saturação: n vale como 32
senão:      rd ← clamp(rs1, −2^(n−1), 2^(n−1) − 1)      comparação com sinal
```

- `rs1` é tratado como inteiro de 32 bits com sinal. Para `n = 8`, o resultado fica entre −128 e 127; para `n = 16`, entre −32768 e 32767; para `n = 1`, entre −1 e 0; para `n = 31`, entre −2^30 e 2^30 − 1.
- A largura vem de um registrador, e não de um imediato, para as três instruções terem o mesmo formato R-type e para `n` poder variar em tempo de execução, lido de uma tabela por camada, sem recompilar. O `li` que carrega `n` fica fora do laço e executa uma vez por camada; ler `rs2` não custa nada, porque o R-type já lê dois fontes.
- `n = 0` é identidade: equivale a `n = 32`, e nenhum valor de 32 bits é cortado. Dá um modo "sem saturação" selecionável em tempo de execução, e em hardware sai de graça, porque os limites viram os do próprio int32.
- `rs2[31:5]` é ignorado, como as instruções de deslocamento da ISA ignoram os bits altos de `rs2`. Esses bits não são reservados: um valor diferente de zero neles não gera exceção.
- `rd = x0`: sem efeito (4.1). A `sat` não lê `rd`; `rs1` e `rs2` são os dois fontes informados no trace (seção 11).
- Implementação no RTL: `rs1` cabe em `n` bits com sinal exatamente quando os bits `rs1[31:n−1]` são todos iguais ao bit de sinal `rs1[31]`. Se cabem, `rd = rs1`; se não, `rs1[31]` escolhe o limite: 1 leva a `−2^(n−1)`, 0 leva a `2^(n−1) − 1`. Um ciclo, sem comparador de 32 bits. O ISS implementa pela comparação direta com os dois limites, de propósito, para as duas implementações não compartilharem o mesmo raciocínio (risco R3).
- Referência em C (M4.2 e implementação independente), sem comportamento indefinido:

```c
static inline int32_t sat_ref(int32_t x, uint32_t n) {
    n &= 31u;
    if (n == 0u) return x;
    int32_t hi = (int32_t)((1u << (n - 1u)) - 1u);
    int32_t lo = -hi - 1;
    return x > hi ? hi : (x < lo ? lo : x);
}
```

Exemplos:

| `n` | `rs1` | `rd` |
|---|---|---|
| 8 | 100 | 100 |
| 8 | 200 | 127 |
| 8 | −200 | −128 |
| 8 | `0x7FFF_FFFF` | 127 |
| 8 | `0x8000_0000` | −128 |
| 16 | 40000 | 32767 |
| 16 | −40000 | −32768 |
| 1 | 5 | 0 |
| 1 | −5 | −1 |
| 0 | `0x8000_0000` | `0x8000_0000` |

Saturação sem sinal. Não há variante sem sinal: para forçar um valor ao intervalo `[0, 2^k − 1]`, usa-se `sat` com `n = k + 1` seguida de `relu`. Com `k = 8`, a `sat` com `n = 9` dá `[−256, 255]` e a `relu` dá `[0, 255]`.

### 4.4 `relu`: ativação

Zera os valores negativos. Decisão D7.

```
rd ← 0      se rs1[31] = 1, isto é, rs1 negativo com sinal
rd ← rs1    caso contrário
```

- Só lê `rs1`. O campo `rs2` da codificação é ignorado: o software emite `x0` nele (`.insn r 0x0B, 0x2, 0x00, rd, rs1, x0`), mas qualquer valor produz o mesmo resultado e não gera exceção. Como a instrução não lê `rs2`, o trace informa `rvfi_rs2_addr = 0` e `rvfi_rs2_rdata = 0` (seção 11), seja o que estiver no campo.
- `rd = x0`: sem efeito (4.1).
- Implementação no RTL: cada bit de `rd` é `rs1[i] AND NOT rs1[31]`. Um ciclo, sem comparador.
- Referência em C: `static inline int32_t relu_ref(int32_t x) { return x < 0 ? 0 : x; }`
- Exemplos: `relu` de −1 é 0; de `0x8000_0000` é 0; de 0 é 0; de 1 é 1; de `0x7FFF_FFFF` é `0x7FFF_FFFF`.

### 4.5 Efeitos no pipeline e uso conjunto

`sat` e `relu` executam no estágio EX em um ciclo, como uma operação da ALU, e escrevem `rd` pelo caminho normal de WB. Participam do forwarding como qualquer R-type. `mac`, `sat` e `relu` em sequência, cada uma consumindo o resultado da anterior, encadeiam sem stall quando a `mac` é de um ciclo; com a `mac` de dois ciclos (seção 6), a `sat` logo após ela sofre o stall de load-use descrito em 4.2.

Requantização de uma saída de camada int8, com o acumulado em `t0`, o deslocamento em `t1` e `t2 = 8`:

```
sra  t0, t0, t1      # escala em potência de dois
sat  t0, t0, t2      # [-128, 127]
relu t0, t0          # [0, 127]
sb   t0, 0(a1)
```

Sem desvios condicionais no caminho. Em RV32IMAC puro, saturação e ReLU custam de quatro a oito instruções com desvios; medir esse ganho é a M4.4. A ordem `sat` depois `relu` ou `relu` depois `sat` dá o mesmo resultado, porque os dois intervalos contêm o zero. O arredondamento do deslocamento (truncar ou somar meio antes do `sra`) é escolha da M4.3 e tem de ser a mesma na referência NumPy e no C das duas versões da aplicação.

A decidir: nada. Seção pronta para revisão.

## 5. Pipeline

Os cinco estágios (IF, ID, EX, MEM, WB), o conteúdo dos registradores entre estágios, a unidade de hazards (forwarding EX/MEM e MEM/WB para EX, stall de load-use), a resolução de desvios e o flush. Inclui o diagrama de blocos do pipeline, exigido no aceite da M2.1.

A decidir:

- Estágio em que o desvio é resolvido (proposta: EX, com predição "não tomado") e quantas instruções são descartadas no flush.
- Se há forwarding também para o estágio MEM (dado de store) ou só para EX.
- Como os stalls de latência de memória (seção 10) se propagam pelo pipeline.
- Formato do diagrama de blocos e onde fica (`docs/figuras/`).

## 6. Extensão M

Multiplicação (`mul`, `mulh`, `mulhsu`, `mulhu`) e divisão (`div`, `divu`, `rem`, `remu`): latência de cada operação, se há stall, e os casos especiais de divisão por zero e overflow conforme a especificação RISC-V.

A decidir:

- Multiplicador em 1 ou 2 ciclos.
- Divisor multiciclo com stall: número de ciclos.
- Se o multiplicador é compartilhado com a `mac` ou se a unidade custom tem o próprio.

## 7. Extensão A

`lr.w` e `sc.w` com reserva, e as operações atômicas de memória (`amoswap`, `amoadd`, `amoxor`, `amoand`, `amoor`, `amomin`, `amomax`, `amominu`, `amomaxu`). Como a reserva é mantida e invalidada e como a sequência leitura-modificação-escrita é feita no estágio MEM.

A decidir:

- Granularidade da reserva (endereço exato ou bloco) e o que a invalida.
- Número de ciclos de stall da sequência leitura-modificação-escrita.
- Bits `aq` e `rl`: ignorados (núcleo em ordem, sem cache) ou com algum efeito.

## 8. Extensão C

Instruções comprimidas de 16 bits: alinhamento do PC a 2 bytes, busca de instruções de 32 bits que cruzam a fronteira de palavra, expansão das comprimidas em ID, cálculo de PC + 2, e a habilitação por parâmetro. É a última extensão a ser implementada e a primeira candidata a corte (risco R2 em `docs/decisoes.md`).

Já fixado pela seção 10 (D5): a porta de instruções é sempre de palavra inteira, e a remontagem de uma instrução de 32 bits que cruza a fronteira de palavra é feita no IF, com um buffer, a partir de duas leituras consecutivas. A memória e o agente UVM não sabem que a extensão C existe.

Já fixado pela seção 9 (D8): com o parâmetro de C desligado, toda instrução de 16 bits é instrução ilegal (código 2), e um desvio tomado ou salto para endereço que não é múltiplo de 4 gera a exceção de endereço de instrução desalinhado (código 0), com `mtval` igual ao alvo.

A decidir:

- Redação completa da seção: buffer de remontagem no IF, expansor em ID, cálculo de PC + 2 e os casos de borda da busca que cruza a palavra.

## 9. CSRs e traps

Define o estado de controle do núcleo: a lista dos CSRs implementados, com endereço, bits que existem e comportamento de leitura e escrita; o mecanismo de trap em modo máquina, com as exceções que o núcleo gera, o que cada uma salva e para onde o PC vai; o `mret`; o comportamento dos contadores; e o tratamento das instruções de sistema e das que estão fora do conjunto implementado. Só o modo máquina existe. Não há interrupções: todo trap é uma exceção síncrona, causada pela própria instrução que o sofre. Decisão D8 em `docs/decisoes.md`, com as alternativas descartadas.

### 9.1 CSRs implementados

| Endereço | Nome | Acesso | Bits implementados e comportamento | Reset |
|---|---|---|---|---|
| `0x300` | `mstatus` | leitura e escrita | `MIE` (bit 3) e `MPIE` (bit 7) graváveis; `MPP` (bits 12:11) fixo em `11`. Os demais bits leem 0 e ignoram escrita. Fora do trap e do `mret`, `MIE` não tem efeito, porque não há interrupções. | `MIE = 0`, `MPIE = 0` |
| `0x301` | `misa` | constante | `MXL = 01` (bits 31:30); `I` (bit 8) sempre 1; `M` (bit 12), `A` (bit 0) e `C` (bit 2) iguais aos parâmetros de habilitação da seção 12; `X` (bit 23) igual ao parâmetro da extensão custom. Com tudo habilitado, `0x4080_1105`. Escrita ignorada. | constante |
| `0x304` | `mie` | lê 0 | Escrita ignorada. | 0 |
| `0x305` | `mtvec` | leitura e escrita | `BASE` (bits 31:2) gravável; `MODE` (bits 1:0) fixo em `00`, modo direto. | 0 |
| `0x310` | `mstatush` | lê 0 | Escrita ignorada. | 0 |
| `0x340` | `mscratch` | leitura e escrita | 32 bits livres para o tratador. | 0 |
| `0x341` | `mepc` | leitura e escrita | Bits 31:1 graváveis; bit 0 fixo em 0. Com a extensão C desligada, o bit 1 também fica fixo em 0. | 0 |
| `0x342` | `mcause` | leitura e escrita | 32 bits. O trap escreve o código da tabela de 9.2, com o bit 31 em 0; o software pode escrever qualquer valor e lê o que escreveu. | 0 |
| `0x343` | `mtval` | leitura e escrita | 32 bits. O trap escreve o valor da tabela de 9.2. | 0 |
| `0x344` | `mip` | lê 0 | Escrita ignorada. | 0 |
| `0xB00`, `0xB80` | `mcycle`, `mcycleh` | leitura e escrita | Contador de 64 bits de ciclos (9.4). | 0 |
| `0xB02`, `0xB82` | `minstret`, `minstreth` | leitura e escrita | Contador de 64 bits de instruções retiradas (9.4). | 0 |
| `0xF11` a `0xF15` | `mvendorid`, `marchid`, `mimpid`, `mhartid`, `mconfigptr` | lê 0 | Só leitura pelo endereço: escrita é instrução ilegal. `mhartid = 0` porque há um só núcleo. | constante |
| `0x7C0` | `macacc` | leitura e escrita | 32 bits, o `ACC` da `mac` (seção 4.2). | 0 |

Regras de acesso:

- Qualquer CSR fora da tabela não existe, e acessá-lo, por leitura ou escrita, é instrução ilegal. Inclui `medeleg` e `mideleg` (não há modo supervisor), `mcounteren` (não há modo usuário), `mcountinhibit`, `menvcfg`, os `mhpmcounter*` e `mhpmevent*`, e as sombras sem privilégio `cycle`, `time`, `instret`, `cycleh`, `timeh` e `instreth` (`0xC00` a `0xC02` e `0xC80` a `0xC82`). O software lê os contadores por `mcycle` e `minstret` (seção 2).
- Escrita em endereço de `0xC00` a `0xFFF` é instrução ilegal, pela própria codificação do endereço. Escrita em CSR da tabela marcado "lê 0" ou "constante" é aceita e ignorada.
- `csrrs` e `csrrc` com `rs1 = x0`, e `csrrsi` e `csrrci` com imediato zero, não escrevem (ISA): não disparam a ilegalidade de escrita e não aparecem na porta de CSR do trace (seção 11). `csrrw` e `csrrwi` escrevem sempre, mesmo com `rd = x0`.
- As instruções Zicsr executam no estágio EX, em ordem de programa, e uma escrita de CSR vale para a instrução seguinte (seção 4.2). O `macacc` fica na unidade custom, mas segue as mesmas regras.

### 9.2 Exceções

| Código em `mcause` | Exceção | Quando o núcleo a gera | `mtval` |
|---|---|---|---|
| 0 | Endereço de instrução desalinhado | Só com a extensão C desligada: desvio condicional tomado, `jal` ou `jalr` cujo alvo não é múltiplo de 4. A exceção é da instrução de desvio ou salto. Com C ligada nunca ocorre: `jalr` zera o bit 0 do alvo e todo deslocamento é múltiplo de 2. | O alvo |
| 2 | Instrução ilegal | Codificação que não é de RV32I, da Zicsr, de uma extensão habilitada (M, A, C, custom) nem das instruções de 9.5; codificação reservada do `custom-0` (seção 4.1); instrução de extensão desligada por parâmetro; acesso a CSR inexistente ou escrita em CSR só de leitura; e as palavras `0x0000_0000` e `0xFFFF_FFFF`, que a ISA define como ilegais. | Os bits da instrução: 32 bits para uma instrução de 32 bits; para uma comprimida, os 16 bits em `[15:0]` com `[31:16] = 0`, como em `rvfi_insn`. |
| 3 | Breakpoint | `ebreak` e `c.ebreak`. | O PC do `ebreak` |
| 4 | Load desalinhado | `lw` e `lr.w` com `addr[1:0]` diferente de `00`; `lh` e `lhu` com `addr[0] = 1` (seção 10.3). | O endereço |
| 6 | Store ou AMO desalinhado | `sw`, `sc.w` e `amo*.w` com `addr[1:0]` diferente de `00`; `sh` com `addr[0] = 1`. | O endereço |
| 11 | Chamada de ambiente | `ecall`. | 0 |

Não há exceções de acesso (códigos 1, 5 e 7): a interface de memória não tem sinal de erro (seção 10), e acesso fora do mapa segue a seção 2. Um salto para fora do mapa é pego de outro modo: a leitura devolve a palavra zero, que é instrução ilegal.

Uma instrução gera no máximo uma exceção. Instrução ilegal se decide primeiro, na decodificação; as exceções de endereço só se aplicam a instruções legais. Não há exceção durante um stall nem depois que a instrução passa do EX (9.3).

### 9.3 Mecanismo de trap e `mret`

Quando uma instrução gera exceção:

```
mepc         <- PC da instrução
mcause       <- código da tabela 9.2, bit 31 em 0
mtval        <- valor da tabela 9.2
mstatus.MPIE <- mstatus.MIE
mstatus.MIE  <- 0
PC           <- mtvec.BASE          bits 1:0 em zero
```

- A instrução não completa: não escreve `rd`, não acessa a memória, não escreve CSR pela via normal e não conta em `minstret`. Aparece uma vez no trace, com `rvfi_trap = 1` e `rvfi_pc_wdata = mtvec.BASE` (seção 11, regra 3). As quatro escritas acima, em `mepc`, `mcause`, `mtval` e `mstatus`, não passam pela porta de CSR do trace; são observadas no `csrr` que as ler, e o ISS as faz do mesmo modo.
- As instruções mais novas que já estavam no pipeline são descartadas, como num desvio tomado. O trap é tomado no estágio EX, o mesmo ponto em que os desvios se resolvem e em que as escritas de CSR executam: a ilegalidade, o `ecall` e o `ebreak` são detectados no ID e carregados até o EX; o desalinhamento é calculado no EX, antes de a transação de memória ser emitida no MEM. Consequência para a seção 5: há um só ponto de redirecionamento do PC, e toda instrução que passa do EX sem trap retira com certeza.
- `mret`: `PC <- mepc`, `mstatus.MIE <- mstatus.MPIE`, `mstatus.MPIE <- 1`, `MPP` continua `11`. É um salto incondicional com flush, como `jalr`. No trace, `rvfi_pc_wdata = mepc`; a alteração de `mstatus` segue a mesma regra das escritas do trap e não passa pela porta de CSR.
- Trap dentro do tratador sobrescreve `mepc`, `mcause` e `mtval`, como manda a ISA; o tratador que quiser preservá-los salva antes, por exemplo em `mscratch`.
- `mtvec` vale 0 após o reset. Um trap antes de o software programá-lo salta para o vetor de reset. Recomendação para a M4.1, não requisito do núcleo: o `crt0` escreve `mtvec` antes de qualquer outra coisa, apontando para um tratador de depuração que imprime `mcause`, `mepc` e `mtval` no console e encerra pelo `tohost` com código de falha.

### 9.4 Contadores

- `mcycle` e `mcycleh` formam um contador de 64 bits que incrementa a cada ciclo de relógio a partir da saída do reset, inclusive durante stalls, e nunca para. Uma leitura devolve o valor do contador no ciclo em que a instrução está no EX. Esse valor não é verificável pelo ISS; a seção 11.3 diz como o testbench o entrega a ele.
- `minstret` e `minstreth` formam um contador de 64 bits que incrementa a cada instrução retirada sem trap (seção 11, regra 2). As instruções custom contam uma cada; instruções descartadas por flush e instruções que sofrem trap não contam. Uma leitura devolve o número de instruções retiradas antes dela em ordem de programa. Nota de implementação: como o trap é tomado no EX, toda instrução que sai do EX sem trap retira; o RTL pode incrementar o contador na saída do EX, e então a leitura feita no EX já devolve o valor certo, sem corrigir pelas instruções em voo no MEM e no WB.
- Escrita pelo software: `csrrw`, `csrrs`, `csrrc` e as formas com imediato gravam a metade endereçada, baixa ou alta, e o contador segue dali. Em `minstret`, o valor escrito prevalece e a instrução que escreve não é contada: logo depois de `csrw minstret, zero`, `minstret` vale 0. Esta regra é confirmada contra o Spike na M3.2; se o Spike divergir, prevalece o Spike e esta linha é corrigida.

### 9.5 Instruções de sistema e fora do conjunto

| Instrução | Tratamento |
|---|---|
| `ecall` | Exceção, código 11. |
| `ebreak`, `c.ebreak` | Exceção, código 3. |
| `mret` | Retorno de trap (9.3). |
| `wfi` | `nop`, como a ISA permite. |
| `fence` | `nop`: um só mestre, sem cache, toda ordenação já é respeitada. |
| `fence.i` | Instrução ilegal: a Zifencei está fora do escopo (seção 2). |
| `sret`, `uret`, `dret`, `sfence.vma` | Instrução ilegal: não há modos supervisor, usuário nem depuração. |
| Instrução de extensão desligada por parâmetro (seção 12) | Instrução ilegal. Com C desligada, toda instrução de 16 bits é ilegal e um alvo de desvio ou salto não múltiplo de 4 gera o código 0. |

A simulação só termina pelo `tohost` (seção 2). Nenhuma exceção encerra a simulação por si; quem decide é o tratador.

A decidir: nada. Seção pronta para revisão.

## 10. Interface de memória

Define as duas portas pelas quais o núcleo (`core_top`) acessa tudo o que está fora dele: a porta de instruções, prefixo `ibus_`, e a porta de dados, prefixo `dbus_`. Para cada sinal, nome, direção, largura e semântica; o protocolo de handshake; as regras de alinhamento e de byte enables; e o que o núcleo faz enquanto espera. É o contrato entre o núcleo, as memórias e os periféricos de `soc_top` (M2.2, itens da pessoa C) e o agente de memória do ambiente UVM (M3.1), que ocupa o lugar das memórias no testbench. O modelo de referência não implementa o protocolo: ele enxerga os acessos pelo trace de retire (seção 11). A escolha de qual região atende cada endereço é do decodificador de `soc_top`, pela seção 2, e não do núcleo.

Decisão D5 em `docs/decisoes.md`: protocolo `valid`/`ready` de uma fase, com o dado de leitura no mesmo ciclo do `ready`, e porta de instruções alinhada a palavra. Alternativas descartadas: D5.

### 10.1 Sinais

Relógio e reset, comuns às duas portas e a todo o núcleo:

| Sinal | Direção no núcleo | Largura | Semântica |
|---|---|---|---|
| `clk` | entrada | 1 | Relógio. Todo registrador do núcleo é sensível à borda de subida. |
| `rst_n` | entrada | 1 | Reset síncrono, ativo em nível baixo. Com `rst_n = 0`, `ibus_valid` e `dbus_valid` ficam em 0 e nenhuma transação é emitida. O estado após o reset é o da seção 3. |

Porta de instruções. O núcleo é o mestre e só lê.

| Sinal | Direção no núcleo | Largura | Semântica |
|---|---|---|---|
| `ibus_valid` | saída | 1 | Há um pedido de leitura em curso. |
| `ibus_addr` | saída | 32 | Endereço de byte da palavra pedida, sempre alinhado a 4: `ibus_addr[1:0] = 00`. |
| `ibus_ready` | entrada | 1 | O escravo atende o pedido neste ciclo. |
| `ibus_rdata` | entrada | 32 | Palavra lida. Só tem valor válido no ciclo em que `ibus_valid` e `ibus_ready` estão em 1. |

Porta de dados. O núcleo é o mestre; lê e escreve.

| Sinal | Direção no núcleo | Largura | Semântica |
|---|---|---|---|
| `dbus_valid` | saída | 1 | Há um pedido em curso. |
| `dbus_addr` | saída | 32 | Endereço de byte do acesso, completo, inclusive `dbus_addr[1:0]`. O escravo escolhe a palavra por `dbus_addr[31:2]` e pode ignorar os bits baixos, porque quais bytes participam está em `dbus_be`. |
| `dbus_we` | saída | 1 | 1 para escrita, 0 para leitura. |
| `dbus_be` | saída | 4 | Byte enables. O bit `i` corresponde ao byte `i` da palavra, isto é, a `dbus_wdata[8i+7:8i]` e `dbus_rdata[8i+7:8i]`. Na escrita, só os bytes com bit em 1 são alterados. Na leitura, indica os bytes que o núcleo vai usar; o escravo pode ignorar e devolver a palavra inteira. Com `dbus_valid = 1`, nunca é `0000`. |
| `dbus_wdata` | saída | 32 | Dado de escrita, alinhado por lane (10.3). Os bytes não habilitados em `dbus_be` têm valor indefinido. |
| `dbus_ready` | entrada | 1 | O escravo atende o pedido neste ciclo. |
| `dbus_rdata` | entrada | 32 | Palavra lida. Só tem valor válido no ciclo em que `dbus_valid` e `dbus_ready` estão em 1 e `dbus_we = 0`. |

Não há sinal de erro, de resposta de escrita nem de exclusividade. Acesso fora do mapa ou além do tamanho configurado segue a seção 2: o escravo responde com `ready = 1`, devolve 0 na leitura e ignora a escrita; é o testbench que acusa. As instruções da extensão A (seção 7) usam a porta de dados com transações comuns: `lr.w` é uma leitura, `sc.w` é uma escrita quando a reserva vale e nenhuma transação quando não vale, e cada `amo*.w` é uma leitura seguida de uma escrita, com o pipeline parado entre as duas. A atomicidade vem de o núcleo ser o único mestre do sistema.

### 10.2 Protocolo

Handshake de uma fase. A transação acontece no ciclo em que `valid` e `ready` estão em 1 ao mesmo tempo, e tem efeito na borda de subida de `clk` que encerra esse ciclo. As regras valem igualmente para as duas portas:

1. O mestre levanta `valid` junto com `addr` e, na porta de dados, `we`, `be` e `wdata`, e mantém todos estáveis, ciclo a ciclo, até o ciclo em que `ready = 1`. Um pedido não é retirado nem alterado antes de ser atendido.
2. `ready` pode depender combinacionalmente de `valid` e dos demais sinais do mestre no mesmo ciclo. `valid` e os demais sinais do mestre nunca dependem de `ready`. É o que impede um laço combinacional entre núcleo e escravo.
3. O dado de leitura é entregue no mesmo ciclo do `ready`. Um escravo com `ready` sempre em 1 se comporta como uma memória de leitura combinacional: o dado sai no mesmo ciclo em que o endereço entra. Essa é a latência mínima, zero ciclos de espera, e é o comportamento padrão de `imem` e `dmem`.
4. O escravo pode manter `ready = 0` por qualquer número de ciclos, os wait states, inclusive variando de um acesso para outro. O núcleo não depende de latência fixa: o estágio que fez o pedido para enquanto espera, e a propagação dessa parada pelo pipeline é a da seção 5.
5. Um handshake é exatamente uma transação. Se no ciclo seguinte `valid` continuar em 1, é um pedido novo, mesmo com o mesmo endereço. Consequência para o núcleo: quando uma porta completa o handshake e a outra ainda espera, o núcleo baixa o `valid` da porta que terminou e guarda o dado recebido até o pipeline avançar (seção 5). O escravo nunca supõe que `valid` permanece depois do handshake.
6. Uma transação pendente por porta. O núcleo só emite um pedido novo numa porta depois do handshake do anterior. As duas portas são independentes e podem ter transações em curso ao mesmo tempo, com latências diferentes.
7. Uma escrita tem efeito na borda de relógio do handshake. No ciclo seguinte, uma leitura do mesmo endereço já vê o valor novo. Não há buffer de escrita.

Busca de instruções com zero wait states, uma palavra por ciclo:

```
ciclo         1       2       3
ibus_valid    1       1       1
ibus_addr     0x00    0x04    0x08
ibus_ready    1       1       1
ibus_rdata    I0      I1      I2
```

A mesma busca com dois wait states no primeiro acesso. Nos ciclos 1 e 2 o IF está parado e repete o pedido; a transação do endereço 0 acontece no ciclo 3:

```
ciclo         1       2       3       4       5
ibus_valid    1       1       1       1       1
ibus_addr     0x00    0x00    0x00    0x04    0x08
ibus_ready    0       0       1       1       1
ibus_rdata    --      --      I0      I1      I2
```

Escrita de um byte, `sb t0, 2(a0)` com `a0 = 0x1000_0000` e `t0 = 0xAB`, com um wait state. O byte viaja no lane 2 e só o bit 2 de `dbus_be` está em 1; a escrita acontece na borda que encerra o ciclo 2:

```
ciclo         1             2
dbus_valid    1             1
dbus_addr     0x1000_0002   0x1000_0002
dbus_we       1             1
dbus_be       0100          0100
dbus_wdata    0x00AB_0000   0x00AB_0000
dbus_ready    0             1
```

### 10.3 Alinhamento, byte enables e extensão de sinal

Porta de instruções. A porta é sempre de palavra inteira. O PC pode estar alinhado a 2 por causa da extensão C (seção 8), mas `ibus_addr` é o PC com o bit 1 zerado, e o IF escolhe a metade da palavra que lhe interessa. Uma instrução de 32 bits que começa na metade alta de uma palavra é remontada no IF, com um buffer, a partir de duas leituras de palavras consecutivas. A memória e o agente UVM não sabem que a extensão C existe.

Porta de dados. O dado viaja alinhado por lane: o byte de endereço `a` ocupa sempre a posição `a[1:0]` dentro da palavra, tanto em `dbus_wdata` quanto em `dbus_rdata`. O núcleo monta `dbus_be` e `dbus_wdata` a partir de `dbus_addr[1:0]` e do tamanho do acesso, e na leitura extrai de `dbus_rdata` o lane certo e faz a extensão de sinal ou de zero. Tudo isso é feito no núcleo, no estágio MEM; a memória só lê e escreve bytes inteiros.

| Acesso | `dbus_addr[1:0]` | `dbus_be` | Lane em `wdata` e `rdata` | Valor escrito em `rd` na leitura |
|---|---|---|---|---|
| `lw`, `sw`, `lr.w`, `sc.w`, `amo*.w` | `00` | `1111` | `[31:0]` | `rdata[31:0]` |
| `lh`, `lhu`, `sh` | `00` | `0011` | `[15:0]` | `rdata[15:0]`, com extensão de sinal em `lh` e de zero em `lhu` |
| `lh`, `lhu`, `sh` | `10` | `1100` | `[31:16]` | `rdata[31:16]`, idem |
| `lb`, `lbu`, `sb` | `00` | `0001` | `[7:0]` | `rdata[7:0]`, com extensão de sinal em `lb` e de zero em `lbu` |
| `lb`, `lbu`, `sb` | `01` | `0010` | `[15:8]` | `rdata[15:8]`, idem |
| `lb`, `lbu`, `sb` | `10` | `0100` | `[23:16]` | `rdata[23:16]`, idem |
| `lb`, `lbu`, `sb` | `11` | `1000` | `[31:24]` | `rdata[31:24]`, idem |

Acessos desalinhados: `lw`, `sw` e as instruções da extensão A com `dbus_addr[1:0]` diferente de `00`, e `lh`, `lhu`, `sh` com `dbus_addr[0] = 1`. O núcleo não emite a transação e gera a exceção de endereço desalinhado de load ou de store (seção 9). É a escolha que a ISA deixa ao projetista no lugar de atender o acesso em duas transações; o compilador, com as flags do projeto, não gera acessos desalinhados. Os periféricos da seção 2 são acessados com `lw` e `sw`; uma escrita de byte ou de meia palavra neles segue a regra do byte 0 da seção 2.

### 10.4 Memórias e periféricos padrão

Os escravos de `soc_top`, itens da pessoa C na M2.2, obedecem a este protocolo com o comportamento padrão de zero wait states, `ready = 1` sempre, e leitura combinacional. `imem` e `dmem` têm um parâmetro de wait states, com padrão 0 e nome fixado na seção 12, que faz o `ready` subir só depois de N ciclos em cada acesso, para a simulação local com Verilator ou Icarus exercitar os stalls sem depender do agente UVM. `periph` não tem wait states. Os tamanhos e o decodificador de regiões são os da seção 2.

A decidir: nada. Seção pronta para revisão.

## 11. Interface de trace de retire

Define os sinais que o núcleo (`core_top`) expõe a cada instrução retirada, isto é, a cada instrução que completa o estágio WB e torna seus efeitos visíveis ao programa. É o que o monitor de trace da M3.1 captura, o que o scoreboard da M3.2 compara em lock-step com o modelo de referência e a base das asserções de retire da M3.3. A mesma tupla, com os mesmos campos e a mesma semântica, é o que o ISS devolve a cada passo pela interface DPI-C e o que ele imprime no trace em texto. A grafia da linha de texto, ordem e formato dos campos, é da M3.2 e fica documentada em `model/README.md`; esta seção fixa os campos e o significado.

Os nomes, as larguras e a semântica seguem o RVFI (RISC-V Formal Interface, do projeto riscv-formal), no subconjunto da tabela abaixo, acrescido de uma porta genérica de escrita de CSR. Do RVFI completo ficam fora `rvfi_halt`, `rvfi_intr`, `rvfi_mode` e `rvfi_ixl`: a simulação termina pelo `tohost`, não há interrupções, o modo é sempre máquina e a largura é sempre 32. Decisão D6 em `docs/decisoes.md`, com as alternativas descartadas.

### 11.1 Sinais

Todos são saídas do núcleo, registradas no estágio WB, e só têm valor válido no ciclo em que `rvfi_valid = 1`. Nos demais ciclos o valor é indefinido e não deve ser lido.

| Grupo | Sinal | Largura | Semântica |
|---|---|---|---|
| Controle | `rvfi_valid` | 1 | Uma instrução retirou neste ciclo. No máximo uma por ciclo, em ordem de programa. |
| | `rvfi_order` | 64 | Número de ordem da instrução. Vale 0 na primeira instrução retirada após o reset e cresce de 1 em 1, sem pulo nem repetição. |
| | `rvfi_trap` | 1 | A instrução causou uma exceção (seção 9). |
| Instrução e PC | `rvfi_insn` | 32 | A instrução como foi buscada. Instrução de 32 bits: a palavra inteira. Instrução comprimida (seção 8): os 16 bits em `[15:0]`, com `[31:16] = 0`; reconhece-se por `rvfi_insn[1:0]` diferente de `11`. |
| | `rvfi_pc_rdata` | 32 | PC da instrução. |
| | `rvfi_pc_wdata` | 32 | PC da instrução seguinte em ordem de programa: PC + 4, PC + 2 para comprimida, alvo do desvio ou salto tomado, ou o endereço do tratador quando `rvfi_trap = 1`. |
| Fontes | `rvfi_rs1_addr`, `rvfi_rs2_addr` | 5, 5 | Registradores fonte lidos, já com a instrução comprimida expandida. 0 quando a instrução não lê aquele fonte. |
| | `rvfi_rs1_rdata`, `rvfi_rs2_rdata` | 32, 32 | Valores efetivamente usados pela instrução, depois do forwarding. 0 quando o fonte correspondente não é lido. |
| Destino | `rvfi_rd_addr` | 5 | Registrador escrito. 0 quando a instrução não escreve registrador, inclusive quando o destino codificado é `x0`. |
| | `rvfi_rd_wdata` | 32 | Valor escrito. 0 quando `rvfi_rd_addr = 0`. |
| Memória | `rvfi_mem_addr` | 32 | Endereço de byte do acesso de dados, igual ao `dbus_addr` da transação (seção 10). Indefinido quando as duas máscaras são zero. |
| | `rvfi_mem_rmask`, `rvfi_mem_wmask` | 4, 4 | Bytes lidos e bytes escritos, com a regra de lane de `dbus_be`. Zero quando não houve leitura ou escrita. |
| | `rvfi_mem_rdata`, `rvfi_mem_wdata` | 32, 32 | Palavra lida e palavra escrita, alinhadas por lane como `dbus_rdata` e `dbus_wdata`. Só os bytes marcados na máscara correspondente têm valor definido e são comparados. |
| CSR | `rvfi_csr_we` | 1 | A instrução escreveu um CSR. |
| | `rvfi_csr_addr` | 12 | Endereço do CSR escrito. Indefinido quando `rvfi_csr_we = 0`. |
| | `rvfi_csr_wdata` | 32 | Valor que ficou no CSR depois da instrução. Indefinido quando `rvfi_csr_we = 0`. |

### 11.2 Regras

1. Retire é a saída do estágio WB. Instrução descartada por flush, seja por desvio tomado, seja por trap de uma instrução mais antiga, nunca aparece no trace. Nos ciclos de stall, `rvfi_valid = 0`.
2. `rvfi_order` começa em 0 e conta todas as instruções que aparecem no trace, com ou sem trap. `minstret` (seção 9) conta só as com `rvfi_trap = 0`: a cada instante, `minstret` é igual ao número de retires sem trap até ali.
3. Instrução com `rvfi_trap = 1` aparece uma vez, com `rvfi_rd_addr = 0`, máscaras de memória zero e `rvfi_csr_we = 0`; `rvfi_pc_wdata` é o endereço para onde o PC foi. As escritas que o trap faz nos CSRs de estado (seção 9) não passam pela porta de CSR: são observadas no `csrr` que as ler, e o ISS as faz do mesmo modo.
4. Porta de CSR: informa a única escrita de CSR que uma instrução pode fazer. `csrrw`, `csrrs`, `csrrc` e as formas com imediato informam o CSR acessado e o valor final, exceto quando a ISA define que não há escrita (`csrrs` e `csrrc` com `rs1 = x0`, `csrrsi` e `csrrci` com imediato zero), caso em que `rvfi_csr_we = 0`. A `mac` (seção 4.2) informa `rvfi_csr_addr = 0x7C0` e o novo valor do `ACC`, mesmo com `rd = x0`: é por aqui que o `ACC` fica visível ao scoreboard a cada `mac`. Leituras de CSR aparecem em `rvfi_rd_wdata`, como qualquer resultado. Os incrementos automáticos de `mcycle` e `minstret` não são escritas e não aparecem.
5. Extensão A (seção 7): `lr.w` é uma leitura comum. `sc.w` com sucesso informa a escrita e `rvfi_rd_wdata = 0`; `sc.w` que falha informa máscaras zero e `rvfi_rd_wdata = 1`. `amo*.w` informa as duas máscaras em `1111` no mesmo `rvfi_mem_addr`, o valor lido em `rvfi_mem_rdata`, o valor escrito em `rvfi_mem_wdata` e o valor lido em `rvfi_rd_wdata`.
6. Instruções sem efeito arquitetural, como `nop`, `fence` e `sat` ou `relu` com `rd = x0`, aparecem com `rvfi_rd_addr = 0`, máscaras zero e `rvfi_csr_we = 0`; o que resta é `insn`, `pc_rdata` e `pc_wdata`.

Exemplo com cinco instruções. Campos não mostrados valem zero; traços marcam bytes indefinidos. `a0` é `x10`, `t0` é `x5`, `t1` é `x6`; o `ACC` vale 0 antes da `mac`:

```
lui  a0, 0x10000    order 0  pc 0x0000_0000  pc_wdata 0x0000_0004  rd x10 = 0x1000_0000
addi t0, x0, -1     order 1  pc 0x0000_0004  pc_wdata 0x0000_0008  rs1 x0 = 0
                                                                   rd x5 = 0xFFFF_FFFF
sh   t0, 2(a0)      order 2  pc 0x0000_0008  pc_wdata 0x0000_000C  rs1 x10 = 0x1000_0000  rs2 x5 = 0xFFFF_FFFF
                             mem_addr 0x1000_0002  wmask 1100  mem_wdata 0xFFFF_----
lbu  t1, 3(a0)      order 3  pc 0x0000_000C  pc_wdata 0x0000_0010  rs1 x10 = 0x1000_0000
                             mem_addr 0x1000_0003  rmask 1000  mem_rdata 0xFF--_----  rd x6 = 0x0000_00FF
mac  x0, t1, t0     order 4  pc 0x0000_0010  pc_wdata 0x0000_0014  rs1 x6 = 0x0000_00FF   rs2 x5 = 0xFFFF_FFFF
                             rd x0  csr_we 1  csr_addr 0x7C0  csr_wdata 0xFFFF_FF01
```

Na `mac`, `0xFF × 0xFFFF_FFFF` módulo 2^32 é `0xFFFF_FF01`, que vai para o `ACC` e aparece na porta de CSR; `rd = x0`, então `rvfi_rd_addr` e `rvfi_rd_wdata` são zero.

### 11.3 Comparação com o modelo de referência

A cada retire, o scoreboard compara `rvfi_insn`, `rvfi_pc_rdata`, `rvfi_pc_wdata`, `rvfi_trap`, os seis campos de fontes e destino, os bytes marcados dos campos de memória e os três campos de CSR. `rvfi_order` é conferido contra o contador do próprio scoreboard. Campos marcados como indefinidos nesta seção não são comparados.

Uma exceção: o valor lido de `mcycle` e `mcycleh` não é verificável por um modelo com precisão de instrução, que não sabe quantos ciclos o pipeline gastou. Antes de o ISS executar uma leitura desses contadores, o testbench entrega a ele o valor que o RTL leu, e o ISS usa esse valor; assim RTL e ISS seguem iguais em `rvfi_rd_wdata` e em tudo o que o programa fizer com esse valor depois, inclusive gravá-lo na memória. A forma de entregar o valor é da interface DPI-C da M3.2. Executando sozinho, sem RTL, o ISS define `mcycle` por uma regra própria e determinística, documentada em `model/README.md`. `minstret` não precisa disso: o ISS conta instruções com exatidão.

A decidir: nada. Seção pronta para revisão.

## 12. Parâmetros do topo

Parâmetros Verilog dos módulos `core_top` e `soc_top`, com nome, tipo, valor padrão e efeito: vetor de reset, tamanhos das memórias, habilitação individual das extensões M, A, C e custom.

Já fixado pela seção 9 (D8): com uma extensão desligada, toda instrução dela é instrução ilegal (código 2), e o bit correspondente do `misa` lê 0.

A decidir:

- Nomes e valores padrão de cada parâmetro, inclusive o de wait states de `imem` e `dmem` (seção 10.4).

## 13. Histórico de versões

| Versão | Data | Autor | Mudança |
|---|---|---|---|
| 0.1 | 2026-10-07 | Lucas Lares | Esqueleto com as seções e os pontos a decidir. |
| 0.2 | 2026-10-07 | Lucas Lares | Seção 2 preenchida: regiões, periféricos, protocolo do `tohost` e orçamento da DMEM. |
| 0.3 | 2026-10-07 | Lucas Lares | Seção 4: codificação das três instruções e semântica da `mac`, com acumulador dedicado exposto como CSR `0x7C0`. |
| 0.4 | 2026-11-16 | Lucas Lares | Seção 10: interface de memória com as portas `ibus_` e `dbus_`, protocolo `valid`/`ready` de uma fase com resposta no mesmo ciclo, byte enables alinhados por lane (D5). Seção 8 registra a remontagem das instruções que cruzam a palavra no IF. |
| 0.5 | 2026-11-16 | Lucas Lares | Seção 11: trace de retire no padrão RVFI, com valores das fontes depois do forwarding, porta genérica de escrita de CSR, instrução comprimida crua e regra de sincronização do `mcycle` com o ISS (D6). |
| 0.6 | 2026-11-16 | Lucas Lares | Seções 4.3 e 4.4: semântica de `sat` (largura em `rs2[4:0]`, `n = 0` identidade, `rs2[31:5]` ignorado) e de `relu` (não lê `rs2`); codificações reservadas do `custom-0` como instrução ilegal; nova 4.5 com efeitos no pipeline e a sequência de requantização (D7). |
| 0.7 | 2026-11-16 | Lucas Lares | Seção 9: mecanismo de trap em modo máquina tomado no EX, tabela de CSRs, exceções com códigos e `mtval`, `mret`, contadores e instruções de sistema (D8). Fechados os itens que dependiam da 9 nas seções 2, 4, 8, 10, 11 e 12; as seções 2, 4, 9, 10 e 11 ficam sem pendências. |
| 0.8 | 2026-11-16 | Lucas Lares | Seções 1 e 3: escopo com o que está dentro e fora, parâmetros, conformidade e ordem de corte, extensão custom nomeada `Xqnn`; reset com banco zerado e tabela de estado; término pelo `tohost` com o núcleo continuando e o fim no retire do store (D9). Seções 1 e 3 sem pendências. |
