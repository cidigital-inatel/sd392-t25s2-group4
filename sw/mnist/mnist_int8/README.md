# Protótipo da CNN MNIST quantizada

Este diretório contém um protótipo de inferência int8, não a entrega completa
da M4.3. O modelo foi exportado de pesos float32 de origem que não estão
versionados. O script `quantizar.py` depende desses pesos e das imagens de
treino; portanto, os resultados não podem ser reproduzidos a partir apenas dos
arquivos atualmente no repositório.

## Resultados registrados

| Modelo | Acurácia | Acertos |
|---|---:|---:|
| Float32 | 98,37% | 9.837 / 10.000 |
| Int8 | 98,33% | 9.833 / 10.000 |

Diferença: −0,04 ponto percentual. Concordância entre previsões: 99,93%.
Os resultados completos estão em `resultados_int8.json` e
`matriz_confusao_int8.csv`.

## Arquivos

- `modelo_int8.npz`: pesos, bias, escalas e multiplicadores exportados.
- `modelo_int8.h`: arrays do modelo para inclusão no C.
- `inferencia_int8.c`: referência portátil em aritmética inteira. A função
  `mnist_inferir_int8` recebe 784 pixels `uint8_t` e escreve dez logits `int32_t`.
- `vetores_teste.npz`: 100 imagens, rótulos, previsões e logits finais
  esperados. Ainda não contém resultados intermediários por camada.
- `quantizar.py`: script de quantização e avaliação; requer NumPy, os pesos
  float32 e as imagens de treino e teste.

## Convenção numérica atual

Quantização simétrica por tensor, zero point 0, pesos em [-127,127]. Pixels e
ativações após ReLU ficam em [0,127]. Bias e somas de produtos ficam em int32.
A requantização usa multiplicador Q30, intermediário int64, arredondamento
(empates para cima) e saturação em 127 antes do max pooling.

**Limitação importante:** as escalas atuais não são restritas a potências de
dois e a requantização não é apenas um deslocamento aritmético. Portanto, esta
versão ainda não atende ao requisito de quantização da M4.3 em
`docs/checklist.md`. Os pesos, previsões e medidas registrados pertencem a
esta convenção atual; não devem ser apresentados como resultados de uma
quantização power-of-two.

## Reproduzir a exportação

Instale NumPy no ambiente Python do projeto e forneça os pesos float32 e os
arquivos IDX. Por exemplo, a partir da raiz do repositório:

```sh
python3 sw/mnist/mnist_int8/quantizar.py \
  --pesos /caminho/pesos_float32.npz \
  --treino /caminho/train-images-idx3-ubyte \
  --teste sw/mnist/dados/MNIST/raw/t10k-images-idx3-ubyte \
  --rotulos sw/mnist/dados/MNIST/raw/t10k-labels-idx1-ubyte \
  --saida /caminho/saida
```

Os IDX oficiais foram obtidos do espelho
<https://storage.googleapis.com/cvdf-datasets/mnist/>. Os arquivos de teste
estão em `../dados/MNIST/raw/`; os dados de treino e os pesos float32 ainda
precisam ser fornecidos.

A inferência C atual reproduziu exatamente os dez logits esperados para as
100 imagens de `vetores_teste.npz` em uma validação local. Ainda faltam a
referência NumPy bit-exata da M4.3, os vetores intermediários por camada e a
execução bare-metal RV32IMAC com comparação de ciclos e instruções.
