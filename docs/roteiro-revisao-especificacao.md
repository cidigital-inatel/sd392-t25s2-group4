# Roteiro da reunião de aprovação da especificação, versão 1.0

Guia para a reunião em que os três integrantes e o orientador aprovam `docs/especificacao.md`, fechando a meta M2.1. O documento tem cerca de mil linhas; este roteiro resume o que precisa ser aprovado em uma página de decisões, aponta o que merece discussão e lista o que a aprovação dispara. Lê-se em quinze minutos; a reunião cabe em uma hora.

Versão em revisão: 0.14, na `main` do repositório, resultado dos pull requests 2 a 14. Preparado por Lucas Lares em 2026-11-16.

## 1. Como usar este roteiro

1. Antes da reunião, cada participante lê a especificação uma vez. Para quem tem pouco tempo, a ordem que dá o quadro geral em quarenta minutos: seção 1 (escopo), seção 4 (as instruções `Xqnn`), seção 5.1 (diagrama de blocos), seção 9.2 e 9.3 (exceções e trap), e depois as tabelas das seções 10 e 11.
2. Na reunião, percorre-se a tabela da seção 3 deste roteiro, uma decisão por vez: aprova, ou anota a objeção. Decisão com objeção volta para a especificação como mudança registrada; as demais ficam aprovadas.
3. Em seguida, os pontos de atenção da seção 4, que são as perguntas que um revisor faria.
4. Por fim, os critérios de aceite da seção 5 e o procedimento da seção 6.

## 2. O que mudou em relação ao plano de trabalho

O plano de trabalho deixou pontos "a confirmar na M2.1". A especificação os fechou, e em alguns casos escolheu diferente do que o plano propunha. Nada foi retirado do escopo; estes são os desvios que o orientador precisa conhecer:

| Ponto | O plano dizia | A especificação fixou | Decisão |
|---|---|---|---|
| Tamanho da memória de dados | 64 KiB | 128 KiB, para as 100 imagens MNIST residentes | D2 |
| Semântica da `mac` | `rd ← rd + rs1 × rs2`, lendo `rd` (terceira porta de leitura ou alternativa) | Acumulador dedicado `ACC` na unidade, exposto como CSR `0x7C0`; `rd ← ACC` novo. O intrínseco em C muda de forma | D3 |
| Contadores de ciclos e instruções | Espelhados em registradores de periférico | Só os CSRs `mcycle` e `minstret`, lidos com `csrr` | D4 |
| Latência da multiplicação | "um ou dois ciclos, a definir pela análise de caminho crítico" | Um ciclo, sem alvo de síntese; limitação registrada para uma versão em FPGA | D11 |
| CSRs de trap (`mstatus`, `mtvec`, `mepc`, `mcause`) | "se necessário" | Implementados, com `mtval` e `mret`: mecanismo de trap completo em modo máquina | D8 |
| Acesso a dados desalinhado | Não tratado | Gera exceção, como a ISA permite | D8 |
| Nome da extensão | "extensões customizadas" | `Xqnn`, no formato da ISA para extensões não padrão | D9 |
| Largura da saturação | Registrador ou imediato, a decidir | Registrador, `n = rs2[4:0]`, com `n = 0` como identidade | D7 |

Prazo: a M2.1 venceu na semana 4 (01/11); a aprovação acontece na semana 7 ou depois. As seções de que a M3.1 e a M4.1 dependem, interfaces de memória e de trace, instruções `Xqnn` e mapa de memória, estão estáveis desde os primeiros pull requests, então essas metas não precisam esperar a tag para começar.

## 3. As catorze decisões, uma por linha

Cada linha é uma decisão registrada em `docs/decisoes.md`, com motivo e alternativas descartadas lá. Aqui, só o essencial para aprovar.

