// eight_puzzle_complete_fixed.cpp
// Version corregida y lista para Visual Studio / MinGW
// Comentarios en espanol (sin acentos)
// Compilar: cl /EHsc /std:c++17 eight_puzzle_complete_fixed.cpp
// o con g++: g++ -std=c++17 -O2 -o eight_puzzle_complete_fixed eight_puzzle_complete_fixed.cpp

#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <stack>
#include <list>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <ctime>
#include <string>
#include <map>
#include <set>

using namespace std;

/* CONSTANTES Y TIPOS */
using Board = array<int, 9>;
static const Board GOAL_BOARD = {1, 2, 3, 4, 5, 6, 7, 8, 0};

/* ---------------------------
   ESTRUCTURAS DE DATOS
   ---------------------------*/

/* Lista enlazada personalizada para historial de movimientos */
template<typename T>
class LinkedList {
private:
    struct Node {
        T data;
        unique_ptr<Node> next;
        Node(T val) : data(val), next(nullptr) {}
    };

    unique_ptr<Node> head;
    size_t size_;

public:
    LinkedList() : head(nullptr), size_(0) {}

    void push_front(T value) {
        auto new_node = make_unique<Node>(value);
        new_node->next = move(head);
        head = move(new_node);
        ++size_;
    }

    void clear() {
        head.reset();
        size_ = 0;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    // Iterador basico (no const)
    class Iterator {
    private:
        Node* current;
    public:
        Iterator(Node* node) : current(node) {}
        T& operator*() { return current->data; }
        Iterator& operator++() {
            if (current) current = current->next.get();
            return *this;
        }
        bool operator!=(const Iterator& other) const { return current != other.current; }
    };

    Iterator begin() { return Iterator(head.get()); }
    Iterator end() { return Iterator(nullptr); }
};

/* Arbol binario simple para demostrar uso de arboles */
template<typename T>
class BinaryTree {
private:
    struct TreeNode {
        T data;
        unique_ptr<TreeNode> left;
        unique_ptr<TreeNode> right;
        TreeNode(T val) : data(val), left(nullptr), right(nullptr) {}
    };

    unique_ptr<TreeNode> root;

public:
    BinaryTree() : root(nullptr) {}

    // Inserta en primer lugar disponible por BFS (completa a la izquierda)
    void insert(T value) {
        if (!root) {
            root = make_unique<TreeNode>(value);
            return;
        }
        queue<TreeNode*> q;
        q.push(root.get());
        while (!q.empty()) {
            TreeNode* cur = q.front(); q.pop();
            if (!cur->left) {
                cur->left = make_unique<TreeNode>(value);
                return;
            } else q.push(cur->left.get());
            if (!cur->right) {
                cur->right = make_unique<TreeNode>(value);
                return;
            } else q.push(cur->right.get());
        }
    }
};

/* ---------------------------
   ENTIDADES BASE (POO)
   ---------------------------*/

class GameEntity {
protected:
    string name;

public:
    GameEntity(const string& n) : name(n) {}
    virtual ~GameEntity() = default;

    // Metodos que las entidades pueden sobreescribir
    virtual void initialize() = 0;
    virtual void update() = 0;
    virtual void render() = 0;

    string getName() const { return name; }
};

/* ---------------------------
   CLASE PUZZLE (Hereda GameEntity)
   ---------------------------*/

class Puzzle : public GameEntity {
private:
    Board board;
    LinkedList<string> moveHistory; // historial de movimientos personalizado

    inline pair<int, int> idxToRC(int idx) const { return {idx / 3, idx % 3}; }
    inline int rcToIdx(int r, int c) const { return r * 3 + c; }

public:
    Puzzle() : GameEntity("PuzzleBoard"), board(GOAL_BOARD) {}
    Puzzle(const Board& b) : GameEntity("PuzzleBoard"), board(b) {}

    // Implementacion de los metodos abstractos
    void initialize() override {
        board = GOAL_BOARD;
        moveHistory.clear();
    }
    void update() override { /* logica si se necesitara */ }
    void render() override { print(); }

    Board getBoard() const { return board; }
    void setBoard(const Board& b) { board = b; }

