/*
 * ISS: simulador do conjunto de instruções RV32IMAC com as extensões custom
 * (mac, sat, relu), com precisão de instrução. Modelo de referência do
 * projeto (meta M3.2); o contrato é docs/especificacao.md.
 *
 * Uso: iss_init -> iss_load_elf/iss_load_hex -> iss_reset -> iss_step em laço.
 * Cada iss_step executa uma instrução e devolve a tupla de retire, que é o que
 * o scoreboard UVM compara com o trace do RTL (via DPI-C) e o que o executável
 * autônomo imprime como trace em texto.
 */
#ifndef ISS_H
#define ISS_H

#include <stddef.h>
#include <stdint.h>

/* Mapa de memória: especificação, seção 2. */
#define ISS_IMEM_BASE        0x00000000u
#define ISS_DMEM_BASE        0x10000000u
#define ISS_PERIPH_BASE      0x20000000u
#define ISS_IMEM_SIZE_PADRAO (64u * 1024u)
#define ISS_DMEM_SIZE_PADRAO (128u * 1024u)
#define ISS_ADDR_CONSOLE     0x20000000u
#define ISS_ADDR_TOHOST      0x20000004u

/* CSRs implementados: especificação, seções 4.2 e 9. */
#define ISS_CSR_MACACC   0x7C0
#define ISS_CSR_MCYCLE   0xB00
#define ISS_CSR_MINSTRET 0xB02
#define ISS_CSR_MCYCLEH  0xB80
#define ISS_CSR_MINSTRETH 0xB82
#define ISS_CSR_CYCLE    0xC00
#define ISS_CSR_INSTRET  0xC02
#define ISS_CSR_CYCLEH   0xC80
#define ISS_CSR_INSTRETH 0xC82

typedef enum {
    ISS_EXECUTANDO = 0,
    ISS_FIM_TOHOST,   /* programa escreveu no tohost com bit 0 = 1 */
    ISS_FIM_TRAP      /* trap: tratamento ainda não decidido (seção 9), o ISS para */
} iss_estado_t;

typedef enum {
    ISS_TRAP_NENHUM = 0,
    ISS_TRAP_ILEGAL,
    ISS_TRAP_ECALL,
    ISS_TRAP_EBREAK,
    ISS_TRAP_BUSCA_FORA,      /* PC fora da IMEM */
    ISS_TRAP_DESALINHADO      /* load, store ou AMO desalinhado (seção 10 a decidir) */
} iss_trap_t;

/* Tupla de retire, no estilo RVFI (especificação, seção 11, ainda em aberto). */
typedef struct {
    uint32_t pc;
    uint32_t insn;        /* instrução crua: 16 bits nos bits baixos se comprimida */
    uint8_t  insn_len;    /* 2 ou 4 */
    uint8_t  rd_valid;    /* 1 se escreveu em rd != x0 */
    uint8_t  rd;
    uint32_t rd_wdata;
    uint8_t  mem_valid;   /* 1 se acessou a porta de dados */
    uint8_t  mem_write;   /* 1 em store, SC bem-sucedido ou AMO */
    uint8_t  mem_size;    /* 1, 2 ou 4 bytes */
    uint8_t  mem_fora;    /* 1 se o endereço está fora do mapa (lê 0, ignora escrita) */
    uint32_t mem_addr;
    uint32_t mem_wdata;
    uint32_t mem_rdata;
    uint32_t trap;        /* iss_trap_t */
} iss_retire_t;

typedef struct {
    uint32_t pc;
    uint32_t x[32];
    uint32_t macacc;      /* acumulador da mac, CSR 0x7C0 */
    uint64_t mcycle;      /* no ISS, um ciclo por instrução: o scoreboard não compara */
    uint64_t minstret;

    uint8_t *imem;
    uint8_t *dmem;
    uint32_t imem_size;
    uint32_t dmem_size;
    uint32_t tohost;

    int      reserva_valida;  /* lr/sc */
    uint32_t reserva_addr;

    iss_estado_t estado;
    uint32_t     codigo_saida;  /* tohost >> 1 */
    iss_trap_t   trap;

    /* Saída do console; NULL descarta os caracteres. */
    void (*console_putc)(char c, void *ctx);
    void *console_ctx;
} iss_t;

int  iss_init(iss_t *s, uint32_t imem_size, uint32_t dmem_size);
void iss_free(iss_t *s);
void iss_reset(iss_t *s, uint32_t pc_reset);

/* Carga antes do reset. Devolvem 0 em sucesso. */
int iss_load_elf(iss_t *s, const char *caminho);
/* Formato $readmemh: uma palavra de 32 bits por linha, "@índice" opcional
   (índice de palavra dentro da região). */
int iss_load_hex(iss_t *s, const char *caminho, uint32_t base);
/* Escrita direta de bytes no mapa, para testes e para o carregador. */
int iss_write_bytes(iss_t *s, uint32_t addr, const uint8_t *dados, uint32_t n);

/* Executa uma instrução. 'r' pode ser NULL. */
iss_estado_t iss_step(iss_t *s, iss_retire_t *r);

/* Expande uma instrução comprimida para a de 32 bits equivalente; 0 se ilegal. */
uint32_t iss_expande_c(uint16_t h);

/* Linha de trace no formato do --log-commits do Spike, para comparar com diff. */
void iss_formata_retire(const iss_retire_t *r, char *buf, size_t n);

const char *iss_nome_trap(iss_trap_t t);

#endif
