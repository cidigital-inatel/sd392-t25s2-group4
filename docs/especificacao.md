# Especificação da arquitetura e da ISA estendida

Contrato entre o núcleo em RTL (`rtl/`), o modelo de referência (`model/`), o ambiente de verificação (`tb/`) e a biblioteca de software (`sw/`). Tudo o que esses quatro precisam concordar entre si está escrito aqui e em nenhum outro lugar. Nada de RTL, ISS ou intrínseco definitivo antes da versão 1.0 aprovada (meta M2.1, checklist em `docs/checklist.md`).

Responsável: Lucas Lares (A). Revisão: Pedro Piccichelli (B) e Lucas Teixeira (C). Aprovação: os três integrantes e o orientador. A cada versão aprovada é criada a tag `spec-vX.Y` na `main` (ver `docs/convencoes.md`, seção 5). Mudanças depois da 1.0 seguem a seção 7.1 do plano de trabalho e são registradas em `docs/decisoes.md`.

Última atualização: 2026-11-16. Situação: rascunho, versão 0.6, não aprovada.

Convenção deste documento: cada seção começa dizendo o que define e termina com a lista "A decidir", com os pontos ainda abertos. Quando uma seção não tiver mais nada em "A decidir", ela está pronta para revisão.

## 1. Escopo

O que o processador implementa e o que fica de fora. Lista das extensões (RV32I, M, A, C, Zicsr, custom-0), o modo de privilégio suportado, o que é parametrizável e a ordem de prioridade se for preciso cortar escopo (plano, seção 7.3).

A decidir:

- Confirmar que só o modo máquina (M) é implementado.
- Confirmar quais CSRs da Zicsr entram (depende da seção 9).

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
- **Fora do mapa.** Acesso a uma região não mapeada, ou além do tamanho configurado da IMEM, da DMEM ou dos periféricos: ver "A decidir".

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

A decidir:

- Acesso fora do mapa ou além do tamanho configurado. Proposta: leitura retorna 0 e escrita é ignorada, no RTL e no ISS, e o testbench acusa qualquer acesso desses como erro do teste. Se a seção 9 adotar traps de acesso, passa a gerar trap.
- Acessos desalinhados: seção 10.

## 3. Reset e término

Estado do processador após o reset (PC, registradores, CSRs) e como um programa sinaliza que terminou, com sucesso ou com erro, para o testbench e para o ISS encerrarem a simulação com o mesmo código.

A decidir:

- Vetor de reset (proposta: `0x0000_0000`, parametrizável).
- Sequência de término vista pelo programa: a escrita no `tohost` (seção 2) encerra a simulação; definir o que o núcleo faz depois dela, se continua executando ou para.
- Se o banco de registradores é zerado no reset ou fica indefinido.

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

A decidir:

- Fechar com a seção 9 a exceção de instrução ilegal gerada pelas codificações reservadas do `custom-0`: código de causa e o que é salvo.

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

A decidir:

- Comportamento com o parâmetro de C desligado: instrução comprimida vira ilegal e desvio para endereço desalinhado a 4 bytes gera trap.

## 9. CSRs e traps

Lista dos CSRs implementados com endereço, bits implementados e comportamento de leitura e escrita. Tratamento de `ecall`, `ebreak`, instrução ilegal e eventuais exceções de endereço: se geram trap, para onde o PC vai e o que é salvo. O CSR custom `macacc` (`0x7C0`), acumulador da `mac`, está definido na seção 4.2 e entra nesta lista.

A decidir:

- CSRs mínimos: `mcycle`/`mcycleh`, `minstret`/`minstreth` e `macacc`. Decidir se `mstatus`, `mtvec`, `mepc`, `mcause` e `mret` entram.
- Como `ecall`, `ebreak` e instrução ilegal são tratados: trap completo, término da simulação ou `nop`.
- Se `minstret` conta as instruções custom como uma instrução cada (sim, por padrão) e se conta instruções descartadas por flush (não).

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

A decidir:

- Fechar com a seção 9 o tratamento da exceção de acesso desalinhado: código de causa, valor salvo e para onde o PC vai. Se a seção 9 não adotar traps, definir aqui um comportamento determinístico comum ao núcleo e ao ISS.

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

A decidir:

- Fechar com a seção 9 o que `rvfi_pc_wdata` informa num trap, o endereço do tratador, e a lista dos CSRs de estado que o trap altera fora da porta de CSR. Se a seção 9 não adotar traps, `rvfi_trap` fica sempre em 0 e permanece na interface por compatibilidade com o RVFI.

## 12. Parâmetros do topo

Parâmetros Verilog dos módulos `core_top` e `soc_top`, com nome, tipo, valor padrão e efeito: vetor de reset, tamanhos das memórias, habilitação individual das extensões M, A, C e custom.

A decidir:

- Nomes e valores padrão de cada parâmetro.
- Comportamento quando uma extensão está desligada: instrução ilegal.

## 13. Histórico de versões

| Versão | Data | Autor | Mudança |
|---|---|---|---|
| 0.1 | 2026-10-07 | Lucas Lares | Esqueleto com as seções e os pontos a decidir. |
| 0.2 | 2026-10-07 | Lucas Lares | Seção 2 preenchida: regiões, periféricos, protocolo do `tohost` e orçamento da DMEM. |
| 0.3 | 2026-10-07 | Lucas Lares | Seção 4: codificação das três instruções e semântica da `mac`, com acumulador dedicado exposto como CSR `0x7C0`. |
| 0.4 | 2026-11-16 | Lucas Lares | Seção 10: interface de memória com as portas `ibus_` e `dbus_`, protocolo `valid`/`ready` de uma fase com resposta no mesmo ciclo, byte enables alinhados por lane (D5). Seção 8 registra a remontagem das instruções que cruzam a palavra no IF. |
| 0.5 | 2026-11-16 | Lucas Lares | Seção 11: trace de retire no padrão RVFI, com valores das fontes depois do forwarding, porta genérica de escrita de CSR, instrução comprimida crua e regra de sincronização do `mcycle` com o ISS (D6). |
| 0.6 | 2026-11-16 | Lucas Lares | Seções 4.3 e 4.4: semântica de `sat` (largura em `rs2[4:0]`, `n = 0` identidade, `rs2[31:5]` ignorado) e de `relu` (não lê `rs2`); codificações reservadas do `custom-0` como instrução ilegal; nova 4.5 com efeitos no pipeline e a sequência de requantização (D7). |
