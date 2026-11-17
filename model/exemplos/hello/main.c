/*
 * Exemplo para o ISS: imprime pelo console, lê os contadores e compara um
 * produto escalar int8 feito em C puro com o feito pela instrução custom mac.
 */
#include <stdint.h>

#define CONSOLE (*(volatile uint32_t *)0x20000000u)
#define N 64

static void putc_(char c) { CONSOLE = (uint8_t)c; }

static void puts_(const char *s)
{
    while (*s)
        putc_(*s++);
}

static void putu(uint32_t v)
{
    char b[11];
    int i = 10;
    b[i] = 0;
    do {
        b[--i] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    puts_(b + i);
}

static void puti(int32_t v)
{
    if (v < 0) {
        putc_('-');
        putu((uint32_t)-v);
    } else {
        putu((uint32_t)v);
    }
}

static inline uint32_t instret(void)
{
    uint32_t v;
    asm volatile("csrr %0, minstret" : "=r"(v));
    return v;
}

/* Intrínsecos provisórios; os oficiais são da biblioteca da M4.2. */
static inline void acc_zera(void) { asm volatile("csrwi 0x7c0, 0"); }

static inline int32_t mac(int32_t a, int32_t b)
{
    int32_t r;
    asm volatile(".insn r 0x0B, 0x0, 0x00, %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static int8_t x[N], w[N];

static int32_t dot_c(void)
{
    int32_t s = 0;
    for (int i = 0; i < N; i++)
        s += x[i] * w[i];
    return s;
}

static int32_t dot_mac(void)
{
    int32_t s = 0;
    acc_zera();
    for (int i = 0; i < N; i++)
        s = mac(x[i], w[i]);
    return s;
}

int main(void)
{
    uint32_t t0, n_c, n_mac;
    int32_t r_c, r_mac;

    puts_("Ola do RISC-V! Programa em C rodando no ISS do grupo 4.\n");

    for (int i = 0; i < N; i++) {
        x[i] = (int8_t)(i * 7 - 200);
        w[i] = (int8_t)(100 - i * 3);
    }

    t0 = instret();
    r_c = dot_c();
    n_c = instret() - t0;

    t0 = instret();
    r_mac = dot_mac();
    n_mac = instret() - t0;

    puts_("produto escalar int8, ");
    putu(N);
    puts_(" elementos\n");
    puts_("  C puro : resultado ");
    puti(r_c);
    puts_(", ");
    putu(n_c);
    puts_(" instrucoes\n");
    puts_("  com mac: resultado ");
    puti(r_mac);
    puts_(", ");
    putu(n_mac);
    puts_(" instrucoes\n");

    if (r_c != r_mac) {
        puts_("ERRO: resultados diferentes\n");
        return 1;
    }
    puts_("resultados iguais\n");
    return 0;
}