| Nº | O que foi decidido | Por quê | O que custa | Seção | Aprovada? |
|---|---|---|---|---|---|
| D1 | Cronograma replanejado para 19 semanas, com o mesmo escopo | Início em 05/10, entrega em 28/02, recesso de 21/12 a 03/01 | Janelas mais curtas; extensão C é a candidata a corte na revisão da semana 11 | checklist | organizacional, já em vigor |
| D2 | DMEM de 128 KiB | 100 imagens de 28 × 28 em int8 são 78 KB, mais que os 64 KiB do plano | Nada em simulação | 2 | |
| D3 | `mac` acumula em `ACC` dedicado, CSR `0x7C0`, e copia o novo `ACC` em `rd` | Acumulador dentro da unidade, como num DSP; sem terceira porta no banco; `mac` consecutivas sem stall; acumulado visível ao scoreboard | Um só acumulador; estado fora do banco; intrínsecos com `asm volatile` | 4.2 | |
| D4 | Sem espelhos dos contadores nos periféricos | `csrr` já dá o dado; espelho seria uma segunda interface para o mesmo valor | Nenhum | 2 | |
| D5 | Interface de memória `valid`/`ready` de uma fase, dado no mesmo ciclo do `ready`; porta de instruções alinhada a palavra | Protocolo mais simples para núcleo, memórias e agente UVM; latência variável exercita os stalls; sem alvo de síntese, memória combinacional não custa | Irrealista para SRAM síncrona; uma versão sintetizada mudaria para duas fases | 10 | |
| D6 | Trace de retire no padrão RVFI, com valores de `rs1`/`rs2` depois do forwarding e porta genérica de escrita de CSR | Compatível com riscv-formal; erro de forwarding aparece na própria instrução; `ACC` visível a cada `mac` mesmo com `rd = x0` | Uns 150 flip-flops a mais nos registradores de pipeline | 11 | |
| D7 | `sat` com largura em `rs2[4:0]`, `n = 0` identidade, `rs2[31:5]` ignorado; `relu` só lê `rs1`; codificações reservadas são ilegais, nunca `nop` | Três instruções no mesmo formato R-type; largura variável por camada; `.insn` errado falha na hora | Nenhum relevante | 4.3 a 4.5 | |
| D8 | Trap em modo máquina com `mtvec`, `mepc`, `mcause`, `mtval`, `mstatus` mínimo e `mret`, tomado no EX; lista fechada de CSRs; CSR inexistente é ilegal | Coerente com D5 e D7, que exigem exceções; igual ao Spike; tratador de depuração transforma erro de software em mensagem | Cinco registradores de CSR e um caminho de flush compartilhado com os desvios | 9 | |
| D9 | Escopo fechado; extensão nomeada `Xqnn`; banco zerado no reset; término pelo `tohost` com o fim no retire do store | Lista do que está fora evita "esqueceram X"; banco zerado elimina X na comparação com o ISS; RTL e ISS param na mesma instrução | Nenhum | 1, 3 | |
| D10 | Nove parâmetros do topo; carga dos `.hex` pelo testbench por referência hierárquica em `u_soc.u_imem.mem`; latência fixa, não parâmetro | Parâmetro de arquivo exigiria recompilar o RTL por programa; o checklist já dava a carga ao testbench | Nomes de instância viram contrato | 12 | |
| D11 | Multiplicador de 33 × 33 em um ciclo; divisor com restauração, latência constante até 40 ciclos, com stall; multiplicador próprio na unidade `Xqnn` | Sem síntese, um ciclo elimina hazards de load-use em `mul` e `mac`; divisão quase não aparece na aplicação; unidade independente de `EN_M` | Caminho combinacional longo, inaceitável em FPGA sem mudar para dois ciclos | 6 | |
| D12 | Reserva de `lr`/`sc` de uma palavra, invalidada só por `sc.w` e reset; `amo*.w` em duas transações com ALU própria no MEM; `aq`/`rl` ignorados | Mínimo que a ISA obriga; suíte `rv32ua` exige granularidade fina; núcleo em ordem sem cache já satisfaz acquire e release | Um ciclo a mais por `amo` | 7 | |
| D13 | Extensão C com buffer de uma meia-palavra no IF, expansor combinacional no ID, tabela de expansão reproduzida na especificação | Com resposta no mesmo ciclo, 16 bits de buffer bastam para uma instrução por ciclo; a tabela faz expansor, ISS e gerador lerem a mesma fonte | É a parte do risco R2; cobertura mais fina aqui | 8 | |
| D14 | Toda mudança de fluxo resolvida no EX, flush de duas instruções; forwarding só para o EX; paradas propagadas por domínio; instrução com trap segue como casca até o WB; diagrama em Mermaid | Um só ponto de redirecionamento; regra de load-use em uma frase; retire único e em ordem para o trace | Um ciclo em `lw` seguido de `sw` do mesmo registrador | 5 | |

## 4. Pontos de atenção

Perguntas que um revisor pode fazer, com a resposta que a equipe dá. Se a resposta não convencer, o item vira objeção na tabela acima.

