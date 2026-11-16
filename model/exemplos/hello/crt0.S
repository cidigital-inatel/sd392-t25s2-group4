/*
 * Inicialização mínima: gp, sp, zera .bss, chama main e escreve
 * (retorno << 1) | 1 no tohost. PROVISÓRIO: o oficial é sw/common/crt0.S (M4.1).
 */
    .section .text.init
    .globl _start
_start:
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop
    la      sp, __stack_top

    la      t0, __bss_start
    la      t1, __bss_end
1:  bgeu    t0, t1, 2f
    sw      zero, 0(t0)
    addi    t0, t0, 4
    j       1b

2:  call    main

    slli    a0, a0, 1
    ori     a0, a0, 1
    li      t0, 0x20000004      /* tohost */
    sw      a0, 0(t0)
3:  j       3b
