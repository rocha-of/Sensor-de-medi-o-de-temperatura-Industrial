#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>

struct NodeAVL {
    double key;
    int id_sensor;
    double valor;
    NodeAVL* left;
    NodeAVL* right;
    int height;

    NodeAVL(double k, int id, double v)
        : key(k), id_sensor(id), valor(v), left(nullptr), right(nullptr), height(1) {}
};

class ArvoreAVL {
private:
    NodeAVL* raiz;

    // ── Utilitários internos ──────────────────────────────────────────────────
    int obterAltura(NodeAVL* n) const {
        return n ? n->height : 0;
    }

    int obterBalanceamento(NodeAVL* n) const {
        return n ? obterAltura(n->left) - obterAltura(n->right) : 0;
    }

    void atualizarAltura(NodeAVL* n) {
        if (n)
            n->height = 1 + std::max(obterAltura(n->left), obterAltura(n->right));
    }

    // Comparação segura para double (evita UB de igualdade exata)
    bool iguais(double a, double b) const {
        return std::fabs(a - b) < 1e-9;
    }

    // ── Rotações ─────────────────────────────────────────────────────────────
    NodeAVL* rotacionarDireita(NodeAVL* y) {
        NodeAVL* x  = y->left;
        NodeAVL* T2 = x->right;
        x->right = y;
        y->left  = T2;
        atualizarAltura(y);
        atualizarAltura(x);
        return x;
    }

    NodeAVL* rotacionarEsquerda(NodeAVL* x) {
        NodeAVL* y  = x->right;
        NodeAVL* T2 = y->left;
        y->left  = x;
        x->right = T2;
        atualizarAltura(x);
        atualizarAltura(y);
        return y;
    }

    // Reequilibra um nó após inserção ou remoção
    NodeAVL* reequilibrar(NodeAVL* node, double key) {
        atualizarAltura(node);
        int bal = obterBalanceamento(node);

        // Caso LL
        if (bal > 1 && key < node->left->key)
            return rotacionarDireita(node);
        // Caso RR
        if (bal < -1 && key > node->right->key)
            return rotacionarEsquerda(node);
        // Caso LR
        if (bal > 1 && key > node->left->key) {
            node->left = rotacionarEsquerda(node->left);
            return rotacionarDireita(node);
        }
        // Caso RL
        if (bal < -1 && key < node->right->key) {
            node->right = rotacionarDireita(node->right);
            return rotacionarEsquerda(node);
        }
        return node;
    }

    // ── Inserção ─────────────────────────────────────────────────────────────
    NodeAVL* inserirNo(NodeAVL* node, double key, int id, double valor) {
        if (!node) return new NodeAVL(key, id, valor);

        if (key < node->key)
            node->left  = inserirNo(node->left,  key, id, valor);
        else if (key > node->key)
            node->right = inserirNo(node->right, key, id, valor);
        else
            return node; // chave duplicada: ignora

        return reequilibrar(node, key);
    }

    // ── Busca ────────────────────────────────────────────────────────────────
    bool buscarNo(NodeAVL* node, double key,
                  int& id_encontrado, double& val_encontrado) const {
        if (!node) return false;

        if (iguais(node->key, key)) {
            id_encontrado  = node->id_sensor;
            val_encontrado = node->valor;
            return true;
        }
        if (key < node->key)
            return buscarNo(node->left,  key, id_encontrado, val_encontrado);
        else
            return buscarNo(node->right, key, id_encontrado, val_encontrado);
    }

    // ── Remoção ──────────────────────────────────────────────────────────────
    // Encontra o nó com menor chave na subárvore (sucessor in-order)
    NodeAVL* menorNo(NodeAVL* node) const {
        NodeAVL* atual = node;
        while (atual->left)
            atual = atual->left;
        return atual;
    }

    NodeAVL* removerNo(NodeAVL* node, double key, bool& removido) {
        if (!node) { removido = false; return nullptr; }

        if (key < node->key) {
            node->left  = removerNo(node->left,  key, removido);
        } else if (key > node->key) {
            node->right = removerNo(node->right, key, removido);
        } else {
            // Nó encontrado
            removido = true;

            // Caso 1: sem filhos ou um filho
            if (!node->left || !node->right) {
                NodeAVL* temp = node->left ? node->left : node->right;
                delete node;
                return temp; // pode ser nullptr (sem filhos)
            }

            // Caso 2: dois filhos — substitui pelo sucessor in-order
            NodeAVL* sucessor = menorNo(node->right);
            node->key       = sucessor->key;
            node->id_sensor = sucessor->id_sensor;
            node->valor     = sucessor->valor;
            // Remove o sucessor da subárvore direita
            bool dummy = false;
            node->right = removerNo(node->right, sucessor->key, dummy);
        }

        // Reequilibra após remoção
        atualizarAltura(node);
        int bal = obterBalanceamento(node);

        // LL
        if (bal > 1 && obterBalanceamento(node->left) >= 0)
            return rotacionarDireita(node);
        // LR
        if (bal > 1 && obterBalanceamento(node->left) < 0) {
            node->left = rotacionarEsquerda(node->left);
            return rotacionarDireita(node);
        }
        // RR
        if (bal < -1 && obterBalanceamento(node->right) <= 0)
            return rotacionarEsquerda(node);
        // RL
        if (bal < -1 && obterBalanceamento(node->right) > 0) {
            node->right = rotacionarDireita(node->right);
            return rotacionarEsquerda(node);
        }

        return node;
    }

    // ── Destruição ───────────────────────────────────────────────────────────
    void destruirArvore(NodeAVL* node) {
        if (node) {
            destruirArvore(node->left);
            destruirArvore(node->right);
            delete node;
        }
    }

    // ── Contagem (utilitário para benchmark) ─────────────────────────────────
    int contarNos(NodeAVL* node) const {
        if (!node) return 0;
        return 1 + contarNos(node->left) + contarNos(node->right);
    }

public:
    ArvoreAVL() : raiz(nullptr) {}
    ~ArvoreAVL() { destruirArvore(raiz); }

    // CRUD: Inserir
    void inserir(double key, int id, double valor) {
        raiz = inserirNo(raiz, key, id, valor);
    }

    // CRUD: Buscar — O(log N)
    bool buscar(double key, int& id, double& valor) const {
        return buscarNo(raiz, key, id, valor);
    }

    // CRUD: Remover — O(log N)  [CRITÉRIO 1]
    bool remover(double key) {
        bool removido = false;
        raiz = removerNo(raiz, key, removido);
        return removido;
    }

    // Utilitário para benchmark
    int tamanho() const { return contarNos(raiz); }
    int altura()  const { return obterAltura(raiz); }
};