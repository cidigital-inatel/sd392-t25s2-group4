# Processador RISC-V RV32IMAC com extensões para redes neurais quantizadas

Trabalho de conclusão de curso da 2ª turma do programa CI Digital (polo Inatel), setembro de 2026 a fevereiro de 2027.

Processador RISC-V de 32 bits compatível com RV32IMAC, em pipeline de cinco estágios, descrito em Verilog e estendido com três instruções customizadas para redes neurais quantizadas: multiplicação e acumulação (`mac`), saturação (`sat`) e ativação ReLU (`relu`). A verificação é feita em simulação com UVM no Cadence Xcelium, comparando o processador instrução a instrução com um modelo de referência em C. A demonstração é a classificação de dígitos MNIST em int8 rodando no próprio processador, em duas versões, com e sem as extensões, para medir o ganho em ciclos e instruções.

## Comece por aqui

1. Leia o plano de trabalho: `docs/Plano_de_Trabalho_RISC-V_RV32IMAC_Extensoes.docx`.
2. Monte o ambiente na sua máquina seguindo `docs/ambiente.md`, na ordem das seções.
3. Siga `docs/checklist.md`: é a ordem de execução do projeto inteiro, por semana e por meta, com os critérios de aceite. Marque os itens conforme forem concluídos.

## Equipe

| Pessoa | Frente | Metas do plano |
|---|---|---|
| Lucas Lares (A) | Núcleo em RTL; líder técnico e responsável pela especificação | M2.1 a M2.5 |
| Pedro Piccichelli (B) | Verificação: ambiente UVM e modelo de referência | M3.1 a M3.4 |
| Lucas Teixeira (C) | Plataforma (memórias, periféricos e testbench local), software, IA e consolidação da documentação | M4.1 a M4.4, M5.1 a M5.5 |

As letras A, B e C são as usadas em `docs/checklist.md`. Gestão (atas em rodízio, relatórios de atividades, riscos) é dividida entre os três. Orientador: Elivander.

## Estrutura

| Pasta | Conteúdo |
|---|---|
| `rtl/` | Núcleo em Verilog, memórias e periféricos simulados |
| `tb/` | Ambiente de verificação UVM em SystemVerilog |
| `model/` | Modelo de referência: simulador do conjunto de instruções em C |
| `sw/` | Toolchain, programas de teste, biblioteca de intrínsecos, aplicação MNIST e modelo de IA |
| `docs/` | Plano de trabalho, guia de ambiente, checklist, especificação, plano de verificação, decisões e evidências |

## Convenções

- Documentos em português.
- A especificação da arquitetura e da ISA estendida (`docs/especificacao.md`, meta M2.1) é o contrato entre RTL, modelo de referência, testbench e software. Qualquer mudança nela é aplicada nos quatro ao mesmo tempo e registrada em `docs/decisoes.md`.
- Convenções de branches, commits, revisão, issues e tags: proposta em `docs/convencoes.md`, a aprovar na primeira reunião.
