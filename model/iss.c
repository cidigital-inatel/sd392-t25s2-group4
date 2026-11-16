/*
 * Núcleo do ISS: memória, decodificação e execução de RV32IMAC + custom-0.
 *
 * Pontos em que a especificação ainda está em aberto e o ISS adota uma escolha
 * provisória (marcados com "PROVISÓRIO" no código):
 *   - traps (seção 9): ecall, ebreak, instrução ilegal e acesso desalinhado
 *     param a simulação com ISS_FIM_TRAP em vez de desviar para mtvec;
 *   - acesso fora do mapa (seção 2): leitura devolve 0, escrita é ignorada e o
 *     retire marca mem_fora;
 *   - sat com n = 0 e relu com rs2 != x0 (seção 4).
 */
#include "iss.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Utilidades de bits                                                 */
/* ------------------------------------------------------------------ */

static int32_t sext(uint32_t v, int bits)
{
    return (int32_t)(v << (32 - bits)) >> (32 - bits);
}

static uint32_t imm_i(uint32_t i) { return (uint32_t)((int32_t)i >> 20); }
static uint32_t imm_s(uint32_t i)
{
    return (uint32_t)(((int32_t)(i & 0xfe000000u) >> 20) | ((i >> 7) & 0x1f));
}
static uint32_t imm_b(uint32_t i)
{
    return (uint32_t)(((int32_t)(i & 0x80000000u) >> 19) | ((i & 0x80) << 4) |
                      ((i >> 20) & 0x7e0) | ((i >> 7) & 0x1e));
}
static uint32_t imm_u(uint32_t i) { return i & 0xfffff000u; }
static uint32_t imm_j(uint32_t i)
{
    return (uint32_t)(((int32_t)(i & 0x80000000u) >> 11) | (i & 0xff000) |
                      ((i >> 9) & 0x800) | ((i >> 20) & 0x7fe));
}

/* ------------------------------------------------------------------ */
/* Inicialização e carga                                              */
/* ------------------------------------------------------------------ */

int iss_init(iss_t *s, uint32_t imem_size, uint32_t dmem_size)
{
    memset(s, 0, sizeof(*s));
    s->imem_size = imem_size;
    s->dmem_size = dmem_size;
    s->imem = (uint8_t *)calloc(imem_size, 1);
    s->dmem = (uint8_t *)calloc(dmem_size, 1);
    if (!s->imem || !s->dmem) {
        iss_free(s);
        return -1;
    }
    iss_reset(s, ISS_IMEM_BASE);
    return 0;
}

void iss_free(iss_t *s)
{
    free(s->imem);
    free(s->dmem);
    s->imem = s->dmem = NULL;
}

/* Reset: zera registradores, contadores, ACC e tohost; memórias ficam. */
void iss_reset(iss_t *s, uint32_t pc_reset)
{
    memset(s->x, 0, sizeof(s->x));
    s->pc = pc_reset;
    s->macacc = 0;
    s->mcycle = 0;
    s->minstret = 0;
    s->tohost = 0;
    s->reserva_valida = 0;
    s->estado = ISS_EXECUTANDO;
    s->codigo_saida = 0;
    s->trap = ISS_TRAP_NENHUM;
}

/* Ponteiro para o byte em IMEM ou DMEM, ou NULL se fora delas. */
static uint8_t *ptr_mem(iss_t *s, uint32_t addr)
{
    if (addr - ISS_IMEM_BASE < s->imem_size)
        return &s->imem[addr - ISS_IMEM_BASE];
    if (addr - ISS_DMEM_BASE < s->dmem_size)
        return &s->dmem[addr - ISS_DMEM_BASE];
    return NULL;
}

int iss_write_bytes(iss_t *s, uint32_t addr, const uint8_t *dados, uint32_t n)
{
    uint32_t k;
    for (k = 0; k < n; k++) {
        uint8_t *p = ptr_mem(s, addr + k);
        if (!p) {
            fprintf(stderr, "iss: carga fora da IMEM/DMEM em 0x%08x\n", addr + k);
            return -1;
        }
        *p = dados ? dados[k] : 0;
    }
    return 0;
}

