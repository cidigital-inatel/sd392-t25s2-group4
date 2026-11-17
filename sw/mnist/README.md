# MNIST

Pacote de IA e aplicação de demonstração MNIST da frente `sw`.

| Caminho | Conteúdo |
|---|---|
| `mnist_int8/` | Modelo quantizado provisório, inferência de referência em C, script de exportação e resultados |
| `dados/MNIST/raw/` | Imagens e rótulos oficiais da partição de teste MNIST, comprimidos e descomprimidos |

O pacote ainda não fecha a meta M4.3. Faltam o notebook reprodutível de treino,
os pesos float32 de origem, as ativações de referência por camada e a
quantização com escalas em potência de dois. Os arquivos atuais servem como
protótipo; detalhes e limitações estão em [mnist_int8/README.md](./mnist_int8/README.md).

Os dados versionados são somente o conjunto oficial de teste (10.000 imagens).
O script de quantização também precisa das imagens de treino e dos pesos
float32, que ainda não estão versionados. Não use os arquivos atuais como
evidência de que o fluxo M4.3 já pode ser reproduzido do zero.
