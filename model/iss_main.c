/*
 * Executável autônomo do ISS: carrega um programa, executa até o tohost (ou
 * trap, ou limite de instruções) e, opcionalmente, grava o trace de retire.
 *
 * Código de saída do processo: o código do tohost (0 = sucesso), 2 em trap,
 * 3 se estourar o limite de instruções, 1 em erro de uso ou de carga.
 */
#include "iss.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void putc_stdout(char c, void *ctx)
{
    (void)ctx;
    putchar(c);
    fflush(stdout);
}

static void uso(const char *prog)
{
    fprintf(stderr,
            "Uso: %s [opcoes] programa.elf\n"
            "     %s [opcoes] --imem imem.hex [--dmem dmem.hex]\n"
            "\n"
            "Opcoes:\n"
            "  --trace ARQ      grava o trace de retire (formato --log-commits do Spike); '-' = stderr\n"
            "  --max-insns N    limite de instrucoes (padrao 10000000)\n"
            "  --reset-pc END   vetor de reset (padrao 0x00000000)\n"
            "  --stats          imprime instrucoes executadas ao final\n",
            prog, prog);
}

int main(int argc, char **argv)
{
    iss_t s;
    iss_retire_t r;
    const char *elf = NULL, *imem_hex = NULL, *dmem_hex = NULL, *trace_arq = NULL;
    unsigned long max_insns = 10000000ul, k;
    uint32_t pc_reset = ISS_IMEM_BASE;
    int stats = 0, i, ret;
    FILE *trace = NULL;
    char linha[160];

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_arq = argv[++i];
        else if (!strcmp(argv[i], "--imem") && i + 1 < argc) imem_hex = argv[++i];
        else if (!strcmp(argv[i], "--dmem") && i + 1 < argc) dmem_hex = argv[++i];
        else if (!strcmp(argv[i], "--max-insns") && i + 1 < argc) max_insns = strtoul(argv[++i], NULL, 0);
        else if (!strcmp(argv[i], "--reset-pc") && i + 1 < argc) pc_reset = (uint32_t)strtoul(argv[++i], NULL, 0);
        else if (!strcmp(argv[i], "--stats")) stats = 1;
        else if (argv[i][0] != '-' && !elf) elf = argv[i];
        else { uso(argv[0]); return 1; }
    }
    if (!elf == !imem_hex) {
        uso(argv[0]);
        return 1;
    }

    if (iss_init(&s, ISS_IMEM_SIZE_PADRAO, ISS_DMEM_SIZE_PADRAO) != 0) {
        fprintf(stderr, "iss: sem memoria\n");
        return 1;
    }
    s.console_putc = putc_stdout;

    if (elf) ret = iss_load_elf(&s, elf);
    else {
        ret = iss_load_hex(&s, imem_hex, ISS_IMEM_BASE);
        if (ret == 0 && dmem_hex) ret = iss_load_hex(&s, dmem_hex, ISS_DMEM_BASE);
    }
    if (ret != 0) {
        iss_free(&s);
        return 1;
    }

    if (trace_arq) {
        trace = strcmp(trace_arq, "-") ? fopen(trace_arq, "w") : stderr;
        if (!trace) {
            perror(trace_arq);
            iss_free(&s);
            return 1;
        }
    }

    iss_reset(&s, pc_reset);
    for (k = 0; k < max_insns && s.estado == ISS_EXECUTANDO; k++) {
        iss_step(&s, &r);
        if (trace) {
            iss_formata_retire(&r, linha, sizeof(linha));
            fprintf(trace, "%s\n", linha);
        }
        if (r.mem_fora)
            fprintf(stderr, "iss: aviso: acesso fora do mapa em 0x%08x (pc 0x%08x)\n", r.mem_addr, r.pc);
    }
    if (trace && trace != stderr)
        fclose(trace);

    if (stats)
        fprintf(stderr, "iss: %lu instrucoes executadas\n", (unsigned long)s.minstret);

    switch (s.estado) {
    case ISS_FIM_TOHOST:
        if (s.codigo_saida == 0)
            fprintf(stderr, "iss: PASSOU (tohost = 1)\n");
        else
            fprintf(stderr, "iss: FALHOU com codigo %u\n", s.codigo_saida);
        ret = s.codigo_saida ? (int)(s.codigo_saida & 0xff ? s.codigo_saida & 0xff : 1) : 0;
        break;
    case ISS_FIM_TRAP:
        fprintf(stderr, "iss: trap '%s' em pc 0x%08x (insn 0x%08x)\n",
                iss_nome_trap(s.trap), r.pc, r.insn);
        ret = 2;
        break;
    default:
        fprintf(stderr, "iss: limite de %lu instrucoes atingido em pc 0x%08x\n", max_insns, s.pc);
        ret = 3;
    }
    iss_free(&s);
    return ret;
}
