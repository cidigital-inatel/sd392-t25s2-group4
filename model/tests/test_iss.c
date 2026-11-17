/*
 * Autoteste do ISS. Os programas são montados aqui mesmo, por um mini-montador
 * em C, para não depender do toolchain RISC-V. Cada teste carrega um programa,
 * executa até o tohost e confere registradores, memória e saída do console.
 *
 * Validação definitiva (M3.2): riscv-tests e comparação de trace com o Spike.
 */
#include "../iss.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Mini-montador                                                      */
/* ------------------------------------------------------------------ */

enum { ZERO, RA, SP, GP, TP, T0, T1, T2, S0, S1, A0, A1, A2, A3, A4, A5, A6, A7 };

typedef struct {
    uint8_t buf[4096];
    uint32_t n;
} prog_t;

static void e32(prog_t *p, uint32_t w)
{
    int k;
    for (k = 0; k < 4; k++)
        p->buf[p->n++] = (uint8_t)(w >> (8 * k));
}
static void e16(prog_t *p, uint16_t h)
{
    p->buf[p->n++] = (uint8_t)h;
    p->buf[p->n++] = (uint8_t)(h >> 8);
}

static uint32_t R(uint32_t f7, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t rd, uint32_t op)
{
    return f7 << 25 | rs2 << 20 | rs1 << 15 | f3 << 12 | rd << 7 | op;
}
static uint32_t I(int32_t imm, uint32_t rs1, uint32_t f3, uint32_t rd, uint32_t op)
{
    return (uint32_t)(imm & 0xfff) << 20 | rs1 << 15 | f3 << 12 | rd << 7 | op;
}
static uint32_t S(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3)
{
    return (uint32_t)((imm >> 5) & 0x7f) << 25 | rs2 << 20 | rs1 << 15 | f3 << 12 |
           (uint32_t)(imm & 0x1f) << 7 | 0x23;
}
static uint32_t B(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3)
{
    uint32_t u = (uint32_t)imm;
    return (u >> 12 & 1) << 31 | (u >> 5 & 0x3f) << 25 | rs2 << 20 | rs1 << 15 | f3 << 12 |
           (u >> 1 & 0xf) << 8 | (u >> 11 & 1) << 7 | 0x63;
}
static uint32_t J(uint32_t rd, int32_t imm)
{
    uint32_t u = (uint32_t)imm;
    return (u >> 20 & 1) << 31 | (u >> 1 & 0x3ff) << 21 | (u >> 11 & 1) << 20 |
           (u >> 12 & 0xff) << 12 | rd << 7 | 0x6f;
}

#define ADDI(rd, rs1, imm)  I(imm, rs1, 0, rd, 0x13)
#define LI(rd, imm)         ADDI(rd, ZERO, imm)
#define LUI(rd, imm20)      ((uint32_t)(imm20) << 12 | (rd) << 7 | 0x37)
#define ADD(rd, a, b)       R(0, b, a, 0, rd, 0x33)
#define SUB(rd, a, b)       R(0x20, b, a, 0, rd, 0x33)
#define MUL(rd, a, b)       R(1, b, a, 0, rd, 0x33)
#define MULH(rd, a, b)      R(1, b, a, 1, rd, 0x33)
#define MULHU(rd, a, b)     R(1, b, a, 3, rd, 0x33)
#define DIV(rd, a, b)       R(1, b, a, 4, rd, 0x33)
#define DIVU(rd, a, b)      R(1, b, a, 5, rd, 0x33)
#define REM(rd, a, b)       R(1, b, a, 6, rd, 0x33)
#define LW(rd, off, rs1)    I(off, rs1, 2, rd, 0x03)
#define LB(rd, off, rs1)    I(off, rs1, 0, rd, 0x03)
#define LBU(rd, off, rs1)   I(off, rs1, 4, rd, 0x03)
#define SW(rs2, off, rs1)   S(off, rs2, rs1, 2)
#define SB(rs2, off, rs1)   S(off, rs2, rs1, 0)
#define BEQ(a, b, off)      B(off, b, a, 0)
#define BNE(a, b, off)      B(off, b, a, 1)
#define BLT(a, b, off)      B(off, b, a, 4)
#define JALR(rd, rs1, off)  I(off, rs1, 0, rd, 0x67)
#define CSRRW(rd, csr, rs1) I(csr, rs1, 1, rd, 0x73)
#define CSRRS(rd, csr, rs1) I(csr, rs1, 2, rd, 0x73)
#define CSRRWI(rd, csr, u)  I(csr, u, 5, rd, 0x73)
#define MAC(rd, a, b)       R(0, b, a, 0, rd, 0x0b)
#define SAT(rd, a, b)       R(0, b, a, 1, rd, 0x0b)
#define RELU(rd, a)         R(0, 0, a, 2, rd, 0x0b)
#define LR_W(rd, rs1)       R(0x02 << 2, 0, rs1, 2, rd, 0x2f)
#define SC_W(rd, rs2, rs1)  R(0x03 << 2, rs2, rs1, 2, rd, 0x2f)
#define AMOADD_W(rd, rs2, rs1) R(0x00 << 2, rs2, rs1, 2, rd, 0x2f)
#define AMOSWAP_W(rd, rs2, rs1) R(0x01 << 2, rs2, rs1, 2, rd, 0x2f)