static uint32_t le32(const uint8_t *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

int iss_load_elf(iss_t *s, const char *caminho)
{
    FILE *f = fopen(caminho, "rb");
    uint8_t *buf;
    long tam;
    uint32_t phoff, k;
    uint16_t phentsize, phnum;
    int ret = 0;

    if (!f) {
        perror(caminho);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    tam = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (uint8_t *)malloc((size_t)tam);
    if (!buf || fread(buf, 1, (size_t)tam, f) != (size_t)tam) {
        fprintf(stderr, "iss: erro lendo %s\n", caminho);
        fclose(f);
        free(buf);
        return -1;
    }
    fclose(f);

    /* ELF32, little endian, EM_RISCV (243). */
    if (tam < 52 || memcmp(buf, "\x7f" "ELF", 4) != 0 || buf[4] != 1 || buf[5] != 1 ||
        le16(buf + 18) != 243) {
        fprintf(stderr, "iss: %s não é um ELF32 RISC-V little endian\n", caminho);
        free(buf);
        return -1;
    }
    phoff = le32(buf + 28);
    phentsize = le16(buf + 42);
    phnum = le16(buf + 44);

    for (k = 0; k < phnum && ret == 0; k++) {
        const uint8_t *ph = buf + phoff + k * phentsize;
        uint32_t tipo = le32(ph), off = le32(ph + 4), paddr = le32(ph + 12);
        uint32_t filesz = le32(ph + 16), memsz = le32(ph + 20);
        if (tipo != 1 /* PT_LOAD */ || memsz == 0)
            continue;
        if ((uint64_t)off + filesz > (uint64_t)tam) {
            fprintf(stderr, "iss: segmento %u truncado em %s\n", k, caminho);
            ret = -1;
            break;
        }
        ret = iss_write_bytes(s, paddr, buf + off, filesz);
        if (ret == 0 && memsz > filesz)
            ret = iss_write_bytes(s, paddr + filesz, NULL, memsz - filesz);
    }
    free(buf);
    return ret;
}

int iss_load_hex(iss_t *s, const char *caminho, uint32_t base)
{
    FILE *f = fopen(caminho, "r");
    char tok[64];
    uint32_t idx = 0;

    if (!f) {
        perror(caminho);
        return -1;
    }
    while (fscanf(f, "%63s", tok) == 1) {
        uint8_t b[4];
        uint32_t w;
        if (tok[0] == '/' && tok[1] == '/') { /* comentário até o fim da linha */
            int c;
            while ((c = fgetc(f)) != EOF && c != '\n')
                ;
            continue;
        }
        if (tok[0] == '@') {
            idx = (uint32_t)strtoul(tok + 1, NULL, 16);
            continue;
        }
        w = (uint32_t)strtoul(tok, NULL, 16);
        b[0] = w & 0xff; b[1] = (w >> 8) & 0xff; b[2] = (w >> 16) & 0xff; b[3] = w >> 24;
        if (iss_write_bytes(s, base + 4 * idx, b, 4) != 0) {
            fclose(f);
            return -1;
        }
        idx++;
    }
    fclose(f);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Porta de dados: DMEM e periféricos                                 */
/* ------------------------------------------------------------------ */

static int na_dmem(iss_t *s, uint32_t addr, uint32_t n)
{
    return addr - ISS_DMEM_BASE < s->dmem_size && addr - ISS_DMEM_BASE + n <= s->dmem_size;
}
static int no_periph(uint32_t addr) { return addr - ISS_PERIPH_BASE < 16; }

static uint32_t le_dado(iss_t *s, uint32_t addr, uint32_t n, iss_retire_t *r)
{
    uint32_t v = 0, k;
    if (na_dmem(s, addr, n)) {
        for (k = 0; k < n; k++)
            v |= (uint32_t)s->dmem[addr - ISS_DMEM_BASE + k] << (8 * k);
    } else if (no_periph(addr)) {
        uint32_t palavra = (addr & ~3u) == ISS_ADDR_TOHOST ? s->tohost : 0;
        v = palavra >> (8 * (addr & 3));
        if (n < 4)
            v &= (1u << (8 * n)) - 1;
    } else {
        r->mem_fora = 1; /* PROVISÓRIO: lê 0 */
    }
    return v;
}

static void escreve_dado(iss_t *s, uint32_t addr, uint32_t n, uint32_t v, iss_retire_t *r)
{
    uint32_t k;
    if (na_dmem(s, addr, n)) {
        for (k = 0; k < n; k++)
            s->dmem[addr - ISS_DMEM_BASE + k] = (uint8_t)(v >> (8 * k));
    } else if (no_periph(addr)) {
        uint32_t palavra = addr & ~3u, desl = 8 * (addr & 3);
        int habilita_byte0 = (addr & 3) == 0;
        if (palavra == ISS_ADDR_CONSOLE) {
            if (habilita_byte0 && s->console_putc)
                s->console_putc((char)(v & 0xff), s->console_ctx);
        } else if (palavra == ISS_ADDR_TOHOST) {
            uint32_t mascara = (n == 4 ? 0xffffffffu : (1u << (8 * n)) - 1) << desl;
            s->tohost = (s->tohost & ~mascara) | ((v << desl) & mascara);
            if (habilita_byte0 && (s->tohost & 1)) {
                s->estado = ISS_FIM_TOHOST;
                s->codigo_saida = s->tohost >> 1;
            }
        }
        /* demais registradores reservados: escrita ignorada */
    } else {
        r->mem_fora = 1; /* PROVISÓRIO: escrita ignorada */
    }
}

/* ------------------------------------------------------------------ */
/* CSRs                                                               */
/* ------------------------------------------------------------------ */

/* Devolve 0 se o CSR existe; 'escrevivel' diz se aceita escrita. */
static int csr_le(iss_t *s, uint32_t csr, uint32_t *v)
{
    switch (csr) {
    case ISS_CSR_MACACC:                        *v = s->macacc; return 0;
    case ISS_CSR_MCYCLE:   case ISS_CSR_CYCLE:    *v = (uint32_t)s->mcycle; return 0;
    case ISS_CSR_MCYCLEH:  case ISS_CSR_CYCLEH:   *v = (uint32_t)(s->mcycle >> 32); return 0;
    case ISS_CSR_MINSTRET: case ISS_CSR_INSTRET:  *v = (uint32_t)s->minstret; return 0;
    case ISS_CSR_MINSTRETH: case ISS_CSR_INSTRETH: *v = (uint32_t)(s->minstret >> 32); return 0;
    default: return -1;
    }
}

static void csr_escreve(iss_t *s, uint32_t csr, uint32_t v, int *escreveu_ciclo, int *escreveu_instret)
{
    switch (csr) {
    case ISS_CSR_MACACC: s->macacc = v; break;
    case ISS_CSR_MCYCLE:
        s->mcycle = (s->mcycle & 0xffffffff00000000ull) | v; *escreveu_ciclo = 1; break;
    case ISS_CSR_MCYCLEH:
        s->mcycle = (s->mcycle & 0xffffffffull) | ((uint64_t)v << 32); *escreveu_ciclo = 1; break;
    case ISS_CSR_MINSTRET:
        s->minstret = (s->minstret & 0xffffffff00000000ull) | v; *escreveu_instret = 1; break;
    case ISS_CSR_MINSTRETH:
        s->minstret = (s->minstret & 0xffffffffull) | ((uint64_t)v << 32); *escreveu_instret = 1; break;
    }
}

/* ------------------------------------------------------------------ */
/* Extensão C: expansão para a instrução de 32 bits equivalente       */
/* ------------------------------------------------------------------ */

static uint32_t enc_r(uint32_t f7, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t rd, uint32_t op)
{
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op;
}
static uint32_t enc_i(int32_t imm, uint32_t rs1, uint32_t f3, uint32_t rd, uint32_t op)
{
    return ((uint32_t)imm << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op;
}
static uint32_t enc_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t op)
{
    uint32_t u = (uint32_t)imm;
    return ((u >> 5 & 0x7f) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | ((u & 0x1f) << 7) | op;
}
static uint32_t enc_b(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t op)
{
    uint32_t u = (uint32_t)imm;
    return ((u >> 12 & 1) << 31) | ((u >> 5 & 0x3f) << 25) | (rs2 << 20) | (rs1 << 15) |
           (f3 << 12) | ((u >> 1 & 0xf) << 8) | ((u >> 11 & 1) << 7) | op;
}
static uint32_t enc_j(int32_t imm, uint32_t rd)
{
    uint32_t u = (uint32_t)imm;
    return ((u >> 20 & 1) << 31) | ((u >> 1 & 0x3ff) << 21) | ((u >> 11 & 1) << 20) |
           ((u >> 12 & 0xff) << 12) | (rd << 7) | 0x6f;
}

uint32_t iss_expande_c(uint16_t h)
{
    uint32_t f3 = h >> 13;
    uint32_t rd = (h >> 7) & 0x1f, rs2 = (h >> 2) & 0x1f;
    uint32_t rdp = 8 + ((h >> 2) & 7), rs1p = 8 + ((h >> 7) & 7);
    int32_t imm6 = sext(((h >> 7) & 0x20) | ((h >> 2) & 0x1f), 6);
    uint32_t shamt = ((h >> 7) & 0x20) | ((h >> 2) & 0x1f);
    int32_t imm;

    switch (h & 3) {
    case 0:
        switch (f3) {
        case 0: /* c.addi4spn */
            imm = ((h >> 7) & 0x30) | ((h >> 1) & 0x3c0) | ((h >> 4) & 4) | ((h >> 2) & 8);
            return imm ? enc_i(imm, 2, 0, rdp, 0x13) : 0;
        case 2: /* c.lw */
            imm = ((h >> 7) & 0x38) | ((h >> 4) & 4) | ((h << 1) & 0x40);
            return enc_i(imm, rs1p, 2, rdp, 0x03);
        case 6: /* c.sw */
            imm = ((h >> 7) & 0x38) | ((h >> 4) & 4) | ((h << 1) & 0x40);
            return enc_s(imm, rdp, rs1p, 2, 0x23);
        }
        return 0;
    case 1:
        switch (f3) {
        case 0: /* c.addi / c.nop */
            return enc_i(imm6, rd, 0, rd, 0x13);
        case 1: /* c.jal */
        case 5: /* c.j */
            imm = sext(((h >> 1) & 0x800) | ((h >> 7) & 0x10) | ((h >> 1) & 0x300) |
                       ((h << 2) & 0x400) | ((h >> 1) & 0x40) | ((h << 1) & 0x80) |
                       ((h >> 2) & 0xe) | ((h << 3) & 0x20), 12);
            return enc_j(imm, f3 == 1 ? 1 : 0);
        case 2: /* c.li */
            return enc_i(imm6, 0, 0, rd, 0x13);
        case 3:
            if (rd == 2) { /* c.addi16sp */
                imm = sext(((h >> 3) & 0x200) | ((h >> 2) & 0x10) | ((h << 1) & 0x40) |
                           ((h << 4) & 0x180) | ((h << 3) & 0x20), 10);
                return imm ? enc_i(imm, 2, 0, 2, 0x13) : 0;
            }
            /* c.lui */
            return imm6 ? (((uint32_t)imm6 << 12) | (rd << 7) | 0x37) : 0;
        case 4:
            switch ((h >> 10) & 3) {
            case 0: /* c.srli */
                return (shamt & 0x20) ? 0 : enc_r(0x00, shamt, rs1p, 5, rs1p, 0x13);
            case 1: /* c.srai */
                return (shamt & 0x20) ? 0 : enc_r(0x20, shamt, rs1p, 5, rs1p, 0x13);
            case 2: /* c.andi */
                return enc_i(imm6, rs1p, 7, rs1p, 0x13);
            default:
                if (h & 0x1000)
                    return 0;
                switch ((h >> 5) & 3) {
                case 0: return enc_r(0x20, rdp, rs1p, 0, rs1p, 0x33); /* c.sub */
                case 1: return enc_r(0x00, rdp, rs1p, 4, rs1p, 0x33); /* c.xor */
                case 2: return enc_r(0x00, rdp, rs1p, 6, rs1p, 0x33); /* c.or */
                default: return enc_r(0x00, rdp, rs1p, 7, rs1p, 0x33); /* c.and */
                }
            }
        case 6: /* c.beqz */
        case 7: /* c.bnez */
            imm = sext(((h >> 4) & 0x100) | ((h >> 7) & 0x18) | ((h << 1) & 0xc0) |
                       ((h >> 2) & 6) | ((h << 3) & 0x20), 9);
            return enc_b(imm, 0, rs1p, f3 == 6 ? 0 : 1, 0x63);
        }
        return 0;
    case 2:
        switch (f3) {
        case 0: /* c.slli */
            return (shamt & 0x20) ? 0 : enc_r(0x00, shamt, rd, 1, rd, 0x13);
        case 2: /* c.lwsp */
            imm = ((h >> 7) & 0x20) | ((h >> 2) & 0x1c) | ((h << 4) & 0xc0);
            return rd ? enc_i(imm, 2, 2, rd, 0x03) : 0;
        case 4:
            if (!(h & 0x1000)) {
                if (rs2 == 0) /* c.jr */
                    return rd ? enc_i(0, rd, 0, 0, 0x67) : 0;
                return enc_r(0, rs2, 0, 0, rd, 0x33); /* c.mv */
            }
            if (rs2 == 0) {
                if (rd == 0) /* c.ebreak */
                    return 0x00100073;
                return enc_i(0, rd, 0, 1, 0x67); /* c.jalr */
            }
            return enc_r(0, rs2, rd, 0, rd, 0x33); /* c.add */
        case 6: /* c.swsp */
            imm = ((h >> 7) & 0x3c) | ((h >> 1) & 0xc0);
            return enc_s(imm, rs2, 2, 2, 0x23);
        }
        return 0;
    }
    return 0; /* h & 3 == 3 não é comprimida */
}

/* ------------------------------------------------------------------ */
/* Execução                                                           */
/* ------------------------------------------------------------------ */

static uint32_t mulh_ss(int32_t a, int32_t b) { return (uint32_t)(((int64_t)a * b) >> 32); }
static uint32_t mulh_su(int32_t a, uint32_t b) { return (uint32_t)(((int64_t)a * (int64_t)b) >> 32); }
static uint32_t mulh_uu(uint32_t a, uint32_t b) { return (uint32_t)(((uint64_t)a * b) >> 32); }

/* sat: clamp com sinal para n bits, n = rs2[4:0] (proposta do plano). */
static uint32_t custom_sat(uint32_t v, uint32_t rs2)
{
    uint32_t n = rs2 & 0x1f;
    int32_t x = (int32_t)v, max, min;
    if (n == 0) /* PROVISÓRIO (seção 4.3): n = 0 não satura */
        return v;
    max = (int32_t)((1u << (n - 1)) - 1);
    min = -max - 1;
    return (uint32_t)(x > max ? max : (x < min ? min : x));
}

static void para_trap(iss_t *s, iss_retire_t *r, iss_trap_t t)
{
    s->estado = ISS_FIM_TRAP; /* PROVISÓRIO (seção 9): trap encerra a simulação */
    s->trap = t;
    r->trap = t;
}

iss_estado_t iss_step(iss_t *s, iss_retire_t *saida)
{
    iss_retire_t tmp, *r = saida ? saida : &tmp;
    uint32_t pc = s->pc, insn, op, rd, rs1, rs2, f3, f7, a, b, v = 0, pc_prox;
    uint16_t lo;
    int escreve_rd = 0, escreveu_ciclo = 0, escreveu_instret = 0;

    memset(r, 0, sizeof(*r));
    r->pc = pc;
    if (s->estado != ISS_EXECUTANDO)
        return s->estado;

    /* Busca: só na IMEM, alinhada a 16 bits. */
    if ((pc & 1) || pc - ISS_IMEM_BASE + 2 > s->imem_size) {
        para_trap(s, r, ISS_TRAP_BUSCA_FORA);
        return s->estado;
    }
    lo = le16(&s->imem[pc - ISS_IMEM_BASE]);
    if ((lo & 3) == 3) {
        if (pc - ISS_IMEM_BASE + 4 > s->imem_size) {
            para_trap(s, r, ISS_TRAP_BUSCA_FORA);
            return s->estado;
        }
        insn = lo | ((uint32_t)le16(&s->imem[pc - ISS_IMEM_BASE + 2]) << 16);
        r->insn = insn;
        r->insn_len = 4;
    } else {
        r->insn = lo;
        r->insn_len = 2;
        insn = iss_expande_c(lo);
        if (insn == 0) {
            para_trap(s, r, ISS_TRAP_ILEGAL);
            return s->estado;
        }
    }
    pc_prox = pc + r->insn_len;

    op = insn & 0x7f;
    rd = (insn >> 7) & 0x1f;
    f3 = (insn >> 12) & 7;
    rs1 = (insn >> 15) & 0x1f;
    rs2 = (insn >> 20) & 0x1f;
    f7 = insn >> 25;
    a = s->x[rs1];
    b = s->x[rs2];

    switch (op) {
    case 0x37: /* lui */
        v = imm_u(insn); escreve_rd = 1;
        break;
    case 0x17: /* auipc */
        v = pc + imm_u(insn); escreve_rd = 1;
        break;
    case 0x6f: /* jal */
        v = pc_prox; escreve_rd = 1;
        pc_prox = pc + imm_j(insn);
        break;
    case 0x67: /* jalr */
        if (f3 != 0)
            goto ilegal;
        v = pc_prox; escreve_rd = 1;
        pc_prox = (a + imm_i(insn)) & ~1u;
        break;
    case 0x63: { /* desvios */
        int tomado;
        switch (f3) {
        case 0: tomado = a == b; break;
        case 1: tomado = a != b; break;
        case 4: tomado = (int32_t)a < (int32_t)b; break;
        case 5: tomado = (int32_t)a >= (int32_t)b; break;
        case 6: tomado = a < b; break;
        case 7: tomado = a >= b; break;
        default: goto ilegal;
        }
        if (tomado)
            pc_prox = pc + imm_b(insn);
        break;
    }
    case 0x03: { /* loads */
        uint32_t addr = a + imm_i(insn), n;
        switch (f3) {
        case 0: case 4: n = 1; break;
        case 1: case 5: n = 2; break;
        case 2: n = 4; break;
        default: goto ilegal;
        }
        r->mem_valid = 1; r->mem_addr = addr; r->mem_size = (uint8_t)n;
        if (addr & (n - 1)) {
            para_trap(s, r, ISS_TRAP_DESALINHADO);
            return s->estado;
        }
        v = le_dado(s, addr, n, r);
        r->mem_rdata = v;
        if (f3 == 0) v = (uint32_t)sext(v, 8);
        if (f3 == 1) v = (uint32_t)sext(v, 16);
        escreve_rd = 1;
        break;
    }
    case 0x23: { /* stores */
        uint32_t addr = a + imm_s(insn), n;
        if (f3 > 2)
            goto ilegal;
        n = 1u << f3;
        r->mem_valid = 1; r->mem_write = 1; r->mem_addr = addr; r->mem_size = (uint8_t)n;
        r->mem_wdata = n == 4 ? b : b & ((1u << (8 * n)) - 1);
        if (addr & (n - 1)) {
            para_trap(s, r, ISS_TRAP_DESALINHADO);
            return s->estado;
        }
        escreve_dado(s, addr, n, b, r);
        break;
    }
    case 0x13: { /* op-imm */
        uint32_t imm = imm_i(insn), sh = rs2;
        switch (f3) {
        case 0: v = a + imm; break;
        case 2: v = (int32_t)a < (int32_t)imm; break;
        case 3: v = a < imm; break;
        case 4: v = a ^ imm; break;
        case 6: v = a | imm; break;
        case 7: v = a & imm; break;
        case 1:
            if (f7 != 0) goto ilegal;
            v = a << sh; break;
        case 5:
            if (f7 == 0x00) v = a >> sh;
            else if (f7 == 0x20) v = (uint32_t)((int32_t)a >> sh);
            else goto ilegal;
            break;
        }
        escreve_rd = 1;
        break;
    }
    case 0x33: /* op */
        if (f7 == 0x01) { /* extensão M */
            int32_t sa = (int32_t)a, sb = (int32_t)b;
            switch (f3) {
            case 0: v = a * b; break;
            case 1: v = mulh_ss(sa, sb); break;
            case 2: v = mulh_su(sa, b); break;
            case 3: v = mulh_uu(a, b); break;
            case 4: /* div */
                if (b == 0) v = 0xffffffffu;
                else if (a == 0x80000000u && sb == -1) v = a;
                else v = (uint32_t)(sa / sb);
                break;
            case 5: v = b == 0 ? 0xffffffffu : a / b; break; /* divu */
            case 6: /* rem */
                if (b == 0) v = a;
                else if (a == 0x80000000u && sb == -1) v = 0;
                else v = (uint32_t)(sa % sb);
                break;
            case 7: v = b == 0 ? a : a % b; break; /* remu */
            }
        } else if (f7 == 0x00 || (f7 == 0x20 && (f3 == 0 || f3 == 5))) {
            switch (f3) {
            case 0: v = f7 ? a - b : a + b; break;
            case 1: v = a << (b & 31); break;
            case 2: v = (int32_t)a < (int32_t)b; break;
            case 3: v = a < b; break;
            case 4: v = a ^ b; break;
            case 5: v = f7 ? (uint32_t)((int32_t)a >> (b & 31)) : a >> (b & 31); break;
            case 6: v = a | b; break;
            case 7: v = a & b; break;
            }
        } else {
            goto ilegal;
        }
        escreve_rd = 1;
        break;
    case 0x0f: /* fence: sem efeito num núcleo em ordem sem cache (fence.i fora do escopo) */
        if (f3 != 0)
            goto ilegal;
        break;
    case 0x73: { /* system */
        uint32_t csr = insn >> 20, antigo, novo, fonte;
        if (f3 == 0) {
            if (insn == 0x00000073) { para_trap(s, r, ISS_TRAP_ECALL); return s->estado; }
            if (insn == 0x00100073) { para_trap(s, r, ISS_TRAP_EBREAK); return s->estado; }
            goto ilegal;
        }
        if (f3 == 4 || csr_le(s, csr, &antigo) != 0)
            goto ilegal;
        fonte = (f3 & 4) ? rs1 : a; /* csrr?i usa o campo rs1 como imediato */
        switch (f3 & 3) {
        case 1: novo = fonte; break;
        case 2: novo = antigo | fonte; break;
        default: novo = antigo & ~fonte; break;
        }
        /* csrrs/csrrc com rs1 = x0 (ou imediato 0) não escrevem */
        if ((f3 & 3) == 1 || rs1 != 0) {
            if ((csr >> 10) == 3) /* CSR somente leitura */
                goto ilegal;
            csr_escreve(s, csr, novo, &escreveu_ciclo, &escreveu_instret);
        }
        v = antigo; escreve_rd = 1;
        break;
    }
    case 0x2f: { /* extensão A */
        uint32_t f5 = insn >> 27, addr = a, m;
        if (f3 != 2)
            goto ilegal;
        r->mem_valid = 1; r->mem_addr = addr; r->mem_size = 4;
        if (addr & 3) {
            para_trap(s, r, ISS_TRAP_DESALINHADO);
            return s->estado;
        }
        if (f5 == 0x02) { /* lr.w */
            if (rs2 != 0) goto ilegal;
            v = le_dado(s, addr, 4, r);
            r->mem_rdata = v;
            s->reserva_valida = 1;
            s->reserva_addr = addr;
        } else if (f5 == 0x03) { /* sc.w */
            if (s->reserva_valida && s->reserva_addr == addr) {
                r->mem_write = 1; r->mem_wdata = b;
                escreve_dado(s, addr, 4, b, r);
                v = 0;
            } else {
                r->mem_valid = 0;
                v = 1;
            }
            s->reserva_valida = 0;
        } else {
            m = le_dado(s, addr, 4, r);
            r->mem_rdata = m;
            switch (f5) {
            case 0x01: v = b; break;                                            /* amoswap */
            case 0x00: v = m + b; break;                                        /* amoadd */
            case 0x04: v = m ^ b; break;                                        /* amoxor */
            case 0x0c: v = m & b; break;                                        /* amoand */
            case 0x08: v = m | b; break;                                        /* amoor */
            case 0x10: v = (int32_t)m < (int32_t)b ? m : b; break;              /* amomin */
            case 0x14: v = (int32_t)m > (int32_t)b ? m : b; break;              /* amomax */
            case 0x18: v = m < b ? m : b; break;                                /* amominu */
            case 0x1c: v = m > b ? m : b; break;                                /* amomaxu */
            default: goto ilegal;
            }
            r->mem_write = 1; r->mem_wdata = v;
            escreve_dado(s, addr, 4, v, r);
            v = m;
        }
        escreve_rd = 1;
        break;
    }
    case 0x0b: /* custom-0: especificação, seção 4 */
        if (f7 != 0)
            goto ilegal;
        switch (f3) {
        case 0: /* mac: ACC <- ACC + rs1*rs2; rd <- ACC (atualiza ACC mesmo com rd = x0) */
            s->macacc = s->macacc + a * b;
            v = s->macacc;
            break;
        case 1: v = custom_sat(a, b); break;                 /* sat */
        case 2: v = (int32_t)a < 0 ? 0 : a; break;           /* relu: PROVISÓRIO, ignora rs2 */
        default: goto ilegal;
        }
        escreve_rd = 1;
        break;
    default:
        goto ilegal;
    }

    if (escreve_rd && rd != 0) {
        s->x[rd] = v;
        r->rd_valid = 1;
        r->rd = (uint8_t)rd;
        r->rd_wdata = v;
    }
    s->pc = pc_prox;
    /* A escrita explícita num contador prevalece sobre o incremento da própria instrução. */
    if (!escreveu_instret) s->minstret++;
    if (!escreveu_ciclo) s->mcycle++;
    return s->estado;

ilegal:
    para_trap(s, r, ISS_TRAP_ILEGAL);
    return s->estado;
}

/* ------------------------------------------------------------------ */
/* Trace                                                              */
/* ------------------------------------------------------------------ */

const char *iss_nome_trap(iss_trap_t t)
{
    switch (t) {
    case ISS_TRAP_NENHUM:       return "nenhum";
    case ISS_TRAP_ILEGAL:       return "instrucao ilegal";
    case ISS_TRAP_ECALL:        return "ecall";
    case ISS_TRAP_EBREAK:       return "ebreak";
    case ISS_TRAP_BUSCA_FORA:   return "busca fora da IMEM";
    case ISS_TRAP_DESALINHADO:  return "acesso desalinhado";
    }
    return "?";
}

void iss_formata_retire(const iss_retire_t *r, char *buf, size_t n)
{
    int k = snprintf(buf, n, "core   0: 3 0x%08x (0x%0*x)", r->pc, r->insn_len == 2 ? 4 : 8, r->insn);
    if (r->rd_valid && k < (int)n)
        k += snprintf(buf + k, n - k, " x%-2u 0x%08x", r->rd, r->rd_wdata);
    if (r->mem_valid && k < (int)n) {
        k += snprintf(buf + k, n - k, " mem 0x%08x", r->mem_addr);
        if (r->mem_write && k < (int)n)
            k += snprintf(buf + k, n - k, " 0x%0*x", 2 * r->mem_size, r->mem_wdata);
    }
    if (r->trap && k < (int)n)
        snprintf(buf + k, n - k, " trap: %s", iss_nome_trap((iss_trap_t)r->trap));
}
