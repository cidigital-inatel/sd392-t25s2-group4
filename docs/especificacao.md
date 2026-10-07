# Especificação da arquitetura e da ISA estendida

Contrato entre o núcleo em RTL (`rtl/`), o modelo de referência (`model/`), o ambiente de verificação (`tb/`) e a biblioteca de software (`sw/`). Tudo o que esses quatro precisam concordar entre si está escrito aqui e em nenhum outro lugar. Nada de RTL, ISS ou intrínseco definitivo antes da versão 1.0 aprovada (meta M2.1, checklist em `docs/checklist.md`).

Responsável: Lucas Lares (A). Revisão: Pedro Piccichelli (B) e Lucas Teixeira (C). Aprovação: os três integrantes e o orientador. A cada versão aprovada é criada a tag `spec-vX.Y` na `main` (ver `docs/convencoes.md`, seção 5). Mudanças depois da 1.0 seguem a seção 7.1 do plano de trabalho e são registradas em `docs/decisoes.md`.

Última atualização: 2026-10-07. Situação: rascunho, versão 0.1, não aprovada.

Convenção deste documento: cada seção começa dizendo o que define e termina com a lista "A decidir", com os pontos ainda abertos. Quando uma seção não tiver mais nada em "A decidir", ela está pronta para revisão.

## 1. Escopo

O que o processador implementa e o que fica de fora. Lista das extensões (RV32I, M, A, C, Zicsr, custom-0), o modo de privilégio suportado, o que é parametrizável e a ordem de prioridade se for preciso cortar escopo (plano, seção 7.3).

A decidir:

- Confirmar que só o modo máquina (M) é implementado.
- Confirmar quais CSRs da Zicsr entram (depende da seção 9).

## 2. Mapa de memória e periféricos

Endereço base, tamanho e tipo de acesso permitido de cada região: memória de instruções (IMEM), memória de dados (DMEM) e periféricos. Para cada periférico, o endereço, a largura, o sentido (leitura, escrita ou ambos) e a semântica de cada registrador: console, `tohost` e espelhos dos contadores. Comportamento de acessos fora das regiões mapeadas.

A decidir:

- Confirmar as bases e os tamanhos propostos no plano: IMEM em `0x0000_0000` e DMEM em `0x1000_0000`, 64 KiB cada.
- Tamanho da DMEM: 100 imagens MNIST de 784 bytes ocupam cerca de 78 KB, mais do que 64 KiB. Escolher entre DMEM maior ou carga de uma imagem por vez pelo testbench. Afeta a pessoa C (M4.1, M4.4).
- Endereços e semântica do console (`0x2000_0000`) e do `tohost` (`0x2000_0004`) propostos na M4.1; confirmar aqui.
- Se os espelhos dos contadores entram como periféricos ou se só os CSRs bastam.
- O que acontece em acesso fora do mapa: ignorar, devolver zero ou gerar trap.

## 3. Reset e término

Estado do processador após o reset (PC, registradores, CSRs) e como um programa sinaliza que terminou, com sucesso ou com erro, para o testbench e para o ISS encerrarem a simulação com o mesmo código.

A decidir:

- Vetor de reset (proposta: `0x0000_0000`, parametrizável).
- Protocolo de término pelo `tohost`: proposta da M4.1 é escrever `(código << 1) | 1`, com `1` significando sucesso, mesma convenção do Spike e dos `riscv-tests`. Confirmar.
- Se o banco de registradores é zerado no reset ou fica indefinido.

## 4. Instruções custom

As três instruções para redes neurais quantizadas: `mac`, `sat` e `relu`. Tabela de codificação completa (opcode, `funct3`, `funct7`, campos `rd`, `rs1`, `rs2`), sintaxe em assembly, semântica exata em pseudocódigo bit a bit, e o comportamento em todos os casos de borda. Esta seção é a base para o RTL (M2.4), o ISS (M3.2), os intrínsecos (M4.2) e os covergroups (M3.3).

A decidir:

- Confirmar a codificação proposta: opcode `0001011` (custom-0), `funct3` = `000` (`mac`), `001` (`sat`), `010` (`relu`), `funct7` = `0000000`.
- Semântica exata da `mac`: largura dos operandos (int8 × int8, int32 × int32 ou outra), acumulação em 32 bits, comportamento em overflow.
- Como a `mac` lê o acumulador em `rd`: terceira porta de leitura no banco, acumulador dedicado, execução em dois ciclos ou acumulador em `rs1`. Registrar o efeito no forwarding.
- Largura da saturação: em registrador (`rs2[4:0]`, como proposto) ou em imediato.
- Comportamento de `sat` com `n = 0`.
- Comportamento com `rd = x0`.
- Comportamento com `funct3` ou `funct7` reservados: instrução ilegal ou `nop`.

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

A decidir:

- Onde fica a remontagem de instruções que cruzam a palavra: no IF, com buffer, ou na própria interface de memória.
- Comportamento com o parâmetro de C desligado: instrução comprimida vira ilegal e desvio para endereço desalinhado a 4 bytes gera trap.

## 9. CSRs e traps

Lista dos CSRs implementados com endereço, bits implementados e comportamento de leitura e escrita. Tratamento de `ecall`, `ebreak`, instrução ilegal e eventuais exceções de endereço: se geram trap, para onde o PC vai e o que é salvo.

A decidir:

- CSRs mínimos: `mcycle`/`mcycleh` e `minstret`/`minstreth`. Decidir se `mstatus`, `mtvec`, `mepc`, `mcause` e `mret` entram.
- Como `ecall`, `ebreak` e instrução ilegal são tratados: trap completo, término da simulação ou `nop`.
- Se `minstret` conta as instruções custom como uma instrução cada (sim, por padrão) e se conta instruções descartadas por flush (não).

## 10. Interface de memória

Nome, direção, largura e semântica de cada sinal das duas portas do núcleo: instruções e dados. Handshake que permita latência variável, para o testbench inserir wait states e exercitar stalls. Byte enables, extensão de sinal e alinhamento. É o que o agente de memória da M3.1 implementa.

A decidir:

- Protocolo do handshake (proposta: `req`/`ready` ou `valid`/`ready`) e se a resposta pode chegar no mesmo ciclo.
- Se acessos desalinhados geram trap ou são proibidos.
- Prefixo e convenção de nomes dos sinais.

## 11. Interface de trace de retire

Sinais expostos pelo estágio WB a cada instrução retirada, no estilo RVFI: PC, instrução, `rd` e valor escrito, endereço e dado de memória, indicação de trap. É o que o monitor de trace da M3.1 captura e o que o scoreboard compara com o ISS na M3.2. A mesma tupla define o formato do trace em texto do ISS.

A decidir:

- Lista exata de sinais e larguras.
- Se o trace inclui o valor de `rs1` e `rs2` lidos (RVFI inclui) ou só o resultado.
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