/* t0 <- base dos periféricos; tohost <- (codigo << 1) | 1 */
static void fim(prog_t *p, int codigo)
{
    e32(p, LUI(T0, 0x20000));
    e32(p, LI(T1, (codigo << 1) | 1));
    e32(p, SW(T1, 4, T0));
}

/* ------------------------------------------------------------------ */
/* Infraestrutura de teste                                            */
/* ------------------------------------------------------------------ */

static int falhas, verificacoes;
static char console[256];
static unsigned console_n;

static void putc_buf(char c, void *ctx)
{
    (void)ctx;
    if (console_n + 1 < sizeof(console))
        console[console_n++] = c;
    console[console_n] = 0;
}

#define CONFERE(cond, ...)                                       \
    do {                                                         \
        verificacoes++;                                          \
        if (!(cond)) {                                           \
            falhas++;                                            \
            printf("    FALHA %s:%d: ", __FILE__, __LINE__);     \
            printf(__VA_ARGS__);                                 \
            printf("\n");                                        \
        }                                                        \
    } while (0)

#define CONFERE_REG(s, r, esperado) \
    CONFERE((s)->x[r] == (uint32_t)(esperado), "x%d = 0x%08x, esperado 0x%08x", r, (s)->x[r], (uint32_t)(esperado))

static void roda(iss_t *s, prog_t *p, const uint8_t *dados, uint32_t ndados)
{
    unsigned long k;
    iss_init(s, ISS_IMEM_SIZE_PADRAO, ISS_DMEM_SIZE_PADRAO);
    s->console_putc = putc_buf;
    console_n = 0;
    console[0] = 0;
    iss_write_bytes(s, ISS_IMEM_BASE, p->buf, p->n);
    if (dados)
        iss_write_bytes(s, ISS_DMEM_BASE, dados, ndados);
    iss_reset(s, ISS_IMEM_BASE);
    for (k = 0; k < 100000 && s->estado == ISS_EXECUTANDO; k++)
        iss_step(s, NULL);
}

static void confere_passou(iss_t *s)
{
    CONFERE(s->estado == ISS_FIM_TOHOST && s->codigo_saida == 0,
            "terminou com estado %d, codigo %u, trap '%s' em pc 0x%08x",
            s->estado, s->codigo_saida, iss_nome_trap(s->trap), s->pc);
}

/* ------------------------------------------------------------------ */
/* Testes                                                             */
/* ------------------------------------------------------------------ */

/* Laço com desvio: soma de 1 a 10. */
static void teste_soma(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, LI(A0, 0));
    e32(&p, LI(A1, 1));
    e32(&p, LI(A2, 11));
    e32(&p, ADD(A0, A0, A1));     /* laço: */
    e32(&p, ADDI(A1, A1, 1));
    e32(&p, BNE(A1, A2, -8));
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE_REG(&s, A0, 55);
    CONFERE(s.minstret == 3 + 3 * 10 + 3, "minstret = %lu", (unsigned long)s.minstret);
    iss_free(&s);
}

/* Programa "hello": lê a string da DMEM byte a byte e escreve no console. */
static void teste_hello(void)
{
    static const char msg[] = "Ola, RISC-V! O ISS do grupo 4 esta vivo.\n";
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, LUI(A0, 0x10000));    /* a0 <- endereço da string na DMEM */
    e32(&p, LUI(T0, 0x20000));    /* t0 <- console */
    e32(&p, LBU(A1, 0, A0));      /* laço: */
    e32(&p, BEQ(A1, ZERO, 16));
    e32(&p, SW(A1, 0, T0));
    e32(&p, ADDI(A0, A0, 1));
    e32(&p, J(ZERO, -16));
    fim(&p, 0);
    roda(&s, &p, (const uint8_t *)msg, sizeof(msg));
    confere_passou(&s);
    CONFERE(strcmp(console, msg) == 0, "console = \"%s\"", console);
    iss_free(&s);
}

