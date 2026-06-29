#pragma once
#include <iostream>

// Elemento da tabela com suporte a exclusão lógica (lazy deletion)
struct ElementoHash {
    int    id_sensor;
    double ultimo_valor;
    bool   ocupado;
    bool   removido; // Marca de exclusão lógica — necessária para sondagem linear correta

    ElementoHash() : id_sensor(-1), ultimo_valor(0.0), ocupado(false), removido(false) {}
};

class TabelaHash {
private:
    static const int TAMANHO_TABELA = 1009; // Número primo reduz colisões
    ElementoHash tabela[TAMANHO_TABELA];
    int total_colisoes; // Contador para benchmark (Critério 9)
    int total_elementos;

    // Função hash: resto da divisão por primo
    int funcao_hash(int id) const {
        return std::abs(id) % TAMANHO_TABELA;
    }

public:
    TabelaHash() : total_colisoes(0), total_elementos(0) {}

    // ── CRUD: Inserir / Atualizar ─────────────────────────────────────────────
    // Complexidade: O(1) amortizado; O(N) pior caso (tabela quase cheia)
    bool inserir_ou_atualizar(int id, double valor) {
        int pos         = funcao_hash(id);
        int pos_inicial = pos;
        int slot_livre  = -1; // Primeiro slot marcado como removido (reutilizável)

        while (tabela[pos].ocupado || tabela[pos].removido) {
            // Atualiza se o ID já existe (mesmo que após slots removidos)
            if (tabela[pos].ocupado && tabela[pos].id_sensor == id) {
                tabela[pos].ultimo_valor = valor;
                return true;
            }
            // Guarda o primeiro slot reutilizável
            if (tabela[pos].removido && slot_livre == -1)
                slot_livre = pos;

            pos = (pos + 1) % TAMANHO_TABELA;
            total_colisoes++;

            if (pos == pos_inicial) {
                // Tabela cheia mas pode haver slot reutilizável
                if (slot_livre != -1) break;
                std::cerr << "[Hash] Erro: tabela cheia sem slots livres.\n";
                return false;
            }
        }

        // Usa slot reutilizável se disponível, senão usa posição livre encontrada
        int alvo = (slot_livre != -1) ? slot_livre : pos;
        // Só incrementa se for inserção real (não atualização de ID existente)
        bool e_novo = !(tabela[alvo].ocupado && tabela[alvo].id_sensor == id);
        tabela[alvo].id_sensor    = id;
        tabela[alvo].ultimo_valor = valor;
        tabela[alvo].ocupado      = true;
        tabela[alvo].removido     = false;
        if (e_novo) total_elementos++;
        return true;
    }

    // ── CRUD: Buscar ──────────────────────────────────────────────────────────
    // Complexidade: O(1) amortizado
    bool buscar(int id, double& valor_encontrado) const {
        int pos         = funcao_hash(id);
        int pos_inicial = pos;

        // Percorre enquanto houver slots ocupados OU removidos (manter cadeia de sondagem)
        while (tabela[pos].ocupado || tabela[pos].removido) {
            if (tabela[pos].ocupado && tabela[pos].id_sensor == id) {
                valor_encontrado = tabela[pos].ultimo_valor;
                return true;
            }
            pos = (pos + 1) % TAMANHO_TABELA;
            if (pos == pos_inicial) break;
        }
        return false;
    }

    // ── CRUD: Remover ─────────────────────────────────────────────────────────
    // Exclusão lógica: mantém a cadeia de sondagem intacta  [CRITÉRIO 1]
    // Complexidade: O(1) amortizado
    bool remover(int id) {
        int pos         = funcao_hash(id);
        int pos_inicial = pos;

        while (tabela[pos].ocupado || tabela[pos].removido) {
            if (tabela[pos].ocupado && tabela[pos].id_sensor == id) {
                tabela[pos].ocupado  = false;  // Libera o slot fisicamente
                tabela[pos].removido = true;   // Mantém a cadeia de sondagem
                tabela[pos].id_sensor = -1;
                total_elementos--;
                return true;
            }
            pos = (pos + 1) % TAMANHO_TABELA;
            if (pos == pos_inicial) break;
        }
        return false;
    }

    // ── Métricas para benchmark (Critério 9) ─────────────────────────────────
    int getColisoes()    const { return total_colisoes; }
    int getTotalSlots()  const { return TAMANHO_TABELA; }
    int getTotalItens()  const { return total_elementos; }
    double getFatorCarga() const {
        return static_cast<double>(total_elementos) / TAMANHO_TABELA;
    }

    void resetarColisoes() { total_colisoes = 0; }

    void exibirEstatisticas() const {
        std::cout << "[TabelaHash] Itens: "   << total_elementos
                  << " | Slots: "             << TAMANHO_TABELA
                  << " | Fator de carga: "    << getFatorCarga()
                  << " | Colisões totais: "   << total_colisoes << "\n";
    }
};