    // Imprime el tablero en formato consola
    void print() const {
        cout << "+---+---+---+\n";
        for (int r = 0; r < 3; ++r) {
            cout << "| ";
            for (int c = 0; c < 3; ++c) {
                int v = board[rcToIdx(r, c)];
                if (v == 0) cout << "_";
                else cout << v;
                cout << " | ";
            }
            cout << "\n+---+---+---+\n";
        }
    }

    // Encuentra indice del hueco (0)
    int findZero() const {
        for (int i = 0; i < 9; ++i) if (board[i] == 0) return i;
        return -1;
    }

    // Mueve la ficha en srcIdx si es adyacente al hueco.
    // Registra en historial la ficha movida (valor antes del swap).
    bool moveTileAt(int srcIdx) {
        int z = findZero();
        if (srcIdx < 0 || srcIdx > 8 || z < 0) return false;
        auto pr = idxToRC(z);
        auto ps = idxToRC(srcIdx);
        int dist = abs(pr.first - ps.first) + abs(pr.second - ps.second);
        if (dist == 1) {
            int tile = board[srcIdx];            // guardar ficha antes del swap
            swap(board[z], board[srcIdx]);
            moveHistory.push_front("Movimiento ficha: " + to_string(tile));
            return true;
        }
        return false;
    }

    // Mueve por direccion respecto del hueco: 'u','d','l','r'
    bool moveDir(char dir) {
        int z = findZero();
        if (z < 0) return false;
        auto rc = idxToRC(z);
        int nr = rc.first, nc = rc.second;
        if (dir == 'u') nr = rc.first - 1;
        else if (dir == 'd') nr = rc.first + 1;
        else if (dir == 'l') nc = rc.second - 1;
        else if (dir == 'r') nc = rc.second + 1;
        else return false;
        if (nr < 0 || nr > 2 || nc < 0 || nc > 2) return false;
        int tile = board[rcToIdx(nr, nc)];
        swap(board[z], board[rcToIdx(nr, nc)]);
        moveHistory.push_front(string("Movimiento: ") + dir + " ficha:" + to_string(tile));
        return true;
    }

    // Mezcla el tablero desde la meta aplicando movimientos aleatorios validos
    void shuffleRandom(int steps = 50) {
        board = GOAL_BOARD;
        moveHistory.clear();
        random_device rd;
        mt19937 gen(rd());
        vector<char> dirs = {'u','d','l','r'};
        for (int i = 0; i < steps; ++i) {
            vector<char> poss;
            for (char d : dirs) {
                Puzzle tmp = *this;
                if (tmp.moveDir(d)) poss.push_back(d);
            }
            if (!poss.empty()) {
                uniform_int_distribution<int> dis(0, (int)poss.size() - 1);
                moveDir(poss[dis(gen)]);
            }
        }
    }

    // Heuristica Manhattan respecto a un puzzle meta
    int manhattan(const Puzzle& goal) const {
        Board g = goal.getBoard();
        array<int,9> posGoal;
        for (int i = 0; i < 9; ++i) posGoal[g[i]] = i;
        int sum = 0;
        for (int i = 0; i < 9; ++i) {
            int v = board[i];
            if (v == 0) continue;
            int gi = posGoal[v];
            auto a = idxToRC(i);
            auto b = idxToRC(gi);
            sum += abs(a.first - b.first) + abs(a.second - b.second);
        }
        return sum;
    }

    // Hamming: piezas mal colocadas (excluye hueco)
    int hamming(const Puzzle& goal) const {
        Board g = goal.getBoard();
        int cnt = 0;
        for (int i = 0; i < 9; ++i) if (board[i] != 0 && board[i] != g[i]) ++cnt;
        return cnt;
    }

    bool isGoal(const Puzzle& goal) const { return board == goal.getBoard(); }

    // Comprueba solvabilidad (inversions) para 3x3
    bool isSolvable() const {
        vector<int> v;
        for (int i = 0; i < 9; ++i) if (board[i] != 0) v.push_back(board[i]);
        int inv = 0;
        for (size_t i = 0; i < v.size(); ++i) for (size_t j = i + 1; j < v.size(); ++j) if (v[i] > v[j]) ++inv;
        return (inv % 2) == 0;
    }