/* Loads e stores de todos os tamanhos, com extensão de sinal. */
static void teste_memoria(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, LUI(A0, 0x10000));
    e32(&p, LI(A1, -2));          /* 0xfffffffe */
    e32(&p, SW(A1, 0, A0));
    e32(&p, LI(A2, 0x7f));
    e32(&p, SB(A2, 1, A0));       /* palavra fica 0xffff7ffe */
    e32(&p, LW(A3, 0, A0));
    e32(&p, LB(A4, 0, A0));       /* 0xfe com sinal = -2 */
    e32(&p, LBU(A5, 0, A0));      /* 0xfe sem sinal = 254 */
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE_REG(&s, A3, 0xffff7ffe);
    CONFERE_REG(&s, A4, -2);
    CONFERE_REG(&s, A5, 254);
    iss_free(&s);
}

/* Extensão M, incluindo os casos de borda da divisão. */
static void teste_m(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, LI(A0, -7));
    e32(&p, LI(A1, 3));
    e32(&p, MUL(A2, A0, A1));     /* -21 */
    e32(&p, DIV(A3, A0, A1));     /* -2 (trunca para zero) */
    e32(&p, REM(A4, A0, A1));     /* -1 */
    e32(&p, DIV(A5, A0, ZERO));   /* divisão por zero: -1 */
    e32(&p, REM(A6, A0, ZERO));   /* resto por zero: dividendo */
    e32(&p, LUI(T2, 0x80000));    /* INT_MIN */
    e32(&p, LI(S0, -1));
    e32(&p, DIV(S1, T2, S0));     /* overflow: INT_MIN */
    e32(&p, MULH(A7, T2, T2));    /* (-2^31)^2 >> 32 = 2^30 */
    e32(&p, DIVU(GP, A0, A1));    /* 0xfffffff9 / 3 */
    e32(&p, MULHU(TP, S0, S0));   /* 0xffffffff^2 >> 32 = 0xfffffffe */
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE_REG(&s, A2, -21);
    CONFERE_REG(&s, A3, -2);
    CONFERE_REG(&s, A4, -1);
    CONFERE_REG(&s, A5, -1);
    CONFERE_REG(&s, A6, -7);
    CONFERE_REG(&s, S1, 0x80000000u);
    CONFERE_REG(&s, A7, 0x40000000);
    CONFERE_REG(&s, GP, 0xfffffff9u / 3);
    CONFERE_REG(&s, TP, 0xfffffffeu);
    iss_free(&s);
}

/* Instruções custom: mac com acumulador em CSR, sat e relu. */
static void teste_custom(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, CSRRWI(ZERO, 0x7C0, 0));  /* ACC <- 0 */
    e32(&p, LI(A0, 3));
    e32(&p, LI(A1, -4));
    e32(&p, MAC(A2, A0, A1));         /* ACC = -12, a2 = -12 */
    e32(&p, MAC(ZERO, A0, A0));       /* rd = x0: ACC = -3, sem escrita */
    e32(&p, MAC(A3, A1, A1));         /* ACC = 13 */
    e32(&p, CSRRS(A4, 0x7C0, ZERO));  /* a4 <- ACC */
    e32(&p, LI(T1, 300));
    e32(&p, LI(T2, 8));
    e32(&p, SAT(A5, T1, T2));         /* clamp 300 em int8 = 127 */
    e32(&p, LI(T1, -300));
    e32(&p, SAT(A6, T1, T2));         /* -128 */
    e32(&p, LI(T1, 100));
    e32(&p, SAT(A7, T1, T2));         /* dentro da faixa: 100 */
    e32(&p, LI(T1, -5));
    e32(&p, RELU(S0, T1));            /* 0 */
    e32(&p, RELU(S1, A5));            /* 127 */
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE_REG(&s, A2, -12);
    CONFERE_REG(&s, A3, 13);
    CONFERE_REG(&s, A4, 13);
    CONFERE_REG(&s, A5, 127);
    CONFERE_REG(&s, A6, -128);
    CONFERE_REG(&s, A7, 100);
    CONFERE_REG(&s, S0, 0);
    CONFERE_REG(&s, S1, 127);
    iss_free(&s);
}

