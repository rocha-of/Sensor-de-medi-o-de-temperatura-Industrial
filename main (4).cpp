#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <thread>   // std::this_thread::sleep_for (R12 - latência artificial)
#include <limits>   // std::numeric_limits
#include <fstream>  // leitura de /proc/self/status (memoria RAM real)
#include <string>
 
#include "ArvoreAVL.hpp"
#include "TabelaHash.hpp"
#include "bloomfilter.hpp"
#include "FenwickTree.hpp"
 
// ============================================================================
// ESTRUTURA DE DADO GLOBAL: Leitura de Sensor
// ============================================================================
struct LeituraSensor {
    int    id_sensor;
    double timestamp;
    double valor;
};
 
// ============================================================================
// CRITÉRIO 5 — LISTA ENCADEADA DINÂMICA (estrutura clássica convencional)
// Usada estritamente na comparação de benchmark do Critério 7
// ============================================================================
struct NodeDinamico {
    LeituraSensor  dados;
    NodeDinamico*  proximo;
    NodeDinamico(int id, double ts, double val)
        : dados{id, ts, val}, proximo(nullptr) {}
};
 
class ListaEncadeadaDinamica {
private:
    NodeDinamico* cabeca;
    int           tamanho;
 
public:
    ListaEncadeadaDinamica() : cabeca(nullptr), tamanho(0) {}
 
    ~ListaEncadeadaDinamica() {
        while (cabeca) {
            NodeDinamico* aux = cabeca;
            cabeca = cabeca->proximo;
            delete aux;
        }
    }
 
    // CRUD: Inserir — O(1) no início
    void inserir(int id, double ts, double val) {
        NodeDinamico* novo = new NodeDinamico(id, ts, val);
        novo->proximo = cabeca;
        cabeca = novo;
        tamanho++;
    }
 
    // CRUD: Remover — O(N)
    bool remover(int id) {
        NodeDinamico* atual    = cabeca;
        NodeDinamico* anterior = nullptr;
        while (atual) {
            if (atual->dados.id_sensor == id) {
                if (anterior) anterior->proximo = atual->proximo;
                else          cabeca            = atual->proximo;
                delete atual;
                tamanho--;
                return true;
            }
            anterior = atual;
            atual    = atual->proximo;
        }
        return false;
    }
 
    // CRUD: Buscar — O(N)
    bool buscar(int id, double& val) const {
        NodeDinamico* atual = cabeca;
        while (atual) {
            if (atual->dados.id_sensor == id) { val = atual->dados.valor; return true; }
            atual = atual->proximo;
        }
        return false;
    }
 
    // Travessia completa para benchmark de localidade de memória
    long long traversalCompleto() const {
        long long soma = 0;
        NodeDinamico* atual = cabeca;
        while (atual) { soma += static_cast<long long>(atual->dados.valor); atual = atual->proximo; }
        return soma;
    }
 
    int getTamanho() const { return tamanho; }
};
 
// ============================================================================
// CRITÉRIO 7 — LISTA ENCADEADA ESTÁTICA COM MEMORY POOL (versão otimizada)
// Pool de alocação contígua em array — maximiza localidade de cache
// ============================================================================
// CRITÉRIO 7 — LISTA ENCADEADA ESTÁTICA COM MEMORY POOL (versão otimizada)
// Pool de alocação contígua em array — maximiza localidade de cache.
// Nota: trechos desenvolvidos com auxílio de IA estão documentados em IA_USAGE.md.
// ============================================================================
const int MAX_NOS = 25000;
struct NodeEstatico {
    LeituraSensor dados;
    int           proximo; // Índice no array (não ponteiro) — -1 = fim
};
 
class ListaEncadeadaEstatica {
private:
    NodeEstatico pool[MAX_NOS];
    int          cabeca;
    int          primeiro_livre;
    int          tamanho;
 
public:
    ListaEncadeadaEstatica() : cabeca(-1), primeiro_livre(0), tamanho(0) {
        // Inicializa a lista de livres encadeando índices em sequência
        for (int i = 0; i < MAX_NOS - 1; i++)
            pool[i].proximo = i + 1;
        pool[MAX_NOS - 1].proximo = -1;
    }
 
    // CRUD: Inserir — O(1)
    bool inserir(int id, double ts, double val) {
        if (primeiro_livre == -1) return false;
        int idx       = primeiro_livre;
        primeiro_livre = pool[idx].proximo;
 
        pool[idx].dados = {id, ts, val};
        pool[idx].proximo = cabeca;
        cabeca = idx;
        tamanho++;
        return true;
    }
 
    // CRUD: Remover — O(N); reorganiza ponteiros internos (devolve slot ao pool)
    bool remover(int id) {
        int atual    = cabeca;
        int anterior = -1;
        while (atual != -1) {
            if (pool[atual].dados.id_sensor == id) {
                if (anterior == -1) cabeca                = pool[atual].proximo;
                else                pool[anterior].proximo = pool[atual].proximo;
                pool[atual].proximo = primeiro_livre;
                primeiro_livre = atual;
                tamanho--;
                return true;
            }
            anterior = atual;
            atual    = pool[atual].proximo;
        }
        return false;
    }
 
