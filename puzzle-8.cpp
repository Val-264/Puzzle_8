// eight_puzzle_clean_vs.cpp
// Version limpia para Visual Studio / MinGW
// Comentarios en espanol SIN acentos
// Compilar: cl /EHsc /std:c++17 eight_puzzle_clean_vs.cpp
// o con g++: g++ -std=c++17 -O2 -o eight_puzzle_clean_vs eight_puzzle_clean_vs.cpp

#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <ctime>
#include <string>

using namespace std;

/* Tipos y constantes */
using Board = array<int,9>;
using TimePoint = chrono::system_clock::time_point;
static const Board GOAL_BOARD = {1,2,3,4,5,6,7,8,0};

/* Clase Puzzle
   - Representa el tablero 3x3 del 8-puzzle
   - Proporciona operaciones: print, movimientos, shuffle, heuristicas, sucesores, solvabilidad
*/
class Puzzle {
private:
    Board board;

    inline pair<int,int> idxToRC(int idx) const { return {idx/3, idx%3}; }
    inline int rcToIdx(int r,int c) const { return r*3 + c; }

public:
    Puzzle() { board = GOAL_BOARD; }
    Puzzle(const Board &b) : board(b) {}

    Board getBoard() const { return board; }
    void setBoard(const Board &b) { board = b; }

    // Imprime el tablero en consola
    void print() const {
        cout << "+---+---+---+\n";
        for (int r=0; r<3; ++r) {
            cout << "| ";
            for (int c=0; c<3; ++c) {
                int v = board[rcToIdx(r,c)];
                if (v == 0) cout << "_";
                else cout << v;
                cout << " | ";
            }
            cout << "\n+---+---+---+\n";
        }
    }

    int findZero() const {
        for (int i=0;i<9;i++) if (board[i]==0) return i;
        return -1;
    }

    // Mueve la ficha en srcIdx hacia el hueco si es adyacente
    bool moveTileAt(int srcIdx) {
        int z = findZero();
        if (srcIdx < 0 || srcIdx > 8 || z < 0) return false;
        auto pr = idxToRC(z);
        auto ps = idxToRC(srcIdx);
        int dist = abs(pr.first - ps.first) + abs(pr.second - ps.second);
        if (dist == 1) {
            swap(board[z], board[srcIdx]);
            return true;
        }
        return false;
    }

    // Mueve por direccion respecto del hueco: 'u' 'd' 'l' 'r'
    bool moveDir(char dir) {
        int z = findZero();
        auto rc = idxToRC(z);
        int nr = rc.first, nc = rc.second;
        if (dir == 'u') nr = rc.first - 1;
        else if (dir == 'd') nr = rc.first + 1;
        else if (dir == 'l') nc = rc.second - 1;
        else if (dir == 'r') nc = rc.second + 1;
        else return false;
        if (nr < 0 || nr > 2 || nc < 0 || nc > 2) return false;
        swap(board[z], board[rcToIdx(nr,nc)]);
        return true;
    }

    // Mezcla aleatoria aplicando movimientos validos desde el objetivo
    void shuffleRandom(int steps = 50) {
        board = GOAL_BOARD;
        random_device rd;
        mt19937 gen(rd());
        vector<char> dirs = {'u','d','l','r'};
        for (int i=0;i<steps;i++) {
            vector<char> poss;
            for (char d : dirs) {
                Puzzle tmp = *this;
                if (tmp.moveDir(d)) poss.push_back(d);
            }
            if (!poss.empty()) {
                uniform_int_distribution<int> dis(0, (int)poss.size()-1);
                moveDir(poss[dis(gen)]);
            }
        }
    }

    // Heuristica Manhattan respecto a un estado meta
    int manhattan(const Puzzle &goal) const {
        Board g = goal.getBoard();
        array<int,9> posGoal;
        for (int i=0;i<9;i++) posGoal[g[i]] = i;
        int sum = 0;
        for (int i=0;i<9;i++) {
            int v = board[i];
            if (v == 0) continue;
            int gi = posGoal[v];
            auto a = idxToRC(i);
            auto b = idxToRC(gi);
            sum += abs(a.first - b.first) + abs(a.second - b.second);
        }
        return sum;
    }

    // Hamming (nro de piezas fuera de lugar, excluye hueco)
    int hamming(const Puzzle &goal) const {
        Board g = goal.getBoard();
        int cnt = 0;
        for (int i=0;i<9;i++) if (board[i] != 0 && board[i] != g[i]) cnt++;
        return cnt;
    }

    bool isGoal(const Puzzle &goal) const {
        return board == goal.getBoard();
    }