/* Equivalência mac == mul + add com wrap-around (especificação, seção 4.2). */
static void teste_mac_wrap(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, LUI(A0, 0x7ffff));
    e32(&p, LUI(A1, 0x00010));
    e32(&p, LUI(T1, 0x12345));
    e32(&p, CSRRW(ZERO, 0x7C0, T1));  /* ACC <- 0x12345000 */
    e32(&p, MAC(A2, A0, A1));
    e32(&p, MUL(A3, A0, A1));         /* referência: mul + add */
    e32(&p, ADD(A3, A3, T1));
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE(s.x[A2] == s.x[A3], "mac = 0x%08x, mul+add = 0x%08x", s.x[A2], s.x[A3]);
    iss_free(&s);
}

/* Extensão A: lr/sc com e sem reserva, amoadd e amoswap. */
static void teste_a(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, LUI(A0, 0x10000));
    e32(&p, LI(T1, 10));
    e32(&p, SW(T1, 0, A0));
    e32(&p, LR_W(A1, A0));            /* a1 = 10, reserva */
    e32(&p, ADDI(T1, A1, 5));
    e32(&p, SC_W(A2, T1, A0));        /* sucesso: a2 = 0, mem = 15 */
    e32(&p, SC_W(A3, T1, A0));        /* sem reserva: a3 = 1 */
    e32(&p, LI(T2, 7));
    e32(&p, AMOADD_W(A4, T2, A0));    /* a4 = 15, mem = 22 */
    e32(&p, AMOSWAP_W(A5, ZERO, A0)); /* a5 = 22, mem = 0 */
    e32(&p, LW(A6, 0, A0));
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE_REG(&s, A1, 10);
    CONFERE_REG(&s, A2, 0);
    CONFERE_REG(&s, A3, 1);
    CONFERE_REG(&s, A4, 15);
    CONFERE_REG(&s, A5, 22);
    CONFERE_REG(&s, A6, 0);
    iss_free(&s);
}

/* Expansão das comprimidas contra as codificações do objdump. */
static void teste_expansao_c(void)
{
    static const struct { uint16_t c; const char *nome; } casos[] = {
        {0x4515, "c.li a0,5"},       {0x0505, "c.addi a0,1"},     {0x157d, "c.addi a0,-1"},
        {0x952e, "c.add a0,a1"},     {0x85aa, "c.mv a1,a0"},      {0xc188, "c.sw a0,0(a1)"},
        {0x4190, "c.lw a2,0(a1)"},   {0xfd7d, "c.bnez a0,-2"},    {0xc111, "c.beqz a0,4"},
        {0x8082, "c.jr ra"},         {0x1141, "c.addi sp,-16"},   {0xc606, "c.swsp ra,12(sp)"},
        {0x40b2, "c.lwsp ra,12(sp)"},{0x0028, "c.addi4spn a0,sp,8"}, {0x6505, "c.lui a0,1"},
        {0x8905, "c.andi a0,1"},     {0x8d0d, "c.sub a0,a1"},     {0x0506, "c.slli a0,1"},
        {0x8105, "c.srli a0,1"},     {0x8505, "c.srai a0,1"},     {0x9002, "c.ebreak"},
        {0xbff5, "c.j -4"},          {0x713d, "c.addi16sp sp,-32"},
    };
    const uint32_t esperado[] = {
        LI(A0, 5), ADDI(A0, A0, 1), ADDI(A0, A0, -1),
        ADD(A0, A0, A1), ADD(A1, ZERO, A0), SW(A0, 0, A1),
        LW(A2, 0, A1), BNE(A0, ZERO, -2), BEQ(A0, ZERO, 4),
        JALR(ZERO, RA, 0), ADDI(SP, SP, -16), SW(RA, 12, SP),
        LW(RA, 12, SP), ADDI(A0, SP, 8), LUI(A0, 1),
        I(1, A0, 7, A0, 0x13), SUB(A0, A0, A1), R(0, 1, A0, 1, A0, 0x13),
        R(0, 1, A0, 5, A0, 0x13), R(0x20, 1, A0, 5, A0, 0x13), 0x00100073,
        J(ZERO, -4), ADDI(SP, SP, -32),
    };
    unsigned k;
    for (k = 0; k < sizeof(casos) / sizeof(casos[0]); k++) {
        uint32_t e = iss_expande_c(casos[k].c);
        CONFERE(e == esperado[k], "%s (0x%04x) expandiu para 0x%08x, esperado 0x%08x",
                casos[k].nome, casos[k].c, e, esperado[k]);
    }
    CONFERE(iss_expande_c(0x0000) == 0, "0x0000 deve ser ilegal");
}