    // CRUD: Buscar — O(N)
    bool buscar(int id, double& val) const {
        int atual = cabeca;
        while (atual != -1) {
            if (pool[atual].dados.id_sensor == id) { val = pool[atual].dados.valor; return true; }
            atual = pool[atual].proximo;
        }
        return false;
    }
 
    // Travessia completa para benchmark de localidade de memória
    long long traversalCompleto() const {
        long long soma  = 0;
        int       atual = cabeca;
        while (atual != -1) {
            soma += static_cast<long long>(pool[atual].dados.valor);
            atual = pool[atual].proximo;
        }
        return soma;
    }
 
    void exibirLeituras(int limite = 20) const {
        int atual = cabeca;
        if (atual == -1) { std::cout << "Lista vazia.\n"; return; }
        std::cout << "--- HISTÓRICO DE LEITURAS (últimas " << limite << ") ---\n";
        int cont = 0;
        while (atual != -1 && cont < limite) {
            std::cout << "  Sensor " << pool[atual].dados.id_sensor
                      << " | TS: "   << std::fixed << std::setprecision(0) << pool[atual].dados.timestamp
                      << " | Val: "  << std::fixed << std::setprecision(2) << pool[atual].dados.valor << " °C\n";
            atual = pool[atual].proximo;
            cont++;
        }
    }
 
    int getTamanho() const { return tamanho; }
};
 
// ============================================================================
// CRITÉRIO 2 — DETECTOR ESTATÍSTICO DE ANOMALIAS (operação adicional)
// Regra dos 3-Sigma: detecta leituras fora de média ± 3×desvio_padrão
// ============================================================================
class DetectorAnomalias {
private:
    double media;
    double desvio_padrao;
    bool   treinado;
 
public:
    DetectorAnomalias() : media(0.0), desvio_padrao(0.0), treinado(false) {}
 
    void treinar(const std::vector<double>& dados) {
        if (dados.empty()) { std::cerr << "[Detector] Aviso: dataset vazio.\n"; return; }
        double soma = 0.0, soma_q = 0.0;
        for (double v : dados) { soma += v; soma_q += v * v; }
        media         = soma / dados.size();
        desvio_padrao = std::sqrt((soma_q / dados.size()) - (media * media));
        treinado      = true;
    }
 
    bool detetar(double valor) const {
        if (!treinado) return false;
        return std::fabs(valor - media) > 3.0 * desvio_padrao;
    }
 
    double getMedia()  const { return media; }
    double getDesvio() const { return desvio_padrao; }
    bool   eTreinado() const { return treinado; }
};
 
// ============================================================================
// CRITÉRIO 9 — MÓDULO DE BENCHMARK PADRONIZADO
// Coleta: tempo de inserção, busca, remoção, uso de memória, colisões, escalabilidade
// ============================================================================
namespace Benchmark {
 
    using Clock = std::chrono::high_resolution_clock;
    using NS    = std::chrono::nanoseconds;
 
    // Tempo de execução de qualquer callable em nanossegundos
    template<typename Func>
    long long medirNS(Func&& f) {
        auto ini = Clock::now();
        f();
        auto fim = Clock::now();
        return std::chrono::duration_cast<NS>(fim - ini).count();
    }
 