    // Genera sucesores (estados alcanzables en un movimiento)
    vector<Puzzle> successors() const {
        vector<Puzzle> out;
        int z = findZero();
        auto rc = idxToRC(z);
        const array<int,4> dr = {-1,1,0,0};
        const array<int,4> dc = {0,0,-1,1};
        for (int k = 0; k < 4; ++k) {
            int nr = rc.first + dr[k], nc = rc.second + dc[k];
            if (nr < 0 || nr > 2 || nc < 0 || nc > 2) continue;
            Puzzle p = *this;
            swap(p.board[z], p.board[rcToIdx(nr,nc)]);
            out.push_back(p);
        }
        return out;
    }

    // Serializacion legible (uso '0'..'8' como caracteres)
    string serialize() const {
        string s; s.reserve(9);
        for (int i = 0; i < 9; ++i) s.push_back(char('0' + board[i]));
        return s;
    }

    // Muestra historial de movimientos (mas reciente primero)
    void showMoveHistory() {
        cout << "Historial de movimientos (" << moveHistory.size() << "):\n";
        for (auto it = moveHistory.begin(); it != moveHistory.end(); ++it) {
            cout << " - " << *it << "\n";
        }
    }
};

/* ---------------------------
   SISTEMA DE SONIDO (esqueleto)
   ---------------------------*/

class SoundSystem {
private:
    bool enabled;
public:
    SoundSystem() : enabled(false) {}
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    bool isEnabled() const { return enabled; }
    void playMoveSound() { if (enabled) { /* reproducir sonido con lib externa */ } }
    void playWinSound() { if (enabled) { /* reproducir sonido con lib externa */ } }
    void playErrorSound() { if (enabled) { /* reproducir sonido con lib externa */ } }
};

/* ---------------------------
   A* (Nodo y Solver)
   ---------------------------*/

struct Node {
    Puzzle state;
    int g;
    int f;
    shared_ptr<Node> parent;
    Node(const Puzzle& s, int g_, int f_, shared_ptr<Node> p) : state(s), g(g_), f(f_), parent(p) {}
};

struct NodeCmp {
    bool operator()(const shared_ptr<Node>& a, const shared_ptr<Node>& b) const {
        if (a->f == b->f) return a->g < b->g;
        return a->f > b->f;
    }
};

class AStarSolver {
private:
    Puzzle start;
    Puzzle goal;
    size_t maxExpansions;
    size_t maxOpenSize;

public:
    AStarSolver(const Puzzle& s, const Puzzle& g, size_t maxExp = 200000, size_t maxOpen = 500000)
        : start(s), goal(g), maxExpansions(maxExp), maxOpenSize(maxOpen) {}

    pair<bool, vector<Puzzle>> solve() {
        priority_queue<shared_ptr<Node>, vector<shared_ptr<Node>>, NodeCmp> open;
        unordered_map<string,int> closed;

        int h0 = start.manhattan(goal);
        open.push(make_shared<Node>(start, 0, h0, nullptr));
        size_t expansions = 0;

        while (!open.empty()) {
            if (expansions > maxExpansions) break;
            auto cur = open.top(); open.pop();
            ++expansions;

            string key = cur->state.serialize();
            auto it = closed.find(key);
            if (it != closed.end() && it->second <= cur->g) continue;
            closed[key] = cur->g;

            if (cur->state.isGoal(goal)) {
                vector<Puzzle> path;
                auto itn = cur;
                while (itn) {
                    path.push_back(itn->state);
                    itn = itn->parent;
                }
                reverse(path.begin(), path.end());
                return {true, path};
            }

            auto succs = cur->state.successors();
            for (auto& s : succs) {
                string sk = s.serialize();
                int ng = cur->g + 1;
                auto it2 = closed.find(sk);
                if (it2 != closed.end() && it2->second <= ng) continue;
                int h = s.manhattan(goal);
                int nf = ng + h;
                open.push(make_shared<Node>(s, ng, nf, cur));
                if (open.size() > maxOpenSize) break;
            }
        }
        return {false, {}};
    }
};

/* ---------------------------
   ScoreManager (archivo de puntajes)
   ---------------------------*/

class ScoreManager {
private:
    string filename;
    map<string, int> scoreCache; // cache en memoria

