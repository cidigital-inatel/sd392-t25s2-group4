# Guia de ambiente

Como preparar uma máquina do zero para trabalhar no projeto. Siga as seções na ordem. Cada ferramenta traz o que é, para que serve, quando instalar, os comandos de instalação e como verificar. Quando uma nova ferramenta entrar no projeto, ela é acrescentada aqui no mesmo passo, com versão e comandos.

Última atualização: 2026-11-16.

## 1. Requisitos

| Item | Requisito |
|---|---|
| Sistema | Windows 10 ou 11 com WSL 2, ou Linux nativo. Os comandos abaixo assumem Ubuntu 24.04 (no WSL ou nativo). |
| Editor | VS Code, de preferência com a extensão WSL para abrir a pasta do projeto dentro do Ubuntu. |
| Git | No Windows, Git for Windows (git-scm.com), para clonar o repositório. Ele traz o gerenciador de credenciais, que abre o navegador no primeiro acesso ao GitHub. Em Linux nativo, o `git` do sistema. |
| Disco | Cerca de 15 GB livres no Linux (toolchain, Spike, simuladores e ambiente Python). |
| Rede | Acesso à internet para os downloads. |

## 2. WSL 2 com Ubuntu 24.04

O WSL permite rodar Ubuntu dentro do Windows. Todo o fluxo de compilação e simulação local do projeto roda nele.

No PowerShell como administrador:

```
wsl --install -d Ubuntu-24.04
```

Reinicie o Windows, abra "Ubuntu" no menu Iniciar e crie o usuário e a senha.

Daqui em diante, salvo quando a seção disser o contrário, os comandos deste guia são digitados no terminal do Ubuntu, não no PowerShell: comandos como `sudo` e `apt` só existem no Ubuntu. O terminal do Ubuntu é reconhecido pelo começo da linha, no formato `usuario@maquina:~$`. Para entrar nele a partir do PowerShell ou do terminal do VS Code, use `wsl -d Ubuntu-24.04` (o nome da distribuição aparece em `wsl -l -v`); para sair, `exit`. A senha pedida pelo `sudo` é a do usuário do Ubuntu, e nada aparece na tela enquanto ela é digitada.

No terminal do Ubuntu, instale os pacotes básicos de compilação:

```
sudo apt update
sudo apt install -y build-essential git python3 python3-venv python3-pip curl xz-utils
```

Verificação:

```
lsb_release -ds
gcc --version
make --version
python3 --version
```

A primeira linha deve mostrar `Ubuntu 24.04`; as demais devem responder com uma versão.

## 3. Pasta do projeto

O projeto fica no GitHub, no repositório da organização do curso, criado pelo orientador: `https://github.com/cidigital-inatel/sd392-t25s2-group4`. É o único repositório do projeto desde 2026-11-16 (decisão D15 em `docs/decisoes.md`); o repositório pessoal `lucaslaress/riscv-rv32imac-ci-digital`, onde a especificação foi escrita, foi descontinuado, e todo o histórico dele está aqui. Para clonar, é preciso ser membro da equipe na organização; o convite chega por e-mail.

Esta seção é a exceção à regra da seção 2: os comandos abaixo são digitados no PowerShell, dentro da pasta do Windows em que o projeto vai ficar.

```
git clone https://github.com/cidigital-inatel/sd392-t25s2-group4.git
cd sd392-t25s2-group4
```

No primeiro acesso, o gerenciador de credenciais do Git for Windows abre o navegador para o login no GitHub. Se o git nunca foi usado nesta máquina, registre também quem assina os commits:

```
git config --global user.name "Seu Nome"
git config --global user.email "seu-email-do-github"
```

Verificação:

```
git status
git log --oneline -n 1
```

O primeiro comando deve responder `On branch main` e `nothing to commit, working tree clean`; o segundo mostra o commit mais recente.

Arquivos gerados e arquivos de configuração pessoal de editores e ferramentas ficam fora do repositório, pelo `.gitignore`.

Uma pasta do Windows como `C:\...\sd392-t25s2-group4` aparece no Ubuntu em `/mnt/c/.../sd392-t25s2-group4`. Compilações e simulações longas ficam mais rápidas se a pasta estiver dentro do sistema de arquivos do Ubuntu (por exemplo `~/sd392-t25s2-group4`). Nesse caso, o clone é feito no terminal do Ubuntu, o git de lá precisa de autenticação no GitHub configurada à parte, e a pasta é aberta no VS Code pela extensão WSL.

## 4. Toolchain RISC-V: xPack GNU RISC-V Embedded GCC 15.2.0-1

Quando instalar: no início (etapa M4.1).

O que é: o compilador C (gcc), o assembler, o linker, o objdump, o gdb e as bibliotecas newlib e libgcc, todos do projeto GNU, empacotados pelo projeto xPack para uso em sistemas embarcados sem sistema operacional. É um compilador cruzado: roda no PC e gera código para RISC-V.