/* Programa com instruções de 16 e 32 bits misturadas: PC + 2 e PC + 4. */
static void teste_programa_c(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e16(&p, 0x4515);              /* c.li a0,5 */
    e32(&p, LI(A1, 0));
    e16(&p, 0x0585);              /* c.addi a1,1    (laço) */
    e16(&p, 0x157d);              /* c.addi a0,-1 */
    e16(&p, 0xfd75);              /* c.bnez a0,-4 */
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE_REG(&s, A0, 0);
    CONFERE_REG(&s, A1, 5);
    iss_free(&s);
}

/* Contadores lidos por csrr. */
static void teste_contadores(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    e32(&p, LI(ZERO, 0));
    e32(&p, LI(ZERO, 0));
    e32(&p, CSRRS(A0, ISS_CSR_MINSTRET, ZERO)); /* 2 instruções antes desta */
    e32(&p, CSRRS(A1, ISS_CSR_MCYCLE, ZERO));
    fim(&p, 0);
    roda(&s, &p, NULL, 0);
    confere_passou(&s);
    CONFERE_REG(&s, A0, 2);
    CONFERE_REG(&s, A1, 3);
    iss_free(&s);
}

/* Término com erro pelo tohost e parada em instrução ilegal. */
static void teste_termino(void)
{
    prog_t p = { {0}, 0 };
    iss_t s;
    fim(&p, 3);
    roda(&s, &p, NULL, 0);
    CONFERE(s.estado == ISS_FIM_TOHOST && s.codigo_saida == 3, "codigo %u", s.codigo_saida);
    iss_free(&s);

    p.n = 0;
    e32(&p, LI(A0, 1));
    e32(&p, 0xffffffff);          /* ilegal */
    roda(&s, &p, NULL, 0);
    CONFERE(s.estado == ISS_FIM_TRAP && s.trap == ISS_TRAP_ILEGAL && s.pc == 4,
            "estado %d, trap '%s', pc 0x%08x", s.estado, iss_nome_trap(s.trap), s.pc);
    iss_free(&s);
}

/* Formato do trace: o mesmo do --log-commits do Spike. */
static void teste_trace(void)
{
    iss_retire_t r;
    char linha[160];
    memset(&r, 0, sizeof(r));
    r.pc = 0x100; r.insn = 0x00500513; r.insn_len = 4;
    r.rd_valid = 1; r.rd = 10; r.rd_wdata = 5;
    iss_formata_retire(&r, linha, sizeof(linha));
    CONFERE(!strcmp(linha, "core   0: 3 0x00000100 (0x00500513) x10 0x00000005"), "'%s'", linha);

    memset(&r, 0, sizeof(r));
    r.pc = 0x104; r.insn = 0xc188; r.insn_len = 2;
    r.mem_valid = 1; r.mem_write = 1; r.mem_size = 4; r.mem_addr = 0x10000000; r.mem_wdata = 0x2a;
    iss_formata_retire(&r, linha, sizeof(linha));
    CONFERE(!strcmp(linha, "core   0: 3 0x00000104 (0xc188) mem 0x10000000 0x0000002a"), "'%s'", linha);
}

int main(void)
{
    static const struct { const char *nome; void (*f)(void); } testes[] = {
        {"soma de 1 a 10 (laco e desvio)", teste_soma},
        {"hello pelo console", teste_hello},
        {"loads e stores", teste_memoria},
        {"extensao M", teste_m},
        {"instrucoes custom mac/sat/relu", teste_custom},
        {"mac == mul + add com wrap-around", teste_mac_wrap},
        {"extensao A", teste_a},
        {"expansao das comprimidas", teste_expansao_c},
        {"programa com comprimidas", teste_programa_c},
        {"contadores", teste_contadores},
        {"termino e trap", teste_termino},
        {"formato do trace", teste_trace},
    };
    unsigned k;
    for (k = 0; k < sizeof(testes) / sizeof(testes[0]); k++) {
        int antes = falhas;
        testes[k].f();
        printf("[%s] %s\n", falhas == antes ? " OK " : "FALHA", testes[k].nome);
        if (k == 1)
            printf("       console: %s", console);
    }
    printf("\n%d verificacoes, %d falhas\n", verificacoes, falhas);
    return falhas != 0;
}