1. **Multiplicador de 32 bits em um ciclo e memória de leitura combinacional são realistas?** Não para chip ou FPGA, e a especificação diz isso nas seções 1.2, 6.1 e 10.2. O projeto não tem alvo de síntese; a síntese da M2.5 é opcional e só para estimativa. As duas escolhas eliminam classes inteiras de hazards e de casos de verificação. Uma versão sintetizada mudaria para multiplicador em dois ciclos e protocolo em duas fases; os dois caminhos estão registrados em D5 e D11.
2. **Por que a `mac` não compartilha o multiplicador da extensão M?** A `mac` só precisa dos 32 bits baixos, a unidade fica independente de `EN_M`, e um defeito num multiplicador não mascara o outro. A área duplicada só aparece na estimativa opcional.
3. **A reserva de `lr`/`sc` não é invalidada por trap. Isso é correto?** A ISA só obriga a invalidação pelo próprio `sc.w`; invalidar em trap é permitido, não exigido, e serve a sistemas com sistema operacional, que não temos. A suíte `rv32ua` passa.
4. **`mtval` implementado: vale o custo?** Um registrador e um seletor de três entradas com dados que já estão no EX. É o que permite ao tratador dizer "store desalinhado em `0x1000_0003`" em vez de "deu erro".
5. **Banco de registradores zerado no reset não é artificial?** É. Em simulação, registrador não inicializado vale X e o modelo de referência vale 0; qualquer leitura antes de escrita divergiria sem haver defeito. Sem FPGA, zerar não custa nada.
6. **A regra de escrita em `minstret` foi conferida?** Sim: a ISA manda que a escrita prevaleça sobre o incremento, e o Spike implementa exatamente isso. A M3.2 confere como teste normal.
7. **A latência do divisor não tem número.** A seção 6.2 fixa a constância e o teto de 40 ciclos e diz que o valor exato entra na própria seção quando o RTL da M2.3 existir. A reunião deve combinar que esse preenchimento é atualização de versão prevista, não mudança de escopo pela seção 7.1 do plano. O mesmo vale para a avaliação do teste `ma_data` na M4.1, prevista na seção 1.4.
8. **O diagrama de blocos aparece?** Ele é texto Mermaid na seção 5.1, desenhado pelo GitHub. Confirmar na reunião que todos o veem desenhado; para o documento do TCC ele é exportado para imagem na M5.2.
9. **A sincronização do `mcycle` entre RTL e ISS é viável?** O ISS não sabe quantos ciclos o pipeline gastou; a seção 11.3 pede que o testbench entregue ao ISS o valor que o RTL leu antes de o ISS executar a leitura. O Pedro confirma a forma na interface DPI-C da M3.2.
10. **A extensão C ainda está no escopo?** Sim, por último, como o plano manda, com `EN_C = 0` dando um RV32IMA completo. O ponto de decisão do corte continua na revisão de riscos da semana 11.

## 5. Critérios de aceite da M2.1

Do plano de trabalho: "especificação versionada, diagrama de blocos do pipeline e tabela de codificação aprovados pela equipe".

- [ ] Especificação versionada: versão 0.14 na `main`, histórico na seção 13, cada versão entrou por pull request.
- [ ] Diagrama de blocos do pipeline: seção 5.1, desenhado e coerente com as seções 5.2 a 5.6.
- [ ] Tabela de codificação das instruções `Xqnn`: seção 4.1, com `opcode`, `funct3`, `funct7` e o equivalente em `.insn`.
- [ ] Nenhuma seção com itens em "A decidir".
- [ ] Decisões D2 a D14 registradas em `docs/decisoes.md` com motivo e alternativas.
- [ ] Revisão pelos três integrantes feita.
- [ ] Aprovação do orientador.

## 6. O que a aprovação dispara

1. Um commit na `main`, por pull request, trocando o cabeçalho da especificação para "versão 1.0, aprovada em <data>", acrescentando a linha 1.0 no histórico e marcando os dois últimos itens da M2.1 no checklist. Depois do merge, a tag `spec-v1.0` na `main`, que não é movida.
2. A situação do risco R8 em `docs/decisoes.md` passa a "fechado".
3. Começam, com a especificação como contrato: M2.2, o núcleo RV32I em RTL; M3.2, o modelo de referência; M4.2, os intrínsecos. A M3.1 e a M4.1 já podiam ter começado com o rascunho e continuam.
4. Daqui em diante, qualquer mudança na especificação segue a seção 7.1 do plano: issue, linha na seção 3 de `docs/decisoes.md`, aprovação em reunião com o orientador e atualização simultânea de RTL, ISS, plano de verificação e biblioteca de software. Exceções previstas: o valor da latência do divisor na 6.2 e a avaliação do `ma_data` na 1.4, que a reunião classifica como preenchimentos previstos.

## 7. Perguntas por frente, para confirmar na reunião

Para o Pedro, verificação:

- As seções 10 e 11 bastam para o agente de memória, o monitor de trace e o scoreboard da M3.1 e da M3.2?
- A lista do código 2 na seção 9.2 é o que o gerador de instruções aleatórias precisa para nunca produzir programa inválido sem querer?
- As regras 1 a 6 da seção 11.2 e as tabelas da seção 5.6 viram asserções da M3.3 como estão?
- A entrega do `mcycle` ao ISS, seção 11.3, é viável na DPI-C?

Para o Lucas Teixeira, plataforma e software:

- O linker script e o conversor ELF para `.hex` da M4.1 seguem a seção 2 e o formato da seção 12.2?
- O `crt0` escreve `mtvec` antes de tudo, aponta para o tratador de depuração da seção 9.3 e, no retorno do `main`, escreve o `tohost` e entra em laço infinito, seção 3.2?
- Os intrínsecos da M4.2 saem direto das seções 4.2 a 4.4, com a referência em C de lá?
- `imem`, `dmem` e `periph` seguem as seções 10.4 e 12.2, com a instância `u_soc` no testbench local?

Para o orientador:

- Os desvios em relação ao plano, seção 2 deste roteiro, são aceitáveis sem mudança formal de escopo, dado que nada foi retirado?
- A ordem de corte da seção 1.5 da especificação continua a mesma do plano?
