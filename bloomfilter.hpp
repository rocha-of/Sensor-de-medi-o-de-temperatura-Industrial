#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>

// Bloom Filter com array de uint8_t (evita a especialização problemática de vector<bool>)
// Usa 3 funções hash independentes para reduzir a taxa de falsos positivos
class BloomFilter {
private:
    uint8_t* bits;       // Array de bytes — cada bit é uma posição do filtro
    int      tamanho;    // Tamanho em bits
    int      num_hashes; // Número de funções hash usadas (padrão: 3)
    int      total_inseridos;

    // Acessa e define bit individual no array de bytes
    void setBit(int pos) {
        bits[pos / 8] |= (1u << (pos % 8));
    }
    bool getBit(int pos) const {
        return (bits[pos / 8] >> (pos % 8)) & 1u;
    }

    // ── Funções hash independentes ────────────────────────────────────────────
    // Hash 1: Murmur-inspired — boa distribuição para inteiros pequenos
    int hash1(int id) const {
        unsigned int h = static_cast<unsigned int>(id);
        h = ((h >> 16) ^ h) * 0x45d9f3bu;
        h = ((h >> 16) ^ h) * 0x45d9f3bu;
        h = (h >> 16) ^ h;
        return static_cast<int>(h % static_cast<unsigned int>(tamanho));
    }

    // Hash 2: DJB2 adaptado para inteiros
    int hash2(int id) const {
        unsigned int h = 5381u;
        h = ((h << 5) + h) ^ static_cast<unsigned int>(id);
        h = ((h << 5) + h) ^ static_cast<unsigned int>(id >> 8);
        return static_cast<int>(h % static_cast<unsigned int>(tamanho));
    }

    // Hash 3: FNV-1a adaptado — produz distribuição diferente das anteriores
    int hash3(int id) const {
        unsigned int h = 2166136261u;
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&id);
        for (int i = 0; i < 4; i++) {
            h ^= bytes[i];
            h *= 16777619u;
        }
        return static_cast<int>(h % static_cast<unsigned int>(tamanho));
    }

public:
    // Construtor: tamanho em bits (padrão: 10.000 bits ≈ 1,25 KB para ~3.000 itens)
    explicit BloomFilter(int tamanho_bits = 10000)
        : tamanho(tamanho_bits), num_hashes(3), total_inseridos(0)
    {
        int bytes_necessarios = (tamanho + 7) / 8;
        bits = new uint8_t[bytes_necessarios];
        std::memset(bits, 0, bytes_necessarios);
    }

    ~BloomFilter() { delete[] bits; }

    // Desabilita cópia para evitar double-free
    BloomFilter(const BloomFilter&)            = delete;
    BloomFilter& operator=(const BloomFilter&) = delete;

    // ── CRUD: Inserir ─────────────────────────────────────────────────────────
    // Complexidade: O(k) onde k = num_hashes (constante = 3)
    void adicionar(int id_sensor) {
        setBit(hash1(id_sensor));
        setBit(hash2(id_sensor));
        setBit(hash3(id_sensor));
        total_inseridos++;
    }

    // ── CRUD: Verificar (Buscar) ──────────────────────────────────────────────
    // Retorna true  → elemento PROVAVELMENTE está no conjunto (pode ser falso positivo)
    // Retorna false → elemento COM CERTEZA ABSOLUTA não está no conjunto
    // Complexidade: O(k) constante
    bool verificar(int id_sensor) const {
        return getBit(hash1(id_sensor)) &&
               getBit(hash2(id_sensor)) &&
               getBit(hash3(id_sensor));
    }

    // ── Métricas para benchmark (Critério 9) ─────────────────────────────────
    int getTotalInseridos() const { return total_inseridos; }
    int getTamanho()        const { return tamanho; }

    // Estimativa teórica da taxa de falsos positivos: (1 - e^(-k*n/m))^k
    double taxaFalsoPositivo() const {
        if (total_inseridos == 0) return 0.0;
        double exp_val = static_cast<double>(num_hashes * total_inseridos) / tamanho;
        double p = 1.0 - std::exp(-exp_val);
        double taxa = 1.0;
        for (int i = 0; i < num_hashes; i++) taxa *= p;
        return taxa;
    }

    // ── Justificativa teorica: por que NAO ha remocao (Criterio 9 do PDF) ────
    // O PDF exige justificar metricas que nao se aplicam. Um Bloom Filter classico
    // NAO suporta remocao: cada bit pode ter sido setado por MULTIPLOS elementos
    // (colisao de hash). Zerar um bit para "remover" X poderia apagar a presenca
    // de Y, gerando FALSO NEGATIVO -- o que quebra a garantia fundamental da
    // estrutura (falso negativo deve ser impossivel). Por isso, as metricas de
    // "tempo de remocao" do benchmark NAO se aplicam a esta estrutura.
    // (Solucao alternativa seria um Counting Bloom Filter, fora do escopo aqui.)
    void justificarSemRemocao() const {
        std::cout << "[BloomFilter] Remocao nao suportada por design: um bit pode\n"
                  << "  pertencer a varios elementos. Remover causaria falso negativo,\n"
                  << "  violando a garantia da estrutura. Metrica de remocao N/A.\n";
    }

    void exibirEstatisticas() const {
        std::cout << "[BloomFilter] Bits: "    << tamanho
                  << " | Inseridos: "          << total_inseridos
                  << " | Hashes: "             << num_hashes
                  << " | FP estimado: "        << taxaFalsoPositivo() * 100.0 << "%\n";
    }
};