    // Lee todos los registros y actualiza cache
    vector<tuple<string, int, string>> readAll() {
        vector<tuple<string,int,string>> rows;
        ifstream ifs(filename);
        if (!ifs) return rows;
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            auto p1 = line.find('|');
            auto p2 = line.find('|', p1 + 1);
            if (p1 == string::npos || p2 == string::npos) continue;
            string alias = line.substr(0, p1);
            int pts = 0;
            try { pts = stoi(line.substr(p1 + 1, p2 - p1 - 1)); } catch (...) { pts = 0; }
            string date = line.substr(p2 + 1);
            rows.emplace_back(alias, pts, date);
            scoreCache[alias] = pts;
        }
        return rows;
    }

public:
    ScoreManager(const string& fname = "scores.txt") : filename(fname) {
        readAll(); // inicializa cache
    }

    // Guarda o acumula puntaje
    void saveScore(const string& alias, int points) {
        auto rows = readAll();
        bool found = false;
        for (auto& t : rows) {
            if (get<0>(t) == alias) {
                get<1>(t) += points;
                found = true;
                break;
            }
        }
        time_t tnow = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&tnow));
        if (!found) rows.emplace_back(alias, points, string(buf));
        else {
            for (auto& t : rows) if (get<0>(t) == alias) get<2>(t) = string(buf);
        }
        ofstream ofs(filename, ios::trunc);
        for (auto& t : rows) {
            ofs << get<0>(t) << "|" << get<1>(t) << "|" << get<2>(t) << "\n";
        }
        // actualizar cache
        scoreCache[alias] = getCachedScore(alias) + points;
    }

    // Muestra reporte ordenado descendente (top 10)
    void showReport() {
        auto rows = readAll();
        set<tuple<int, string, string>, greater<tuple<int,string,string>>> sortedScores;
        for (const auto& r : rows) {
            sortedScores.insert({get<1>(r), get<0>(r), get<2>(r)});
        }
        if (sortedScores.empty()) {
            cout << "No hay registros aun.\n"; return;
        }
        cout << "RANKING DE PUNTAJES\n====================\n";
        cout << "POS\tALIAS\tPUNTOS\tFECHA\n-------------------------------\n";
        int pos = 1;
        for (const auto& score : sortedScores) {
            cout << pos++ << "\t" << get<1>(score) << "\t" << get<0>(score) << "\t" << get<2>(score) << "\n";
            if (pos > 10) break;
        }
    }

    int getCachedScore(const string& alias) {
        auto it = scoreCache.find(alias);
        return (it != scoreCache.end()) ? it->second : 0;
    }
};

/* ---------------------------
   MODOS DE JUEGO (polimorfismo)
   ---------------------------*/

class GameMode : public GameEntity {
protected:
    SoundSystem& soundSystem;
public:
    GameMode(const string& n, SoundSystem& ss) : GameEntity(n), soundSystem(ss) {}
    virtual ~GameMode() = default;

    // play es el metodo principal de cada modo
    virtual void play(ScoreManager& sc) = 0;
    virtual int getDifficulty() const = 0;
    // implementar initialize/update/render por cada modo segun sea necesario
};

/* Modo Manual */
class ManualMode : public GameMode {
private:
    Puzzle puzzle;
    int shuffleSteps;
    int difficulty;

public:
    ManualMode(SoundSystem& ss, int shuffle = 50, int diff = 1)
        : GameMode("ManualMode", ss), shuffleSteps(shuffle), difficulty(diff) {}

    void initialize() override { puzzle.initialize(); }
    void update() override {}
    void render() override { puzzle.render(); }

    int getDifficulty() const override { return difficulty; }