    // Comprobar solvabilidad mediante conteo de inversiones (3x3)
    bool isSolvable() const {
        vector<int> v;
        for (int i=0;i<9;i++) if (board[i] != 0) v.push_back(board[i]);
        int inv = 0;
        for (size_t i=0;i<v.size();++i) for (size_t j=i+1;j<v.size();++j) if (v[i] > v[j]) ++inv;
        return (inv % 2) == 0;
    }

    // Sucesores (todos los estados alcanzables en un movimiento)
    vector<Puzzle> successors() const {
        vector<Puzzle> out;
        int z = findZero();
        auto rc = idxToRC(z);
        const array<int,4> dr = {-1,1,0,0};
        const array<int,4> dc = {0,0,-1,1};
        for (int k=0;k<4;k++) {
            int nr = rc.first + dr[k], nc = rc.second + dc[k];
            if (nr < 0 || nr > 2 || nc < 0 || nc > 2) continue;
            Puzzle p = *this;
            swap(p.board[z], p.board[rcToIdx(nr,nc)]);
            out.push_back(p);
        }
        return out;
    }

    // Serializa el estado en string corto para mapas
    string serialize() const {
        string s; s.reserve(9);
        for (int i=0;i<9;i++) s.push_back(static_cast<char>(board[i]));
        return s;
    }
};

/* Nodo para A* */
struct Node {
    Puzzle state;
    int g;
    int f;
    shared_ptr<Node> parent;
    Node(const Puzzle &s, int g_, int f_, shared_ptr<Node> p) : state(s), g(g_), f(f_), parent(p) {}
};

struct NodeCmp {
    bool operator()(const shared_ptr<Node> &a, const shared_ptr<Node> &b) const {
        if (a->f == b->f) return a->g < b->g;
        return a->f > b->f;
    }
};

/* Clase AStarSolver
   - Implementa A* usando heuristica Manhattan
   - Tiene limites de expansiones y tamano de open para controlar explosion
*/
class AStarSolver {
private:
    Puzzle start;
    Puzzle goal;
    size_t maxExpansions;
    size_t maxOpenSize;

public:
    AStarSolver(const Puzzle &s, const Puzzle &g, size_t maxExp = 200000, size_t maxOpen = 500000)
        : start(s), goal(g), maxExpansions(maxExp), maxOpenSize(maxOpen) {}