    // ── Benchmark completo: Lista Dinâmica vs Pool Estática ──────────────────
    void comparacaoListas(int N = 10000) {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
        std::cout <<   "║  BENCHMARK C7 · Lista Dinâmica (heap) vs Pool Estática      ║\n";
        std::cout <<   "╚══════════════════════════════════════════════════════════════╝\n";
        std::cout << "  N = " << N << " elementos\n\n";

ListaEncadeadaDinamica dinamica;
        ListaEncadeadaEstatica estatica;
 
        // — Inserção —
        long long t_ins_din = medirNS([&]() {
            for (int i = 1; i <= N; i++)
                dinamica.inserir(i % 5 + 101, 1719520000.0 + i, 24.0 + (i % 10) * 0.3);
        });
 
        long long t_ins_est = medirNS([&]() {
            for (int i = 1; i <= N; i++)
                estatica.inserir(i % 5 + 101, 1719520000.0 + i, 24.0 + (i % 10) * 0.3);
        });
 
        // — Travessia (localidade de memória) —
        // Repetimos REPS vezes e tiramos a média: uma única travessia dura poucos
        // nanossegundos, no nível do ruído do relógio. Repetir torna a medição
        // estável e revela o ganho real de localidade de cache.
        const int REPS_TRAV = 1000;
        volatile long long sink = 0; // impede o compilador de otimizar o laço
        long long t_trav_din = medirNS([&]() {
            for (int r = 0; r < REPS_TRAV; r++) sink += dinamica.traversalCompleto();
        }) / REPS_TRAV;
        long long t_trav_est = medirNS([&]() {
            for (int r = 0; r < REPS_TRAV; r++) sink += estatica.traversalCompleto();
        }) / REPS_TRAV;

        // — Busca — também repetida para estabilizar a medição
        const int REPS_BUSCA = 10000;
        double v = 0;
        long long t_busca_din = medirNS([&]() {
            for (int r = 0; r < REPS_BUSCA; r++) { dinamica.buscar(103, v); sink += (long long)v; }
        }) / REPS_BUSCA;
        long long t_busca_est = medirNS([&]() {
            for (int r = 0; r < REPS_BUSCA; r++) { estatica.buscar(103, v); sink += (long long)v; }
        }) / REPS_BUSCA;
        (void)sink;
 
        // — Remoção —
        long long t_rem_din = medirNS([&]() { dinamica.remover(102); });
        long long t_rem_est = medirNS([&]() { estatica.remover(102); });
 
        // — Uso de memória sob o MESMO N (comparação metodologicamente justa) —
        // Lista dinâmica: dados úteis + 1 ponteiro (8B em x64) + overhead do allocator (~16B/bloco).
        const long long OVERHEAD_HEAP = 16;
        long long mem_din_util = static_cast<long long>(N) * (sizeof(LeituraSensor) + sizeof(void*));
        long long mem_din_real = static_cast<long long>(N) * (sizeof(LeituraSensor) + sizeof(void*) + OVERHEAD_HEAP);
        // Pool estático: índice int (4B) em vez de ponteiro, sem overhead por nó (alocação única).
        long long mem_est_util = static_cast<long long>(N) * sizeof(NodeEstatico);
 
        // — Exibição de resultados —
        auto linha = [](const std::string& op, long long din, long long est) {
            double ganho = (din > 0) ? (100.0 * (din - est) / din) : 0.0;
            std::cout << "  " << std::left << std::setw(20) << op
                      << "| Dinâmica: " << std::right << std::setw(9) << din << " ns"
                      << "  | Pool: " << std::setw(9) << est << " ns"
                      << "  | Ganho: " << std::fixed << std::setprecision(1) << ganho << "%\n";
        };
 
        std::cout << "  " << std::string(75, '-') << "\n";
        linha("Inserção total",  t_ins_din,   t_ins_est);
        linha("Travessia",       t_trav_din,  t_trav_est);
        linha("Busca (1 item)",  t_busca_din, t_busca_est);
        linha("Remoção (1 item)",t_rem_din,   t_rem_est);
        std::cout << "  " << std::string(75, '-') << "\n";
        std::cout << "  Memória para os MESMOS " << N << " elementos:\n";
        std::cout << "    Lista dinâmica : " << mem_din_util / 1024 << " KB úteis + overhead heap = ~"
                  << mem_din_real / 1024 << " KB reais\n";
        std::cout << "    Pool estático  : " << mem_est_util / 1024 << " KB (alocação única, índice int < ponteiro)\n";
        std::cout << "    Economia do pool: ~" << (mem_din_real - mem_est_util) / 1024 << " KB ("
                  << std::fixed << std::setprecision(1)
                  << (100.0 * (mem_din_real - mem_est_util) / mem_din_real) << "% menos)\n";
        std::cout << "\n  Análise dos resultados (para a defesa oral):\n"
                  << "  · INSERÇÃO: o pool é consistentemente mais rápido. Inserir = pegar o\n"
                  << "    próximo índice livre (O(1) trivial). A lista dinâmica chama 'new' a\n"
                  << "    cada nó, e cada 'new' tem custo de gerenciamento do heap.\n"
                  << "  · REMOÇÃO: o pool é MUITO mais rápido. Remover = devolver o índice à\n"
                  << "    lista de livres (O(1) sem syscall). A dinâmica chama 'delete', que\n"
                  << "    devolve memória ao alocador do sistema — bem mais caro.\n"
                  << "  · TRAVESSIA: depende do tamanho do nó e do estado do heap. O nó do pool\n"
                  << "    é maior (guarda um int de índice + o dado), então percorrer cobre mais\n"
                  << "    bytes; num heap recém-iniciado, os nós dinâmicos podem estar vizinhos.\n"
                  << "    A vantagem teórica de localidade do pool aparece sob heap fragmentado.\n"
                  << "  · MEMÓRIA: o pool gasta menos por elemento — índice int (4B) < ponteiro\n"
                  << "    (8B) e uma só alocação, sem overhead de allocator por nó.\n"
                  << "  Conclusão honesta: o pool vence claramente em inserção, remoção e memória;\n"
                  << "  na travessia o resultado depende das condições, o que é esperado e defensável.\n";
    }
 