Por que este pacote: ele traz bibliotecas prontas para a combinação `rv32imac` com ABI `ilp32`, que é o alvo do projeto. O pacote pré-compilado do repositório `riscv-collab/riscv-gnu-toolchain` não serve, porque só traz bibliotecas para `rv32imafdc/ilp32d` (ponto flutuante em hardware) e falha ao ligar programas para o nosso alvo. O prefixo dos executáveis é `riscv-none-elf-`; o plano de trabalho cita `riscv32-unknown-elf-`, que é o mesmo tipo de alvo com outro nome.

No terminal do Ubuntu:

```
mkdir -p ~/opt
cd ~/opt
curl -L -o xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v15.2.0-1/xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz
tar -xzf xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz -C ~/opt
rm xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz
```

O download tem 414 MB e a pasta `~/opt/xpack-riscv-none-elf-gcc-15.2.0-1` ocupa 1,6 GB.

PATH: para chamar os programas pelo nome, acrescente a linha abaixo ao `~/.bashrc`, **antes** do bloco que começa com `# If not running interactively, don't do anything`. A posição importa: esse bloco encerra a leitura do arquivo em shells não interativos (scripts e `bash -lc`), e a linha precisa vir antes dele para valer também nesses casos.

```
export PATH="$HOME/opt/xpack-riscv-none-elf-gcc-15.2.0-1/bin:$PATH"
```

Abra um terminal novo (ou rode `source ~/.bashrc`) e verifique:

```
which riscv-none-elf-gcc
riscv-none-elf-gcc --version
riscv-none-elf-gcc -march=rv32imac_zicsr -mabi=ilp32 -print-multi-directory
printf 'int main(void){return 0;}\n' > /tmp/t.c && riscv-none-elf-gcc -march=rv32imac_zicsr -mabi=ilp32 -o /tmp/t.elf /tmp/t.c && echo OK
```

Resultados esperados: o caminho dentro de `~/opt`; `gcc 15.2.0`; um ponto (`.`), que indica a biblioteca padrão do pacote, a de `rv32imac/ilp32`; e `OK`, mostrando que o programa foi ligado com a newlib sem erro.

Flags de compilação do projeto: `-march=rv32imac_zicsr -mabi=ilp32`. O sufixo `_zicsr` é obrigatório desde o binutils 2.38 para as instruções de acesso a CSR (`csrr` etc.), usadas para ler `mcycle` e `minstret`.

Observação para comandos disparados do Windows: use `wsl -d Ubuntu-24.04 -e bash -lc "comando"`. O nome depois de `-d` é o da distribuição, como aparece em `wsl -l -v`; em algumas máquinas ele é apenas `Ubuntu`. A forma `wsl -- comando` abre um shell que não lê o `~/.bashrc` e não enxerga o PATH.

## 5. Verilator e Icarus Verilog

Quando instalar: antes de começar o RTL (etapa M2.2).

O que são: dois simuladores de Verilog gratuitos. O Icarus Verilog interpreta o código e é simples de usar; o Verilator compila o Verilog para C++ e é muito mais rápido em simulações longas. Nenhum dos dois roda UVM; servem para testes de fumaça do RTL na própria máquina, sem depender do Xcelium remoto. O GTKWave abre as formas de onda geradas por eles.

```
sudo apt install -y verilator iverilog gtkwave
```

Verificação:

```
verilator --version
iverilog -V | head -n 1
```

Resultados esperados no Ubuntu 24.04: `Verilator 5.020` e `Icarus Verilog version 12.0`, ou versões próximas.

## 6. Spike

Quando instalar: até a etapa M3.2 (validação do modelo de referência); útil já na M4.1 para executar programas antes de existir RTL.

O que é: o simulador de referência oficial do conjunto de instruções RISC-V, mantido pela RISC-V International. Executa programas RISC-V no PC com precisão de instrução. No projeto ele é o "juiz externo": o modelo de referência próprio (ISS em C) é validado comparando seus resultados com os do Spike nos `riscv-tests`. Não há pacote pronto no Ubuntu; é compilado a partir do código-fonte, o que leva de 5 a 15 minutos.

Dependências:

```
sudo apt install -y autoconf automake autotools-dev libtool pkg-config device-tree-compiler libboost-dev libboost-regex-dev libboost-system-dev
```

Compilação e instalação em `~/opt/spike`:

```
mkdir -p ~/opt/src
cd ~/opt/src
git clone https://github.com/riscv-software-src/riscv-isa-sim.git
cd riscv-isa-sim
git checkout 609dbe0b9994154833039209fa37151e7c05e9d4
mkdir build
cd build
../configure --prefix=$HOME/opt/spike
make -j4
make install
```

