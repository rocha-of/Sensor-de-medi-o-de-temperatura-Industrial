# Sistema de Monitoramento de Sensores Industriais

Simulador em C++ (padrão C++17) que integra cinco estruturas de dados a um sistema
de monitoramento de sensores de temperatura: lista encadeada (versão dinâmica e
versão com *memory pool*), árvore AVL, tabela hash, Bloom Filter e Fenwick Tree.

## Arquivos

| Arquivo            | Conteúdo                                                        |
|--------------------|----------------------------------------------------------------|
| `main.cpp`         | Orquestração, geração do dataset, menu, benchmarks e testes    |
| `ArvoreAVL.hpp`    | Árvore AVL auto-balanceada                                      |
| `TabelaHash.hpp`   | Tabela hash com endereçamento aberto                           |
| `bloomfilter.hpp`  | Bloom Filter probabilístico                                     |
| `FenwickTree.hpp`  | Fenwick Tree (Binary Indexed Tree)                             |
| `sensores`         | Executável já compilado para **Linux x86-64**                  |

## Como compilar

Requisito: compilador g++ com suporte a C++17 (GCC 7+ ou equivalente).

### Linux / macOS
```bash
./build.sh
# ou diretamente:
g++ -std=c++17 -O2 -Wall -o sensores main.cpp
```

### Windows (MinGW-w64)
Instale o MinGW-w64 (por exemplo, via https://www.msys2.org/) e garanta que o
`g++` esteja no PATH. Em seguida, dê dois cliques em `build.bat` ou execute:
```bat
g++ -std=c++17 -O2 -Wall -o sensores.exe main.cpp
```

## Como executar

```bash
./sensores        # Linux / macOS
sensores.exe      # Windows
```

Ao iniciar, o programa indexa 10.000 amostras sintéticas e abre um menu interativo:

- **[1]** Estado atual dos sensores (Tabela Hash)
- **[2]** Busca histórica por timestamp (AVL)
- **[3]** Média térmica por intervalo (Fenwick Tree)
- **[4]** Triagem de segurança (Bloom Filter)
- **[5]** Histórico recente (Lista com *pool*)
- **[6]** Remover sensor (CRUD em todas as estruturas)
- **[7]** Benchmarks de desempenho
- **[8]** Cenários de teste em condições restritivas
- **[0]** Encerrar

## Observação sobre o executável pré-compilado

O arquivo `sensores` foi compilado para **Linux x86-64**. Em Windows ou macOS,
recompile a partir do código-fonte com os comandos acima — o resultado é idêntico,
mudando apenas o binário nativo da sua plataforma.