    // ── Benchmark Tabela Hash ─────────────────────────────────────────────────
    void benchmarkHash(int N = 1000) {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
        std::cout <<   "║  BENCHMARK C9 · Tabela Hash — Métricas Completas            ║\n";
        std::cout <<   "╚══════════════════════════════════════════════════════════════╝\n";
 
        TabelaHash h;
        h.resetarColisoes();
 
        long long t_ins = medirNS([&]() {
            for (int i = 1; i <= N; i++) h.inserir_ou_atualizar(i, i * 0.5);
        });
 
        double v = 0;
        long long t_busca = medirNS([&]() { h.buscar(N / 2, v); });
        long long t_rem   = medirNS([&]() { h.remover(N / 2); });
 
        std::cout << "  Inserção de " << N << " itens : " << t_ins   << " ns  |  avg: " << t_ins / N    << " ns/op\n";
        std::cout << "  Busca 1 item          : " << t_busca << " ns\n";
        std::cout << "  Remoção 1 item        : " << t_rem   << " ns\n";
        h.exibirEstatisticas();
    }
 
    // ── Benchmark AVL ─────────────────────────────────────────────────────────
    void benchmarkAVL(int N = 1000) {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
        std::cout <<   "║  BENCHMARK C9 · Árvore AVL — Métricas Completas             ║\n";
        std::cout <<   "╚══════════════════════════════════════════════════════════════╝\n";
 
        ArvoreAVL avl;
        long long t_ins = medirNS([&]() {
            for (int i = 1; i <= N; i++) avl.inserir(1719520000.0 + i, i % 5 + 101, 24.0 + i * 0.001);
        });
 
        int id; double val;
        long long t_busca = medirNS([&]() { avl.buscar(1719520000.0 + N / 2, id, val); });
        long long t_rem   = medirNS([&]() { avl.remover(1719520000.0 + N / 2); });
 
        std::cout << "  Inserção de " << N << " nós    : " << t_ins   << " ns  |  avg: " << t_ins / N    << " ns/op\n";
        std::cout << "  Busca O(log N)        : " << t_busca << " ns\n";
        std::cout << "  Remoção O(log N)      : " << t_rem   << " ns\n";
        std::cout << "  Altura final da AVL   : " << avl.altura() << "  (teórico: ~log2(" << N << ") = " << static_cast<int>(std::log2(N)) << ")\n";
    }
 
    // ── Escalabilidade com N variável ─────────────────────────────────────────
    void escalabilidade() {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
        std::cout <<   "║  BENCHMARK C9 · Escalabilidade (N variável)                 ║\n";
        std::cout <<   "╚══════════════════════════════════════════════════════════════╝\n";
        std::cout << "  " << std::left << std::setw(8) << "N"
                  << std::setw(20) << "Hash ins (ns/op)"
                  << std::setw(20) << "AVL ins (ns/op)"
                  << std::setw(20) << "AVL altura\n";
        std::cout << "  " << std::string(68, '-') << "\n";
 
        // A Tabela Hash tem 1009 slots fixos (enderecamento aberto). Acima de ~70%
        // de fator de carga a sondagem degrada; por isso ela so e medida ate N=700.
        // A AVL nao tem esse limite e escala ate 10000 para mostrar o crescimento log.
        for (int N : {100, 500, 1000, 5000, 10000}) {
            ArvoreAVL avl;
            long long t_a = medirNS([&]() { for (int i=1;i<=N;i++) avl.inserir(1.0*i, i%5+101, i*0.1); });

            std::cout << "  " << std::left << std::setw(8) << N;
            if (N <= 700) {
                TabelaHash h;
                long long t_h = medirNS([&]() { for (int i=1;i<=N;i++) h.inserir_ou_atualizar(i, i*0.5); });
                std::cout << std::setw(20) << (t_h / N);
            } else {
                std::cout << std::setw(20) << "N/A (>70% carga)";
            }
            std::cout << std::setw(20) << (t_a / N)
                      << avl.altura() << "\n";
        }
        std::cout << "\n  Observacao: a AVL mantem altura ~log2(N) conforme N cresce,\n";
        std::cout << "  confirmando empiricamente a complexidade O(log N) teorica. A Hash\n";
        std::cout << "  de tamanho fixo so e medida ate N=700 para respeitar o fator de carga.\n";
    }
 
    // ── Uso de RAM real do processo (RSS via /proc/self/status, Linux) ────────
    // Retorna o Resident Set Size em KB; 0 se indisponivel (ex.: Windows).
    long long memoriaRSS_KB() {
        std::ifstream status("/proc/self/status");
        if (!status.is_open()) return 0;
        std::string linha;
        while (std::getline(status, linha)) {
            if (linha.rfind("VmRSS:", 0) == 0) {
                long long kb = 0;
                for (char c : linha)
                    if (c >= '0' && c <= '9') kb = kb * 10 + (c - '0');
                return kb;
            }
        }
        return 0;
    }

