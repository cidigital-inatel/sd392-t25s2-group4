# Convenções de trabalho no repositório

Como a equipe usa branches, commits, revisão, issues e tags. Depois de aprovadas, valem para os três; mudanças nelas são registradas em `docs/decisoes.md`.

Última atualização: 2026-10-04. Situação: proposta, a aprovar na primeira reunião.

## 1. Branches

- A `main` está sempre em estado utilizável: o que está nela compila e os testes que já existem passam.
- Cada tarefa é feita em uma branch própria, criada a partir da `main`, com o nome `frente/descricao-curta`. A frente é a pasta principal da tarefa: `rtl`, `tb`, `model`, `sw` ou `docs`. Exemplos: `rtl/estagio-if`, `sw/crt0`, `docs/mapa-de-memoria`.
- Branches são curtas: um item do checklist, ou parte dele. Depois de incorporada à `main`, a branch é apagada.

## 2. Commits

- Mensagem em português, começando pela frente e dizendo o que mudou. Exemplos: `rtl: adiciona stall de load-use`, `sw: corrige ordem dos bytes na conversão para hex`.
- Um commit trata de um assunto só.
- Arquivos gerados (compilação, simulação, formas de onda) não são versionados. O `.gitignore` cobre os casos conhecidos; ao surgir um novo tipo de arquivo gerado, ele é acrescentado lá.

## 3. Revisão

- Nada entra na `main` por commit direto. Toda branch é incorporada por pull request, com pelo menos uma aprovação.
- O revisor segue a revisão cruzada definida em `docs/checklist.md`:

| Autor | Revisor |
|---|---|
| Lucas Lares (núcleo RTL) | Pedro Piccichelli |
| Pedro Piccichelli (verificação e ISS) | Lucas Teixeira |
| Lucas Teixeira (plataforma, software e IA) | Lucas Lares |

- Se o revisor estiver indisponível, o outro integrante pode aprovar.
- A especificação (`docs/especificacao.md`) é a exceção: precisa da aprovação dos três.
- As alterações que uma mudança na especificação exige no RTL, no ISS, no plano de verificação e na biblioteca de software entram juntas, na mesma branch, para a `main` nunca ficar com essas partes em desacordo.

## 4. Issues

- Todo defeito, toda mudança de escopo e toda tarefa que não está no checklist viram issue.
- Etiquetas: uma por frente (`rtl`, `tb`, `model`, `sw`, `docs`) e uma por tipo (`defeito`, `mudanca-de-escopo`, `tarefa`).
- Mudança de escopo (plano de trabalho, seção 7.1): a issue traz descrição, motivação, impacto em prazos e entregáveis, alternativas consideradas e decisão. A mesma mudança ganha uma linha na seção 3 de `docs/decisoes.md`, com o número da issue.
- Defeito: a issue traz descrição, causa, correção e o teste que o detectou. São os dados que as metas M2.5 e M3.4 pedem no registro de defeitos.
- O pull request que resolve uma issue cita o número dela na descrição (`Closes #12`), para o GitHub fechá-la no merge.

## 5. Tags

| Tag | Quando é criada | Meta |
|---|---|---|
| `spec-vX.Y` | A cada versão aprovada da especificação; a primeira é `spec-v1.0` | M2.1 |
| `rtl-vX.Y` | Na versão congelada do RTL usada na demonstração | M2.5 |
| `v1.0` | Na entrega final do projeto | M5.5 |

As tags são criadas na `main`, depois do merge, e não são movidas.
