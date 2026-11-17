# CNN MNIST quantizada para int8

Modelo produzido a partir de `pesos_float32.npz` do seu treinamento, sem retreinar.
Calibração: 1.024 imagens sorteadas da partição oficial de treino (semente NumPy 42).
Teste: 10.000 imagens da partição oficial de teste, sem usá-las na calibração.

| Modelo | Acurácia | Acertos |
|---|---:|---:|
| Float32 | 98,37% | 9.837 |
| Int8 | 98,33% | 9.833 |

Diferença: −0,04 ponto percentual. Concordância entre previsões: 99,93%.
Os 9.064 pesos int8 e 34 bias int32 somam 9.200 bytes brutos, contra
36.392 bytes em float32. Escalas, metadados, ativações e código ficam fora da conta.

- `modelo_int8.npz`: pesos, bias, escalas e multiplicadores.
- `modelo_int8.h`: parâmetros prontos para incluir no código C.
- `inferencia_int8.c`: referência portátil em aritmética inteira. A função
  `mnist_inferir_int8` recebe 784 pixels uint8 e escreve dez logits int32.
- `vetores_teste.npz`: 100 imagens, rótulos, previsões e logits esperados.
- `resultados_int8.json` e `matriz_confusao_int8.csv`: medidas no teste completo.
- `quantizar.py`: gera os arquivos e repete a avaliação.

## Repetir no Windows

Use o Python com NumPy instalado. Os arquivos IDX baixados pelo treinamento
normalmente ficam em `dados\MNIST\raw`. Execute na pasta deste pacote e ajuste
os caminhos abaixo para a pasta do seu treinamento:

```bat
py -m pip install numpy
py quantizar.py --pesos "CAMINHO\resultados\pesos_float32.npz" --treino "CAMINHO\dados\MNIST\raw\train-images-idx3-ubyte" --teste "CAMINHO\dados\MNIST\raw\t10k-images-idx3-ubyte" --rotulos "CAMINHO\dados\MNIST\raw\t10k-labels-idx1-ubyte" --saida nova_quantizacao
```

## Convenção numérica

Quantização simétrica por tensor, zero point 0, pesos em [-127,127].
Pixels e ativações após ReLU ficam em [0,127]. Bias e somas de produtos ficam
em int32. A saída de cada convolução aplica ReLU, multiplicação por fator
de escala Q30 com intermediário int64, arredondamento (empates para cima),
limitação a 127 e max pooling. A camada linear usa escala comum para todos
os logits, permitindo argmax direto. Em empate, vence a menor classe.
O cálculo da escala é feito offline; a inferência C não usa float.

A referência C compilada no computador reproduziu exatamente os dez logits
das 100 imagens fornecidas. Ainda falta executar no RV32IMAC e comparar ciclos
com e sem MAC, ReLU e SAT. A requantização usa multiplicação larga em software;
a instrução SAT limita o valor, mas não substitui o cálculo de escala.

Referência de contexto: Jacob et al., Quantization and Training of Neural
Networks for Efficient Integer-Arithmetic-Only Inference, https://arxiv.org/abs/1712.05877.
Os dados IDX para a execução foram obtidos no espelho
https://storage.googleapis.com/cvdf-datasets/mnist/.