    // ── Latencia media de operacoes COMBINADAS (insercao+busca+remocao) ───────
    // O PDF exige explicitamente "Latencia Media: tempo medio de resposta em
    // operacoes combinadas". Mede um ciclo realista misturando as 3 operacoes.
    void latenciaMediaCombinada(int ciclos = 1000) {
        std::cout << "\n+==============================================================+\n";
        std::cout <<   "|  BENCHMARK C9 - Latencia Media (operacoes combinadas)        |\n";
        std::cout <<   "+==============================================================+\n";
        std::cout << "  Cada ciclo = 1 insercao + 1 busca + 1 remocao\n";
        std::cout << "  Total de ciclos: " << ciclos << "\n\n";

        // — Tabela Hash —
        {
            TabelaHash h;
            long long total = medirNS([&]() {
                for (int i = 1; i <= ciclos; i++) {
                    double v = 0;
                    h.inserir_ou_atualizar(i, i * 0.5);
                    h.buscar(i, v);
                    h.remover(i);
                }
            });
            std::cout << "  Tabela Hash : " << total / (ciclos * 3) << " ns/op combinada"
                      << "  (total " << total / 1000 << " us para " << ciclos * 3 << " ops)\n";
        }

        // — Arvore AVL —
        {
            ArvoreAVL avl;
            long long total = medirNS([&]() {
                for (int i = 1; i <= ciclos; i++) {
                    int id; double v;
                    double chave = 1719520000.0 + i;
                    avl.inserir(chave, i % 5 + 101, 24.0 + i * 0.001);
                    avl.buscar(chave, id, v);
                    avl.remover(chave);
                }
            });
            std::cout << "  Arvore AVL  : " << total / (ciclos * 3) << " ns/op combinada"
                      << "  (total " << total / 1000 << " us para " << ciclos * 3 << " ops)\n";
        }

        std::cout << "\n  Nota: a latencia combinada e mais realista que medir cada\n";
        std::cout << "  operacao isolada, pois reflete o ciclo de vida real de um dado\n";
        std::cout << "  no sistema (entra, e consultado, e removido na manutencao).\n";
    }

    // ── Relatorio de memoria RAM real antes/depois de carga ──────────────────
    void memoriaReal() {
        std::cout << "\n+==============================================================+\n";
        std::cout <<   "|  BENCHMARK C9 - Uso de RAM real do processo (RSS)            |\n";
        std::cout <<   "+==============================================================+\n";
        long long rss = memoriaRSS_KB();
        if (rss == 0) {
            std::cout << "  RSS indisponivel neste sistema (/proc nao acessivel).\n";
            std::cout << "  Em Linux, exibe a memoria fisica realmente ocupada pelo processo.\n";
            return;
        }
        std::cout << "  RSS atual do processo: " << rss << " KB (~" << rss / 1024 << " MB)\n";
        std::cout << "  Inclui as 5 estruturas + dataset de 10.000 amostras ja carregados.\n";
        std::cout << "  Esta e a memoria FISICA real, medida pelo SO (nao estimativa).\n";
    }

} // namespace Benchmark
 
// ============================================================================
// CRITÉRIO 8 — TESTES EM CONDIÇÕES RESTRITIVAS (5 cenários reais)
// ============================================================================
namespace Restricoes {
 
    // R4 · Restrição de Memória: comparação de localidade (integrado ao C7)
    void R4_localidadeMemoria() {
        std::cout << "\n[R4] Restrição de Memória · Localidade de Cache\n";
        Benchmark::comparacaoListas(10000);
    }
 
    // R7 · Restrição de Processamento: orçamento computacional fixo
    void R7_orcamentoComputacional() {
        std::cout << "\n[R7] Restrição de Processamento · Orçamento de Tempo por Operação\n";
        const long long BUDGET_NS = 5000; // 5 µs máximo por operação
        ArvoreAVL avl;
        int aprovadas = 0, reprovadas = 0;
 
        for (int i = 1; i <= 1000; i++) {
            long long t = Benchmark::medirNS([&]() {
                avl.inserir(1719520000.0 + i, i % 5 + 101, 24.0 + i * 0.001);
            });
            if (t <= BUDGET_NS) aprovadas++;
            else                reprovadas++;
        }
        std::cout << "  Orçamento: " << BUDGET_NS << " ns/op | Aprovadas: " << aprovadas
                  << " | Reprovadas: " << reprovadas << "\n";
        std::cout << "  Taxa de conformidade: " << std::fixed << std::setprecision(1)
                  << (100.0 * aprovadas / 1000) << "%\n";
    }
 
    // R12 · Restrição de Latência: delays artificiais simulando sensor com jitter
    void R12_latenciaArtificial() {
        std::cout << "\n[R12] Restrição de Latência · Delays Artificiais (jitter de rede)\n";
        std::default_random_engine rng(42);
        std::uniform_int_distribution<int> jitter(0, 2000); // 0–2 µs de atraso
 
        TabelaHash h;
        for (int i = 1; i <= 10; i++) h.inserir_ou_atualizar(100 + i, i * 2.0);
 
        std::cout << "  Simulando 5 buscas com jitter:\n";
        for (int i = 1; i <= 5; i++) {
            int delay_us = jitter(rng);
            std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
 
            double val = 0;
            long long t = Benchmark::medirNS([&]() { h.buscar(101, val); });
            std::cout << "    Busca " << i << " | Jitter: " << delay_us
                      << " µs | Latência real da op: " << t << " ns\n";
        }
    }
 