    pair<bool, vector<Puzzle>> solve() {
        priority_queue< shared_ptr<Node>, vector< shared_ptr<Node> >, NodeCmp > open;
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
            for (auto &s : succs) {
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

/* ScoreManager - maneja archivo de puntajes */
class ScoreManager {
private:
    string filename;
    vector<tuple<string,int,string>> readAll() {
        vector<tuple<string,int,string>> rows;
        ifstream ifs(filename);
        if (!ifs) return rows;
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            auto p1 = line.find('|');
            auto p2 = line.find('|', p1+1);
            if (p1 == string::npos || p2 == string::npos) continue;
            string alias = line.substr(0,p1);
            int pts = 0;
            try { pts = stoi(line.substr(p1+1, p2-p1-1)); } catch(...) { pts = 0; }
            string date = line.substr(p2+1);
            rows.emplace_back(alias, pts, date);
        }
        return rows;
    }

public:
    ScoreManager(const string &fname = "scores.txt") : filename(fname) {}

    void saveScore(const string &alias, int points) {
        auto rows = readAll();
        bool found = false;
        for (auto &t : rows) {
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
            for (auto &t : rows) if (get<0>(t) == alias) get<2>(t) = string(buf);
        }
        ofstream ofs(filename, ios::trunc);
        for (auto &t : rows) {
            ofs << get<0>(t) << "|" << get<1>(t) << "|" << get<2>(t) << "\n";
        }
    }

    void showReport() {
        auto rows = readAll();
        sort(rows.begin(), rows.end(), [](const auto &a, const auto &b){
            return get<1>(a) > get<1>(b);
        });
        if (rows.empty()) { cout << "No hay registros aun.\n"; return; }
        cout << "ALIAS\tPUNTOS\tFECHA\n-------------------------------\n";
        for (auto &r : rows) {
            cout << get<0>(r) << "\t" << get<1>(r) << "\t" << get<2>(r) << "\n";
        }
    }
};

/* Clase abstracta GameMode para polimorfismo */
class GameMode {
public:
    virtual ~GameMode() = default;
    virtual void play(ScoreManager &sc) = 0;
    virtual string name() const = 0;
};

/* Modo Manual */
class ManualMode : public GameMode {
private:
    Puzzle puzzle;
    int shuffleSteps;
public:
    ManualMode(int shuffle = 50) : shuffleSteps(shuffle) {}
    string name() const override { return "Manual"; }

    void play(ScoreManager &scorer) override {
        cout << "Modo MANUAL.\nIngresa alias (vacío para jugar sin puntaje): ";
        string alias; getline(cin, alias);

        puzzle.shuffleRandom(shuffleSteps);
        while (!puzzle.isSolvable()) puzzle.shuffleRandom(10);
        int moves = 0;

        while (true) {
            puzzle.print();
            cout << "Movimientos: " << moves << "\n";
            cout << "Opciones: [m]over ficha, [d]ir (u/d/l/r), [s]ugerir, [n]uevo, [r]eporte, [q]uit\n> ";
            string cmd; getline(cin, cmd);
            if (cmd.empty()) continue;
            char c = cmd[0];
            if (c == 'm') {
                cout << "Ingresa numero de ficha (1-8): ";
                string arg; getline(cin, arg);
                if (arg.empty()) continue;
                int val = -1;
                try { val = stoi(arg); } catch(...) { val = -1; }
                if (val < 1 || val > 8) { cout << "Valor invalido.\n"; continue; }
                Board b = puzzle.getBoard();
                int idx = -1;
                for (int i=0;i<9;i++) if (b[i] == val) idx = i;
                if (idx == -1) { cout << "Ficha no encontrada.\n"; continue; }
                if (puzzle.moveTileAt(idx)) ++moves;
                else cout << "Ficha no adyacente al hueco.\n";
            } else if (c == 'd') {
                cout << "Ingresa direccion (u/d/l/r): ";
                string arg; getline(cin, arg);
                if (arg.empty()) continue;
                char dir = arg[0];
                if (puzzle.moveDir(dir)) ++moves;
                else cout << "Movimiento invalido.\n";
            } else if (c == 's') {
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
                    if (!r.empty() && (r[0]=='s' || r[0]=='S')) {
                        puzzle = res.second[1];
                        ++moves;
                    }
                }
            } else if (c == 'n') {
                puzzle.shuffleRandom(shuffleSteps);
                while (!puzzle.isSolvable()) puzzle.shuffleRandom(10);
                moves = 0;
                cout << "Nuevo tablero generado.\n";
            } else if (c == 'r') {
                scorer.showReport();
            } else if (c == 'q') {
                Puzzle goal;
                if (puzzle.isGoal(goal)) {
                    cout << "FELICIDADES! Resolviste el puzzle.\n";
                    if (!alias.empty()) {
                        int pts = max(0, 1000 - moves*10);
                        scorer.saveScore(alias, pts);
                        cout << "Puntos guardados: " << pts << "\n";
                    }
                } else {
                    cout << "Salir. Guardar puntaje parcial? (s/n): ";
                    string r; getline(cin, r);
                    if (!alias.empty() && !r.empty() && (r[0]=='s' || r[0]=='S')) {
                        int pts = max(0, 1000 - moves*5);
                        scorer.saveScore(alias, pts);
                        cout << "Puntos parciales guardados: " << pts << "\n";
                    }
                }
                break;
            } else {
                cout << "Opcion invalida.\n";
            }

            Puzzle goal;
            if (puzzle.isGoal(goal)) {
                puzzle.print();
                cout << "Resuelto en " << moves << " movimientos.\n";
                if (!alias.empty()) {
                    int pts = max(0, 1000 - moves*10);
                    scorer.saveScore(alias, pts);
                    cout << "Puntos guardados: " << pts << "\n";
                }
                break;
            }
        }
    }
};

/* Modo Inteligente */
class IntelligentMode : public GameMode {
private:
    size_t maxExp;
    size_t maxOpen;
public:
    IntelligentMode(size_t me = 200000, size_t mo = 500000) : maxExp(me), maxOpen(mo) {}
    string name() const override { return "Inteligente"; }

