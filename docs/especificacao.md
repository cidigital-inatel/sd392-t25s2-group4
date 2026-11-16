# Especificação da arquitetura e da ISA estendida

Contrato entre o núcleo em RTL (`rtl/`), o modelo de referência (`model/`), o ambiente de verificação (`tb/`) e a biblioteca de software (`sw/`). Tudo o que esses quatro precisam concordar entre si está escrito aqui e em nenhum outro lugar. Nada de RTL, ISS ou intrínseco definitivo antes da versão 1.0 aprovada (meta M2.1, checklist em `docs/checklist.md`).

Responsável: Lucas Lares (A). Revisão: Pedro Piccichelli (B) e Lucas Teixeira (C). Aprovação: os três integrantes e o orientador. A cada versão aprovada é criada a tag `spec-vX.Y` na `main` (ver `docs/convencoes.md`, seção 5). Mudanças depois da 1.0 seguem a seção 7.1 do plano de trabalho e são registradas em `docs/decisoes.md`.

Última atualização: 2026-11-16. Situação: rascunho, versão 0.4, não aprovada.

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
- `funct3` de `011` a `111` e qualquer `funct7` diferente de zero são reservados. Tratamento em "A decidir".

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

A definir no próximo passo. Proposta do plano: `rd ← clamp(rs1, −2^(n−1), 2^(n−1)−1)`, com `n = rs2[4:0]`, comparação com sinal.

### 4.4 `relu`: ativação

A definir no próximo passo. Proposta do plano: `rd ← 0` se `rs1 < 0` com sinal, senão `rd ← rs1`; `rs2 = x0`.

A decidir:

- `sat`: largura em registrador (`rs2[4:0]`, como o plano propõe) ou em imediato; comportamento com `n = 0`, `n = 1` e `rs2[31:5]` diferente de zero.
- `relu`: comportamento quando `rs2` não é `x0`: ignorar o campo ou tratar como reservada.
- `funct3` e `funct7` reservados: instrução ilegal ou `nop`. Depende do tratamento de instrução ilegal na seção 9.

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

Sinais expostos pelo estágio WB a cada instrução retirada, no estilo RVFI: PC, instrução, `rd` e valor escrito, endereço e dado de memória, indicação de trap. É o que o monitor de trace da M3.1 captura e o que o scoreboard compara com o ISS na M3.2. A mesma tupla define o formato do trace em texto do ISS.

A decidir:

- Lista exata de sinais e larguras.
- Se o trace inclui o valor de `rs1` e `rs2` lidos (RVFI inclui) ou só o resultado.
- Se o trace inclui escritas de CSR. O `ACC` da `mac` (seção 4.2) aparece em `rd` a cada `mac`, mas uma escrita por `csrw` só é observada na `mac` ou no `csrr` seguinte.
- Como representar o retire de uma instrução comprimida (instrução de 16 bits ou já expandida).

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