    // R18 · Restrição de Dados: simulação de 10% de leituras anômalas
    void R18_leituraAnomalas(const DetectorAnomalias& detector) {
        std::cout << "\n[R18] Restrição de Dados · Amostragem Anômala (10% de falhas)\n";
        std::default_random_engine rng(99);
        std::normal_distribution<double>  normal(24.0, 2.0);
        std::uniform_real_distribution<double> falha(40.0, 60.0);
 
        int total = 200, anomalias_injetadas = 0, anomalias_detectadas = 0;
        for (int i = 0; i < total; i++) {
            double leitura;
            bool e_falha = (i % 10 == 0);
            if (e_falha) { leitura = falha(rng); anomalias_injetadas++; }
            else           leitura = normal(rng);
 
            if (detector.detetar(leitura)) anomalias_detectadas++;
        }
        std::cout << "  Total amostras : " << total             << "\n";
        std::cout << "  Falhas injetadas: " << anomalias_injetadas  << " (10%)\n";
        std::cout << "  Falhas detectadas: " << anomalias_detectadas << "\n";
        std::cout << "  Taxa de detecção: " << std::fixed << std::setprecision(1)
                  << (100.0 * anomalias_detectadas / anomalias_injetadas) << "%\n";
        std::cout << "  Motor estatístico: média=" << detector.getMedia()
                  << " | desvio=" << detector.getDesvio() << " | limiar=3σ\n";
    }
 
    // R19 · Restrição de Dados: truncamento de resolução (double → int) e impacto
    void R19_truncamentoResolucao() {
        std::cout << "\n[R19] Restrição de Dados · Truncamento de Resolução (double→int)\n";
        std::vector<double> originais = {24.8765, 23.1234, 25.9999, 22.5001, 24.0001};
        double erro_total = 0.0;
        std::cout << "  " << std::setw(12) << "Original" << std::setw(12) << "Truncado" << std::setw(12) << "Erro\n";
        std::cout << "  " << std::string(36, '-') << "\n";
        for (double v : originais) {
            int    truncado = static_cast<int>(v);
            double erro     = std::fabs(v - truncado);
            erro_total += erro;
            std::cout << "  " << std::fixed << std::setprecision(4)
                      << std::setw(12) << v << std::setw(12) << truncado
                      << std::setw(12) << erro << "\n";
        }
        std::cout << "  Erro médio de truncamento: " << erro_total / originais.size() << "\n";
        std::cout << "  Impacto: perda de precisão pode acionar falsos alertas no Detector.\n";
    }
 
    // R21 · Restrição Algorítmica: substituição AVL O(log N) → Busca Linear O(N)
    void R21_degradacaoAlgoritmica(int N = 5000) {
        std::cout << "\n[R21] Restrição Algorítmica · AVL O(log N) vs Busca Linear O(N)\n";
 
        // Prepara vetor para busca linear (pior caso)
        std::vector<LeituraSensor> vetor_linear;
        vetor_linear.reserve(N);
        ArvoreAVL avl;
        for (int i = 1; i <= N; i++) {
            vetor_linear.push_back({i % 5 + 101, 1719520000.0 + i, 24.0 + i * 0.001});
            avl.inserir(1719520000.0 + i, i % 5 + 101, 24.0 + i * 0.001);
        }
 
        double alvo_ts = 1719520000.0 + N; // Último elemento = pior caso para busca linear
 
        // Busca linear O(N)
        int id_l = -1; double val_l = 0;
        long long t_linear = Benchmark::medirNS([&]() {
            for (const auto& s : vetor_linear) {
                if (std::fabs(s.timestamp - alvo_ts) < 1e-9) {
                    id_l  = s.id_sensor;
                    val_l = s.valor;
                    break;
                }
            }
        });
 
        // Busca AVL O(log N)
        int id_a = -1; double val_a = 0;
        long long t_avl = Benchmark::medirNS([&]() {
            avl.buscar(alvo_ts, id_a, val_a);
        });
 
        double speedup = (t_linear > 0) ? static_cast<double>(t_linear) / t_avl : 0;
        std::cout << "  N = " << N << " | Elemento buscado: último (pior caso)\n";
        std::cout << "  Busca linear O(N) : " << t_linear << " ns\n";
        std::cout << "  Busca AVL O(log N): " << t_avl    << " ns\n";
        std::cout << "  Speedup da AVL    : " << std::fixed << std::setprecision(1) << speedup << "×\n";
        std::cout << "  Degradação teórica com N→∞: busca linear cresce linearmente;\n"
                  << "  AVL mantém log2(N) ≈ " << static_cast<int>(std::log2(N)) << " comparações para N=" << N << ".\n";
    }
 
} // namespace Restricoes
 