O `git checkout` fixa o commit usado pela equipe, de 01/10/2026, para que as três máquinas compilem exatamente a mesma versão. Se a equipe mudar de commit, ele é atualizado aqui e na tabela da seção 9.

O `-j4` compila quatro arquivos em paralelo, e cada compilação usa algumas centenas de MB de memória. O WSL entrega ao Ubuntu, por padrão, metade da memória da máquina: com cerca de 4 GB no Ubuntu, quatro em paralelo cabem, e a compilação leva cerca de 10 minutos. Com mais memória, `make -j$(nproc)` é mais rápido. Se a compilação parar com a mensagem `Killed`, faltou memória: rode `make -j2`, que ela continua de onde parou. Para ver a memória disponível, use `free -m`.

A instalação ocupa 1,8 GB em `~/opt/spike`. O código-fonte com os arquivos de compilação ocupa 3,9 GB em `~/opt/src/riscv-isa-sim` e pode ser apagado depois do `make install`.

PATH: acrescente esta linha ao `~/.bashrc`, no mesmo lugar da linha do toolchain (antes do bloco de shell interativo):

```
export PATH="$HOME/opt/spike/bin:$PATH"
```

Verificação, em um terminal novo:

```
which spike
spike --help 2>&1 | head -n 1
```

Resultados esperados: o caminho dentro de `~/opt/spike/bin` e a linha `Spike RISC-V ISA Simulator 1.1.1-dev`.

Observação importante para o projeto: o Spike reserva o endereço `0x1000` para o seu boot ROM e usa `0x8000_0000` como início da memória padrão. Os programas do projeto são ligados em `0x0000_0000`, então, para rodar no Spike, precisam ser religados em `0x8000_0000` (a decisão de como fazer isso é um item da etapa M4.1 no `checklist.md`). A opção `--isa=rv32imac_zicsr` seleciona o conjunto de instruções do projeto.

## 7. Python para o modelo de IA

Quando instalar: na etapa M4.3 (treino e quantização da CNN).

O que é: um ambiente virtual do Python, isolado do sistema, com PyTorch (treino da rede), torchvision (download do MNIST), NumPy (referência bit-exata da inferência int8), Jupyter (notebook reprodutível) e matplotlib (gráficos). A versão de PyTorch instalada é a de CPU, que basta para uma rede pequena e evita baixar os pacotes de GPU, que passam de 2 GB.

```
python3 -m venv ~/opt/venv-ia
source ~/opt/venv-ia/bin/activate
pip install --upgrade pip
pip install torch torchvision --index-url https://download.pytorch.org/whl/cpu
pip install numpy jupyter matplotlib
```

O ambiente precisa ser ativado em cada terminal novo antes de usar o Python do projeto:

```
source ~/opt/venv-ia/bin/activate
```

Verificação, com o ambiente ativado:

```
python -c "import torch, torchvision, numpy; print(torch.__version__, torchvision.__version__, numpy.__version__)"
```

Deve imprimir as três versões. Anote-as na tabela de versões da seção 9. Na etapa M4.3, gere também o arquivo de versões exatas com `pip freeze > requirements.txt` dentro de `sw/mnist/`, para que outra máquina instale o mesmo conjunto com `pip install -r requirements.txt`.

Não é instalação: a suíte `riscv-tests` (testes de conformidade da ISA) é código-fonte copiado para dentro de `sw/` e adaptado ao núcleo na etapa M4.1; entra pelo fluxo de software, não por este guia.

## 8. Máquina remota com Xcelium

O Cadence Xcelium é o simulador comercial em que roda o ambiente UVM (etapa M3.1). Ele fica em uma máquina remota; nada dele é instalado nas máquinas de desenvolvimento. O plano exige confirmar o acesso nas semanas 1 e 2.

| Item | Valor |
|---|---|
| Host e forma de acesso | a definir |
| Usuário | a definir |
| Versão do Xcelium e da UVM | a definir |
| Como o projeto chega lá | a definir (sem git: cópia manual, `scp` ou `rsync`) |
| Teste de acesso | a definir: compilar e simular um exemplo mínimo em SystemVerilog com UVM usando `xrun` |

## 9. Versões em uso

| Ferramenta | Versão |
|---|---|
| Ubuntu | 24.04 |
| xPack GNU RISC-V Embedded GCC | 15.2.0-1 (gcc 15.2.0, binutils 2.45, gdb 16.3) |
| Verilator / Icarus Verilog / GTKWave | 5.020 / 12.0 / 3.3.116 (pacotes do Ubuntu 24.04) |
| Spike | 1.1.1-dev, commit `609dbe0b9994154833039209fa37151e7c05e9d4` (01/10/2026) |
| Python / PyTorch / torchvision / NumPy | a preencher |
| Xcelium / UVM | a preencher |