    void play(ScoreManager &scorer) override {
        cout << "Modo INTELIGENTE.\nIngresa tablero de INICIO (9 numeros 0..8, 0 = hueco):\n> ";
        vector<int> vstart;
        while (vstart.empty()) {
            string line; getline(cin, line);
            stringstream ss(line);
            int x;
            while (ss >> x) vstart.push_back(x);
            if (vstart.size() != 9) { cout << "Entrada invalida. Intenta de nuevo:\n> "; vstart.clear(); continue; }
            vector<int> seen(9,0);
            bool ok = true;
            for (int t : vstart) if (t < 0 || t > 8) ok = false; else ++seen[t];
            for (int i=0;i<9;i++) if (seen[i] != 1) ok = false;
            if (!ok) { cout << "Valores invalidos o repetidos. Intenta de nuevo:\n> "; vstart.clear(); }
        }

        cout << "Usar meta por defecto (1 2 3 4 5 6 7 8 0)? (s/n): ";
        string r; getline(cin, r);
        vector<int> vgoal;
        if (!r.empty() && (r[0]=='s' || r[0]=='S')) vgoal = {1,2,3,4,5,6,7,8,0};
        else {
            cout << "Ingresa tablero META (9 numeros 0..8):\n> ";
            while (vgoal.empty()) {
                string line; getline(cin, line);
                stringstream ss(line);
                int x;
                while (ss >> x) vgoal.push_back(x);
                if (vgoal.size() != 9) { cout << "Meta invalida. Intenta de nuevo:\n> "; vgoal.clear(); continue; }
                vector<int> seen(9,0);
                bool ok = true;
                for (int t : vgoal) if (t < 0 || t > 8) ok = false; else ++seen[t];
                for (int i=0;i<9;i++) if (seen[i] != 1) ok = false;
                if (!ok) { cout << "Valores invalidos o repetidos. Intenta de nuevo:\n> "; vgoal.clear(); }
            }
        }

        Board bs, bg;
        for (int i=0;i<9;i++) { bs[i] = vstart[i]; bg[i] = vgoal[i]; }
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

        cout << "Solucion encontrada en " << (res.second.size() - 1) << " movimientos. Tiempo (ms): " << ms << "\n";
        cout << "Mostrar paso a paso? (s/n): ";
        string ans; getline(cin, ans);
        if (!ans.empty() && (ans[0] == 's' || ans[0] == 'S')) {
            for (size_t i=0;i<res.second.size();++i) {
                cout << "Paso " << i << ":\n";
                res.second[i].print();
                if (i + 1 < res.second.size()) {
                    cout << "Presiona Enter para continuar...";
                    string tmp; getline(cin, tmp);
                }
            }
        } else {
            cout << "Resumen de la ruta: total pasos = " << (res.second.size() - 1) << "\n";
        }

        cout << "Deseas guardar puntaje por resolver? (s/n): ";
        string save; getline(cin, save);
        if (!save.empty() && (save[0]=='s' || save[0]=='S')) {
            cout << "Ingresa alias: ";
            string alias; getline(cin, alias);
            int pts = max(0, 2000 - (int)(res.second.size()-1) * 20);
            if (!alias.empty()) {
                scorer.saveScore(alias, pts);
                cout << "Puntaje guardado: " << pts << "\n";
            } else cout << "Alias vacio; no se guardo.\n";
        }
    }
};

/* Clase Game: orquesta menu principal y modos */
class Game {
private:
    ScoreManager scorer;

    unique_ptr<GameMode> createMode(int modeChoice, int level) {
        if (modeChoice == 1) {
            if (level == 1) return make_unique<ManualMode>(30);
            else return make_unique<ManualMode>(80);
        } else if (modeChoice == 2) {
            if (level == 1) return make_unique<IntelligentMode>(100000, 300000);
            else return make_unique<IntelligentMode>(400000, 900000);
        }
        return nullptr;
    }

public:
    Game() : scorer("scores.txt") {}

    void showHelp() {
        cout << "Instrucciones:\n- Modo Manual: mover por numero de ficha o direccion.\n- Sugerencia usa A* (Manhattan).\n- Modo Inteligente: ingresa inicio y meta; A* busca solucion.\n- Puntajes en scores.txt\n";
    }

    void mainMenu() {
        while (true) {
            cout << "\n=== 8-PUZZLE - MENU ===\n";
            cout << "1) Modo Manual\n2) Modo Inteligente (A*)\n3) Reporte Puntajes\n4) Instrucciones\n5) Salir\nSelecciona opcion: ";
            string opt; getline(cin, opt);
            if (opt.empty()) continue;
            if (opt[0] == '1' || opt[0] == '2') {
                int mode = (opt[0] == '1') ? 1 : 2;
                cout << "Selecciona nivel: 1) Facil  2) Dificil : ";
                string lvl; getline(cin, lvl);
                int level = 1;
                if (!lvl.empty() && lvl[0] == '2') level = 2;
                auto modePtr = createMode(mode, level);
                if (modePtr) modePtr->play(scorer);
            } else if (opt[0] == '3') {
                scorer.showReport();
            } else if (opt[0] == '4') {
                showHelp();
            } else if (opt[0] == '5') {
                cout << "Saliendo...\n";
                break;
            } else {
                cout << "Opcion invalida.\n";
            }
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout << "Proyecto 8-Puzzle - Version limpia para Visual Studio\n";
    cout << "Documento de referencia: /mnt/data/Proyecto Final.ED.2025.02 (1).pdf\n";
    Game game;
    game.mainMenu();
    return 0;
}