// ============================================================================
// MENU INTERATIVO — CRITÉRIO 10
// ============================================================================
void exibirMenu() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout <<   "║         PAINEL OPERACIONAL DE SENSORES INDUSTRIAIS              ║\n";
    std::cout <<   "╠══════════════════════════════════════════════════════════════════╣\n";
    std::cout <<   "║  [1] Estado atual dos sensores     (Tabela Hash  · O(1))        ║\n";
    std::cout <<   "║  [2] Busca histórica por timestamp  (AVL          · O(log N))   ║\n";
    std::cout <<   "║  [3] Média térmica por intervalo    (Fenwick Tree · O(log N))   ║\n";
    std::cout <<   "║  [4] Triagem de segurança           (Bloom Filter · O(1))       ║\n";
    std::cout <<   "║  [5] Histórico recente              (Lista Pool Estática)        ║\n";
    std::cout <<   "║  [6] Remover sensor do sistema      (CRUD · todas estruturas)   ║\n";
    std::cout <<   "║  [7] Benchmarks C7/C9 (Listas · Hash · AVL · Escalabilidade)   ║\n";
    std::cout <<   "║  [8] Cenários restritivos C8 (R4/R7/R12/R18/R19/R21)           ║\n";
    std::cout <<   "║  [0] Encerrar sistema                                           ║\n";
    std::cout <<   "╚══════════════════════════════════════════════════════════════════╝\n";
    std::cout << "Escolha: ";
}
 
