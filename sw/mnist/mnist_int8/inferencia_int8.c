#include "modelo_int8.h"

/* ReLU, multiplicador Q30, arredondamento e SAT para int8. */
static int8_t ativar(int32_t acc, int32_t m)
{
    if (acc <= 0) return 0;
    int64_t q = ((int64_t)acc * m + (INT64_C(1) << (MNIST_Q - 1))) >> MNIST_Q;
    return (int8_t)(q > 127 ? 127 : q);
}

/* Calcula cada bloco 2x2 e guarda apenas o maior valor após ReLU/SAT. */
static void conv_pool(const int8_t *x, int8_t *y, int n, int ci, int co,
                      const int8_t *w, const int32_t *b, int32_t m)
{
    const int p = n / 2;
    for (int o = 0; o < co; ++o)
        for (int r = 0; r < p; ++r)
            for (int c = 0; c < p; ++c) {
                int8_t max = 0;
                for (int dr = 0; dr < 2; ++dr)
                    for (int dc = 0; dc < 2; ++dc) {
                        int32_t acc = b[o];
                        for (int i = 0; i < ci; ++i)
                            for (int kr = 0; kr < 3; ++kr)
                                for (int kc = 0; kc < 3; ++kc) {
                                    int ir = 2*r + dr + kr - 1;
                                    int ic = 2*c + dc + kc - 1;
                                    if (ir >= 0 && ir < n && ic >= 0 && ic < n)
                                        acc += (int32_t)x[(i*n+ir)*n+ic] * w[((o*ci+i)*3+kr)*3+kc];
                                }
                        int8_t v = ativar(acc, m);
                        if (v > max) max = v;
                    }
                y[(o*p+r)*p+c] = max;
            }
}

/* Entrada: 784 pixels uint8 em ordem de linhas. Saída: dez logits int32. */
int mnist_inferir_int8(const uint8_t imagem[784], int32_t logits[10])
{
    int8_t x[784], a[8*14*14], b[16*7*7];
    for (int i = 0; i < 784; ++i)
        x[i] = (int8_t)((imagem[i]*127 + 127)/255);
    conv_pool(x,a,28,1,8,mnist_w1,mnist_b1,MNIST_M1);
    conv_pool(a,b,14,8,16,mnist_w2,mnist_b2,MNIST_M2);
    int melhor = 0;
    for (int o = 0; o < 10; ++o) {
        int32_t acc = mnist_b3[o];
        for (int i = 0; i < 784; ++i)
            acc += (int32_t)b[i]*mnist_w3[o*784+i];
        logits[o] = acc;
        if (acc > logits[melhor]) melhor = o;
    }
    return melhor;
}