    void play(ScoreManager& scorer) override {
        cout << "Modo MANUAL - Nivel " << (difficulty == 1 ? "FACIL" : "DIFICIL") << ".\n";
        cout << "Ingresa alias (vacio para jugar sin puntaje): ";
        string alias; getline(cin, alias);

        puzzle.shuffleRandom(shuffleSteps);
        while (!puzzle.isSolvable()) puzzle.shuffleRandom(10);
        int moves = 0;

        while (true) {
            puzzle.print();
            cout << "Movimientos: " << moves << "\n";
            cout << "Opciones: [m]over ficha, [d]ir (u/d/l/r), [s]ugerir, [n]uevo, \n";
            cout << "          [h]istorial, [r]eporte, [q]uit\n> ";

            string cmd; getline(cin, cmd);
            if (cmd.empty()) continue;
            char c = cmd[0];

            if (c == 'm') {
                cout << "Ingresa numero de ficha (1-8): ";
                string arg; getline(cin, arg);
                if (arg.empty()) continue;
                int val = -1;
                try { val = stoi(arg); } catch (...) { val = -1; }
                if (val < 1 || val > 8) {
                    cout << "Valor invalido.\n"; soundSystem.playErrorSound(); continue;
                }
                Board b = puzzle.getBoard();
                int idx = -1;
                for (int i = 0; i < 9; ++i) if (b[i] == val) idx = i;
                if (idx == -1) { cout << "Ficha no encontrada.\n"; continue; }
                if (puzzle.moveTileAt(idx)) { ++moves; soundSystem.playMoveSound(); }
                else { cout << "Ficha no adyacente al hueco.\n"; soundSystem.playErrorSound(); }
            }
            else if (c == 'd') {
                cout << "Ingresa direccion (u/d/l/r): ";
                string arg; getline(cin, arg);
                if (arg.empty()) continue;
                char dir = arg[0];
                if (puzzle.moveDir(dir)) { ++moves; soundSystem.playMoveSound(); }
                else { cout << "Movimiento invalido.\n"; soundSystem.playErrorSound(); }
            }
            else if (c == 's') {
                Puzzle goal;
                AStarSolver solver(puzzle, goal, 50000, 200000);
                cout << "Buscando sugerencia con A*...\n";
                auto res = solver.solve();
                if (!res.first || res.second.size() < 2) {
                    cout << "No se encontro sugerencia dentro de limites.\n";
                } else {
                    cout << "Primer paso sugerido (estado resultante):\n";
                    res.second[1].print();
                    cout << "Aplicar sugerencia? (s/n): ";
                    string r; getline(cin, r);
                    if (!r.empty() && (r[0] == 's' || r[0] == 'S')) {
                        puzzle = res.second[1];
                        ++moves;
                        soundSystem.playMoveSound();
                    }
                }
            }
            else if (c == 'n') {
                puzzle.shuffleRandom(shuffleSteps);
                while (!puzzle.isSolvable()) puzzle.shuffleRandom(10);
                moves = 0;
                cout << "Nuevo tablero generado.\n";
            }
            else if (c == 'h') {
                puzzle.showMoveHistory();
            }
            else if (c == 'r') {
                scorer.showReport();
            }
            else if (c == 'q') {
                Puzzle goal;
                if (puzzle.isGoal(goal)) {
                    cout << "FELICIDADES! Resolviste el puzzle.\n";
                    soundSystem.playWinSound();
                    if (!alias.empty()) {
                        int pts = max(0, 1000 - moves * 10);
                        scorer.saveScore(alias, pts);
                        cout << "Puntos guardados: " << pts << "\n";
                    }
                } else {
                    cout << "Salir. Guardar puntaje parcial? (s/n): ";
                    string r; getline(cin, r);
                    if (!alias.empty() && !r.empty() && (r[0] == 's' || r[0] == 'S')) {
                        int pts = max(0, 1000 - moves * 5);
                        scorer.saveScore(alias, pts);
                        cout << "Puntos parciales guardados: " << pts << "\n";
                    }
                }
                break;
            }
            else {
                cout << "Opcion invalida.\n";
            }

            Puzzle goal;
            if (puzzle.isGoal(goal)) {
                puzzle.print();
                cout << "Resuelto en " << moves << " movimientos.\n";
                soundSystem.playWinSound();
                if (!alias.empty()) {
                    int pts = max(0, 1000 - moves * 10);
                    scorer.saveScore(alias, pts);
                    cout << "Puntos guardados: " << pts << "\n";
                }
                break;
            }
        }
    }
};

