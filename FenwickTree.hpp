#pragma once
#include <vector>
#include <iostream>

// Fenwick Tree (Binary Indexed Tree) para somas de prefixo em O(log N)
// Indexação baseada em 1: posições válidas de 1 a N
class FenwickTree {
private:
    int tamanho;                            // Declarado primeiro — usado nos vetores abaixo
    std::vector<double> arvore;
    std::vector<double> valores_originais;  // Guarda valores para suportar remoção/substituição

public:
    // Construtor: aloca tamanho+1 posições (índice 0 não usado)
    explicit FenwickTree(int n)
        : tamanho(n), arvore(n + 1, 0.0), valores_originais(n + 1, 0.0)
    {}

    // ── CRUD: Inserir / Atualizar (delta) ─────────────────────────────────────
    // Adiciona 'delta' à posição idx. Complexidade: O(log N)
    void atualizar(int idx, double delta) {
        if (idx < 1 || idx > tamanho) return;
        valores_originais[idx] += delta;
        for (int i = idx; i <= tamanho; i += i & (-i))
            arvore[i] += delta;
    }

    // ── CRUD: Remover / Substituir um valor pontual ───────────────────────────
    // Substitui o valor em idx por novo_valor. Complexidade: O(log N)  [CRITÉRIO 1]
    void substituir(int idx, double novo_valor) {
        if (idx < 1 || idx > tamanho) return;
        double delta = novo_valor - valores_originais[idx];
        atualizar(idx, delta); // atualizar já soma delta e atualiza valores_originais via referência
        // Correção: atualizar soma ao original, então ajustamos manualmente:
        valores_originais[idx] = novo_valor;
    }

    // Zera o valor em uma posição (remoção lógica)
    bool remover(int idx) {
        if (idx < 1 || idx > tamanho || valores_originais[idx] == 0.0) return false;
        double delta = -valores_originais[idx];
        valores_originais[idx] = 0.0;
        for (int i = idx; i <= tamanho; i += i & (-i))
            arvore[i] += delta;
        return true;
    }

    // ── CRUD: Buscar valor original numa posição ──────────────────────────────
    double buscar(int idx) const {
        if (idx < 1 || idx > tamanho) return 0.0;
        return valores_originais[idx];
    }

    // ── Consultas ─────────────────────────────────────────────────────────────
    // Soma acumulada de 1 até idx. Complexidade: O(log N)
    double consultar_prefixo(int idx) const {
        double soma = 0.0;
        for (int i = idx; i > 0; i -= i & (-i))
            soma += arvore[i];
        return soma;
    }

    // Soma no intervalo [esquerdo, direito]. Complexidade: O(log N)
    double consultar_intervalo(int esquerdo, int direito) const {
        if (esquerdo < 1 || direito > tamanho || esquerdo > direito) return 0.0;
        return consultar_prefixo(direito) - consultar_prefixo(esquerdo - 1);
    }

    // ── Utilitários para benchmark (Critério 9) ───────────────────────────────
    int getTamanho() const { return tamanho; }

    void exibirEstatisticas() const {
        std::cout << "[FenwickTree] Tamanho: " << tamanho
                  << " | Soma total: " << consultar_prefixo(tamanho) << "\n";
    }
};