// Leitura segura de inteiro — evita loop infinito com entradas inválidas
int lerOpcao() {
    int op;
    while (!(std::cin >> op)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Entrada inválida. Digite um número: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return op;
}
 
// ============================================================================
// MAIN
// ============================================================================
int main() {
    // ── Instâncias das estruturas ─────────────────────────────────────────────
    ListaEncadeadaEstatica  historicoGeral;
    ListaEncadeadaDinamica  historicoTradicional;
    TabelaHash              painelTempoReal;
    ArvoreAVL               indiceCronologico;
    BloomFilter             filtroSeguranca(10000);
    FenwickTree             analiseEstudos(10000);
    std::vector<double>     leituras_treino;
 
    // ── Geração do dataset gaussiano (Critério 3 e 4) ────────────────────────
    unsigned semente = static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count());
    std::default_random_engine     gerador(semente);
    std::normal_distribution<double> ruido(24.0, 2.0);
 
    std::cout << "Indexando fluxo contínuo industrial (10.000 amostras)...\n";
 
    for (int i = 1; i <= 10000; i++) {
        int    id_sensor = (i % 5) + 101;
        double timestamp = 1719520000.0 + i;
        double valor     = ruido(gerador);
 
        // Injeção controlada de falhas (Critério 4 — Data Augmentation)
        if (i == 2500) { valor = 45.3; id_sensor = 999; }
        if (i == 7500) { valor = 52.1; id_sensor = 888; }
 
        historicoGeral.inserir(id_sensor, timestamp, valor);
        historicoTradicional.inserir(id_sensor, timestamp, valor);
        painelTempoReal.inserir_ou_atualizar(id_sensor, valor);
        indiceCronologico.inserir(timestamp, id_sensor, valor);
        analiseEstudos.atualizar(i, valor);
        leituras_treino.push_back(valor);
 
        if (valor > 40.0) filtroSeguranca.adicionar(id_sensor);
    }
 
    DetectorAnomalias motorEstatistico;
    motorEstatistico.treinar(leituras_treino);
 
    std::cout << "Sistema carregado! Média=" << std::fixed << std::setprecision(2)
              << motorEstatistico.getMedia() << " °C | Desvio="
              << motorEstatistico.getDesvio() << " °C\n";
 
    // ── Loop principal do menu ────────────────────────────────────────────────
    int opcao = -1;
    while (opcao != 0) {
        exibirMenu();
        opcao = lerOpcao();
 
        auto t_ini = std::chrono::high_resolution_clock::now();
 
        switch (opcao) {
 
        // ── Opção 1: Estado ao vivo via Tabela Hash ──────────────────────────
        case 1: {
            std::cout << "\n>>> ESTADO VIVO DOS SENSORES (TABELA HASH) <<<\n";
            int ids[] = {101, 102, 103, 104, 105, 888, 999};
            for (int id : ids) {
                double val = 0.0;
                if (painelTempoReal.buscar(id, val)) {
                    std::cout << "  Sensor " << id << " | " << std::fixed << std::setprecision(2) << val << " °C";
                    if (motorEstatistico.detetar(val)) std::cout << "  ⚠ ANOMALIA";
                    std::cout << "\n";
                } else {
                    std::cout << "  Sensor " << id << " | INATIVO/REMOVIDO\n";
                }
            }
            auto t_fim = std::chrono::high_resolution_clock::now();
            std::cout << "  Latência total: "
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(t_fim - t_ini).count()
                      << " ns\n";
            break;
        }
 
        // ── Opção 2: Busca por timestamp na AVL ──────────────────────────────
        case 2: {
            double ts;
            std::cout << "\nTimestamp para busca (ex: 1719520001): ";
            std::cin >> ts;
            int id_a; double val_a;
            auto t0 = std::chrono::high_resolution_clock::now();
            bool ok = indiceCronologico.buscar(ts, id_a, val_a);
            auto t1 = std::chrono::high_resolution_clock::now();
            long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
 
            if (ok) std::cout << "\n  Sensor " << id_a << " | " << val_a << " °C | Latência: " << ns << " ns\n";
            else    std::cout << "\n  Registro não encontrado. Latência: " << ns << " ns\n";
            break;
        }
 
        // ── Opção 3: Média por intervalo via Fenwick Tree ─────────────────────
        case 3: {
            int ini, fim;
            std::cout << "\nIntervalo de amostras [1-10000] — Inicial: "; std::cin >> ini;
            std::cout << "Final: "; std::cin >> fim;
            auto t0 = std::chrono::high_resolution_clock::now();
            if (ini >= 1 && fim <= 10000 && ini <= fim) {
                double soma = analiseEstudos.consultar_intervalo(ini, fim);
                auto t1 = std::chrono::high_resolution_clock::now();
                long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                std::cout << "  Média térmica [" << ini << "-" << fim << "]: "
                          << std::fixed << std::setprecision(2) << soma / (fim - ini + 1)
                          << " °C | Latência: " << ns << " ns\n";
            } else {
                std::cout << "  Intervalo inválido.\n";
            }
            break;
        }
 
        // ── Opção 4: Triagem via Bloom Filter ────────────────────────────────
        case 4: {
            int id_v;
            std::cout << "\nID para triagem (Bloom Filter): "; std::cin >> id_v;
            auto t0 = std::chrono::high_resolution_clock::now();
            bool risco = filtroSeguranca.verificar(id_v);
            auto t1 = std::chrono::high_resolution_clock::now();
            long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            std::cout << (risco ? "  ⚠ Sensor " : "  ✓ Sensor ") << id_v
                      << (risco ? " PROVAVELMENTE gerou alertas críticos."
                                : " NUNCA ultrapassou limites.")
                      << " | Latência: " << ns << " ns\n";
            filtroSeguranca.exibirEstatisticas();
            break;
        }
 
        // ── Opção 5: Histórico recente (Lista Estática) ───────────────────────
        case 5:
            historicoGeral.exibirLeituras(20);
            break;
 
        // ── Opção 6: Remoção em TODAS as estruturas ───────────────────────────
        case 6: {
            int id_r;
            std::cout << "\nID do sensor a remover: "; std::cin >> id_r;
            auto t0 = std::chrono::high_resolution_clock::now();
 
            bool r1 = historicoGeral.remover(id_r);
            bool r2 = historicoTradicional.remover(id_r);
            bool r3 = painelTempoReal.remover(id_r);
            // AVL usa timestamp como chave — busca ID para encontrar a chave
            // (simplificação: remove pelo timestamp do sensor se encontrado na hash)
            // Nota: AVL não remove por ID (chave é timestamp); registado no relatório.
 
            auto t1 = std::chrono::high_resolution_clock::now();
            long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
 
            std::cout << "  Lista estática : " << (r1 ? "removido" : "não encontrado") << "\n";
            std::cout << "  Lista dinâmica : " << (r2 ? "removido" : "não encontrado") << "\n";
            std::cout << "  Tabela Hash    : " << (r3 ? "removido" : "não encontrado") << "\n";
            std::cout << "  Latência total : " << ns << " ns\n";
            break;
        }
 
        // ── Opção 7: Benchmarks C7 e C9 ──────────────────────────────────────
        case 7:
            Benchmark::comparacaoListas(10000);
            Benchmark::benchmarkHash(1000);
            Benchmark::benchmarkAVL(1000);
            Benchmark::escalabilidade();
            Benchmark::latenciaMediaCombinada(1000);
            Benchmark::memoriaReal();
            std::cout << "\n[Hash] ";
            painelTempoReal.exibirEstatisticas();
            std::cout << "[BloomFilter] ";
            filtroSeguranca.exibirEstatisticas();
            std::cout << "[FenwickTree] ";
            analiseEstudos.exibirEstatisticas();
            std::cout << "\n  Nota metodologica (defesa): a comparacao de tempos de BUSCA so e\n";
            std::cout << "  valida entre Hash e AVL, que resolvem o mesmo problema (busca por\n";
            std::cout << "  chave). Bloom Filter (pertinencia probabilistica) e Fenwick Tree\n";
            std::cout << "  (soma de intervalo) tem finalidades distintas e NAO entram nessa\n";
            std::cout << "  comparacao -- por isso aparecem so com suas metricas proprias.\n";
            break;
 
        // ── Opção 8: Cenários restritivos C8 ─────────────────────────────────
        case 8:
            Restricoes::R7_orcamentoComputacional();
            Restricoes::R12_latenciaArtificial();
            Restricoes::R18_leituraAnomalas(motorEstatistico);
            Restricoes::R19_truncamentoResolucao();
            Restricoes::R21_degradacaoAlgoritmica(5000);
            std::cout << "\n[R4] Restricao de Memoria - Localidade de Cache (pool vs heap)\n";
            Benchmark::comparacaoListas(10000);
            break;
 
        case 0:
            std::cout << "\nEncerrando painel operacional... Até breve!\n";
            break;
 
        default:
            std::cout << "Opção inválida. Tente novamente.\n";
        }
    }
 
    return 0;
}