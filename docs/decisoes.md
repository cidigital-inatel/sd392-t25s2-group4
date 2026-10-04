# Registro de decisões, riscos e mudanças

Registro único das decisões de projeto, dos riscos acompanhados e das mudanças de escopo. É consultado nas reuniões semanais e revisado formalmente nas revisões de riscos e mudanças (meta M1.4: semanas 3, 7, 11, 14, 17 e 19). As semanas citadas neste registro são as do cronograma replanejado, descrito em `docs/checklist.md`.

Última atualização: 2026-10-04.

## 1. Decisões tomadas

Uma linha por decisão técnica ou de organização, na ordem em que foram tomadas. Decisões que alteram o escopo do plano de trabalho entram também na seção 3.

| Nº | Data | Decisão | Motivo | Alternativas consideradas |
|---|---|---|---|---|
| D1 | 2026-10-04 | Cronograma replanejado de 24 para 19 semanas de trabalho, com o mesmo escopo. As janelas de cada meta estão em `docs/checklist.md`. | A execução começa em 05/10/2026 e a entrega continua em 28/02/2027, com recesso de 21/12/2026 a 03/01/2027. | Reduzir o escopo desde já, retirando a extensão C. Não adotada: fica como ponto de decisão na revisão de riscos da semana 11 (risco R2). |

## 2. Riscos

Riscos mapeados no plano de trabalho (seção 7.4). A coluna "Situação" é atualizada a cada revisão. Riscos novos são acrescentados ao final da tabela.

| Nº | Risco | Impacto | Mitigação / plano de ação | Situação |
|---|---|---|---|---|
| R1 | Indisponibilidade ou limitação de licença do Xcelium | Impossibilidade de executar o ambiente UVM | Confirmar acesso e licenças nas semanas 1–2; manter o RTL e os testes básicos executáveis em simulador alternativo; avaliar simulador acadêmico equivalente para a UVM. | Aberto |
| R2 | Complexidade da extensão C (alinhamento de 16 bits e busca desalinhada) | Atraso em M2.3 e na regressão | Implementar C por último dentro de M2.3; se necessário, entregar RV32IMA verificado e registrar a redução de escopo. | Aberto |
| R3 | Erros no modelo de referência mascarando erros do RTL | Falsa sensação de correção | Validação cruzada com Spike e riscv-tests; implementações independentes em C das instruções customizadas; revisão por pares. | Aberto |
| R4 | Tempo de simulação da inferência MNIST em RTL | Demonstração incompleta | Usar subconjunto de imagens; validar camadas isoladamente com vetores de referência; otimizar o testbench (desativar dumps de forma de onda). | Aberto |
| R5 | Curva de aprendizado de UVM e SystemVerilog | Atraso em M3.1–M3.3 | Iniciar o ambiente com smoke test simples na semana 2; estudo dirigido com material da Accellera/Cadence; pareamento entre integrantes. | Aberto |
| R6 | Dificuldades na construção da toolchain | Atraso em M4.1 | Utilizar binários pré-compilados; documentar versões; testar o fluxo com programa mínimo antes dos riscv-tests. | Aberto |
| R7 | Compilador reordenar, duplicar ou eliminar instruções asm inline | Resultados incorretos na aplicação | Restrições de operandos corretas (`"+r"`, `"=r"`, `"r"`); testes unitários dos intrínsecos; inspeção do código gerado com objdump. | Aberto |
| R8 | Definição tardia da semântica das instruções (MAC com leitura de `rd`, largura de SAT) | Retrabalho em RTL, modelo e software | Fechar a especificação (M2.1) até a semana 4 com aprovação do orientador; tratar mudanças posteriores pela seção 7.1 do plano (seção 3 deste registro). | Aberto |

## 3. Mudanças de escopo

Toda inclusão, remoção ou alteração de escopo (plano de trabalho, seção 7.1): mudança na semântica de uma instrução, redução do conjunto de extensões, troca de ferramenta. Cada mudança é também uma issue no repositório. Mudanças na especificação (`docs/especificacao.md`) são aprovadas em reunião com o orientador e aplicadas ao mesmo tempo no RTL, no modelo de referência, no plano de verificação e na biblioteca de software.

| Nº | Data | Descrição | Motivação | Impacto em prazos e entregáveis | Alternativas consideradas | Decisão | Issue |
|---|---|---|---|---|---|---|---|