/* Modo Inteligente (A*) */
class IntelligentMode : public GameMode {
private:
    size_t maxExp;
    size_t maxOpen;
    int difficulty;

public:
    IntelligentMode(SoundSystem& ss, size_t me = 200000, size_t mo = 500000, int diff = 1)
        : GameMode("IntelligentMode", ss), maxExp(me), maxOpen(mo), difficulty(diff) {}

    void initialize() override {}
    void update() override {}
    void render() override {}

    int getDifficulty() const override { return difficulty; }

    void play(ScoreManager& scorer) override {
        cout << "Modo INTELIGENTE - Nivel " << (difficulty == 1 ? "FACIL" : "DIFICIL") << ".\n";
        cout << "Ingresa tablero de INICIO (9 numeros 0..8, 0 = hueco):\n> ";

        vector<int> vstart;
        while (vstart.empty()) {
            string line; getline(cin, line);
            stringstream ss(line);
            int x;
            while (ss >> x) vstart.push_back(x);
            if (vstart.size() != 9) { cout << "Entrada invalida. Intenta de nuevo:\n> "; vstart.clear(); continue; }
            vector<int> seen(9,0);
            bool ok = true;
            for (int t : vstart) { if (t < 0 || t > 8) ok = false; else ++seen[t]; }
            for (int i = 0; i < 9; ++i) if (seen[i] != 1) ok = false;
            if (!ok) { cout << "Valores invalidos o repetidos. Intenta de nuevo:\n> "; vstart.clear(); }
        }

        cout << "Usar meta por defecto (1 2 3 4 5 6 7 8 0)? (s/n): ";
        string r; getline(cin, r);

        vector<int> vgoal;
        if (!r.empty() && (r[0] == 's' || r[0] == 'S')) {
            vgoal = {1,2,3,4,5,6,7,8,0};
        } else {
            cout << "Ingresa tablero META (9 numeros 0..8):\n> ";
            while (vgoal.empty()) {
                string line; getline(cin, line);
                stringstream ss(line);
                int x;
                while (ss >> x) vgoal.push_back(x);
                if (vgoal.size() != 9) { cout << "Meta invalida. Intenta de nuevo:\n> "; vgoal.clear(); continue; }
                vector<int> seen(9,0);
                bool ok = true;
                for (int t : vgoal) { if (t < 0 || t > 8) ok = false; else ++seen[t]; }
                for (int i = 0; i < 9; ++i) if (seen[i] != 1) ok = false;
                if (!ok) { cout << "Valores invalidos o repetidos. Intenta de nuevo:\n> "; vgoal.clear(); }
            }
        }

        Board bs, bg;
        for (int i = 0; i < 9; ++i) { bs[i] = vstart[i]; bg[i] = vgoal[i]; }
        Puzzle start(bs), goal(bg);

        if (!start.isSolvable()) {
            cout << "Estado inicial no resolvible. Termina modo inteligente.\n";
            return;
        }

        cout << "Ejecutando A* (heuristica Manhattan)...\n";
        AStarSolver solver(start, goal, maxExp, maxOpen);
        auto t0 = chrono::steady_clock::now();
        auto res = solver.solve();
        auto t1 = chrono::steady_clock::now();
        long long ms = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

        if (!res.first) {
            cout << "No se encontro solucion dentro de limites. Tiempo (ms): " << ms << "\n";
            return;
        }

        cout << "Solucion encontrada en " << (int)(res.second.size() - 1) << " movimientos. Tiempo (ms): " << ms << "\n";
        cout << "Mostrar paso a paso? (s/n): ";
        string ans; getline(cin, ans);
        if (!ans.empty() && (ans[0] == 's' || ans[0] == 'S')) {
            for (size_t i = 0; i < res.second.size(); ++i) {
                cout << "Paso " << i << ":\n";
                res.second[i].print();
                if (i + 1 < res.second.size()) {
                    cout << "Presiona Enter para continuar...";
                    string tmp; getline(cin, tmp);
                }
            }
            soundSystem.playWinSound();
        } else {
            cout << "Resumen de la ruta: total pasos = " << (int)(res.second.size() - 1) << "\n";
        }

        cout << "Deseas guardar puntaje por resolver? (s/n): ";
        string save; getline(cin, save);
        if (!save.empty() && (save[0] == 's' || save[0] == 'S')) {
            cout << "Ingresa alias: ";
            string alias; getline(cin, alias);
            int pts = max(0, 2000 - (int)(res.second.size() - 1) * 20);
            if (!alias.empty()) {
                scorer.saveScore(alias, pts);
                cout << "Puntaje guardado: " << pts << "\n";
            } else cout << "Alias vacio; no se guardo.\n";
        }
    }
};

/* ---------------------------
   CLASE GAME (orquestador)
   ---------------------------*/

class Game {
private:
    ScoreManager scorer;
    SoundSystem soundSystem;

    unique_ptr<GameMode> createMode(int modeChoice, int level) {
        if (modeChoice == 1) {
            if (level == 1) return make_unique<ManualMode>(soundSystem, 30, 1);
            else return make_unique<ManualMode>(soundSystem, 80, 2);
        } else if (modeChoice == 2) {
            if (level == 1) return make_unique<IntelligentMode>(soundSystem, 100000, 300000, 1);
            else return make_unique<IntelligentMode>(soundSystem, 400000, 900000, 2);
        }
        return nullptr;
    }

public:
    Game() : scorer("scores.txt"), soundSystem() {
        soundSystem.disable();
    }

    void showHelp() {
        cout << "=== INSTRUCCIONES 8-PUZZLE ===\n";
        cout << "Modo Manual:\n";
        cout << " - [m]over: Mover ficha por numero\n";
        cout << " - [d]ir: Mover por direccion (u/d/l/r)\n";
        cout << " - [s]ugerir: Sugerencia usando A*\n";
        cout << " - [n]uevo: Nuevo tablero aleatorio\n";
        cout << " - [h]istorial: Ver movimientos\n";
        cout << "Modo Inteligente:\n";
        cout << " - Ingresa estado inicial y meta\n";
        cout << " - A* encuentra solucion optima\n";
        cout << "Sistema de Puntos:\n";
        cout << " - Menos movimientos = mas puntos\n";
        cout << " - Puntos se guardan por alias\n";
    }

    void toggleSound() {
        if (soundSystem.isEnabled()) {
            soundSystem.disable();
            cout << "Sonido DESACTIVADO\n";
        } else {
            soundSystem.enable();
            cout << "Sonido ACTIVADO (simulado en consola)\n";
        }
    }

    void mainMenu() {
        while (true) {
            cout << "\n=== 8-PUZZLE - SISTEMA COMPLETO ===\n";
            cout << "1) Modo Manual\n2) Modo Inteligente (A*)\n3) Reporte Puntajes\n4) Instrucciones\n5) Sonido: " << (soundSystem.isEnabled() ? "ON" : "OFF") << "\n6) Salir\n";
            cout << "Selecciona opcion: ";

            string opt; getline(cin, opt);
            if (opt.empty()) continue;

            if (opt[0] == '1' || opt[0] == '2') {
                int mode = (opt[0] == '1') ? 1 : 2;
                cout << "Selecciona nivel: 1) Facil  2) Dificil : ";
                string lvl; getline(cin, lvl);
                int level = 1;
                if (lvl == "1") level = 1;
                else if (lvl == "2") level = 2;
                else {
                    cout << "Nivel invalido, se usa nivel FACIL por defecto.\n";
                    level = 1;
                }
                auto modePtr = createMode(mode, level);
                if (modePtr) {
                    modePtr->initialize();
                    modePtr->play(scorer);
                }
            }
            else if (opt[0] == '3') {
                scorer.showReport();
            }
            else if (opt[0] == '4') {
                showHelp();
            }
            else if (opt[0] == '5') {
                toggleSound();
            }
            else if (opt[0] == '6') {
                cout << "Saliendo...\n";
                break;
            }
            else {
                cout << "Opcion invalida.\n";
            }
        }
    }
};

/* ---------------------------
   MAIN
   ---------------------------*/

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "========================================\n";
    cout << "    PROYECTO 8-PUZZLE - ESTRUCTURAS DE DATOS II\n";
    cout << "    Sistema Completo con POO y Estructuras\n";
    cout << "========================================\n";

    Game game;
    game.mainMenu();
    return 0;
}
