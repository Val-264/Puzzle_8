#include <bits/stdc++.h>
using namespace std;
using Board = array<int,9>;
using Clock = chrono::system_clock;

/*
  Clase Puzzle:
    - Representa el tablero 3x3 (8-puzzle) en una array de 9 posiciones.
    - Proporciona operaciones de movimiento, validación, muestra, barajar (shuffle),
      comprobación de solvabilidad y helpers para A*.
*/
class Puzzle {
private:
    Board board;

    // índice de fila/col desde índice lineal 0..8
    inline pair<int,int> idxToRC(int idx) const { return {idx/3, idx%3}; }
    inline int rcToIdx(int r, int c) const { return r*3 + c; }

public:
    Puzzle() {
        // estado objetivo por defecto: 1..8, 0 como hueco
        for (int i=0;i<8;i++) board[i]=i+1;
        board[8]=0;
    }

    Puzzle(const Board &b) { board = b; }

    Board getBoard() const { return board; }

    void setBoard(const Board &b) { board = b; }

    // imprimir en consola
    void print() const {
        cout << "-------------\n";
        for (int r=0;r<3;r++) {
            cout << "| ";
            for (int c=0;c<3;c++) {
                int v = board[rcToIdx(r,c)];
                if (v==0) cout << "_";
                else cout << v;
                cout << " | ";
            }
            cout << "\n-------------\n";
        }
    }

    // encuentra índice del hueco (0)
    int findZero() const {
        for (int i=0;i<9;i++) if (board[i]==0) return i;
        return -1;
    }

    // mueve la ficha indicada por índice src hacia el hueco si es adyacente. Retorna true si se movió.
    bool moveTileAt(int srcIdx) {
        int z = findZero();
        auto [zr,zc] = idxToRC(z);
        auto [sr,sc] = idxToRC(srcIdx);
        int dr = abs(zr - sr), dc = abs(zc - sc);
        if ((dr + dc) == 1) { // adyacente
            swap(board[z], board[srcIdx]);
            return true;
        }
        return false;
    }

    // mover por dirección 'u','d','l','r' respecto al hueco (es decir deslizar pieza)
    bool moveDir(char dir) {
        int z = findZero();
        auto [r,c] = idxToRC(z);
        int nr=r, nc=c;
        if (dir=='u') nr = r+1; // mover la ficha arriba del hueco hacia abajo => hueco sube -> implement mapping carefully
        else if (dir=='d') nr = r-1;
        else if (dir=='l') nc = c+1;
        else if (dir=='r') nc = c-1;
        else return false;
        if (nr<0 || nr>2 || nc<0 || nc>2) return false;
        swap(board[z], board[rcToIdx(nr,nc)]);
        return true;
    }

    // genera un puzzle aleatorio resolvable aplicando movimientos válidos desde objetivo
    void shuffleRandom(int steps = 50) {
        // partimos del objetivo
        for (int i=0;i<8;i++) board[i]=i+1;
        board[8]=0;
        random_device rd; mt19937 gen(rd());
        char dirs[4] = {'u','d','l','r'};
        for (int s=0;s<steps;s++) {
            vector<char> possible;
            for (char d: dirs) {
                Puzzle tmp = *this;
                if (tmp.moveDir(d)) possible.push_back(d);
            }
            if (!possible.empty()) {
                uniform_int_distribution<int> dis(0,(int)possible.size()-1);
                moveDir(possible[dis(gen)]);
            }
        }
    }

    // calcula la distancia Manhattan respecto a un tablero objetivo
    int manhattan(const Puzzle &goal) const {
        int sum = 0;
        Board g = goal.getBoard();
        // mapa valor -> índice objetivo
        array<int,9> posGoal;
        for (int i=0;i<9;i++) posGoal[g[i]] = i;
        for (int i=0;i<9;i++) {
            int val = board[i];
            if (val == 0) continue;
            int gi = posGoal[val];
            auto [r1,c1] = idxToRC(i);
            auto [r2,c2] = idxToRC(gi);
            sum += abs(r1-r2) + abs(c1-c2);
        }
        return sum;
    }

    // cuenta cuantas piezas mal colocadas (sin usar) - Hamming
    int hamming(const Puzzle &goal) const {
        int cnt=0;
        Board g = goal.getBoard();
        for (int i=0;i<9;i++) if (board[i]!=0 && board[i]!=g[i]) cnt++;
        return cnt;
    }

    // comparación de igualdad
    bool operator==(const Puzzle &other) const {
        return board == other.board;
    }

    bool isGoal(const Puzzle &goal) const {
        return *this == goal;
    }

    // chequea si la permutación es resolvible (inversion count)
    bool isSolvable() const {
        vector<int> v;
        for (int i=0;i<9;i++) if (board[i]!=0) v.push_back(board[i]);
        int inv = 0;
        for (int i=0;i<(int)v.size();i++) for (int j=i+1;j<(int)v.size();j++) if (v[i]>v[j]) inv++;
        // Para 3x3 con blank en cualquier lugar, solvable si #inversions even
        return (inv % 2) == 0;
    }

    // obtener sucesores (todos los puzzle alcanzables con un movimiento)
    vector<Puzzle> successors() const {
        vector<Puzzle> out;
        int z = findZero();
        auto [r,c] = idxToRC(z);
        array<int,4> dr = {-1,1,0,0};
        array<int,4> dc = {0,0,-1,1};
        for (int k=0;k<4;k++) {
            int nr=r+dr[k], nc=c+dc[k];
            if (nr<0||nr>2||nc<0||nc>2) continue;
            Puzzle p = *this;
            swap(p.board[z], p.board[rcToIdx(nr,nc)]);
            out.push_back(p);
        }
        return out;
    }

    // obtiene el movimiento (índice de ficha) que produce otro estado (si difieren en un swap con el hueco)
    int movedTileToGet(const Puzzle &other) const {
        // devuelve índice en 'other' de la ficha que quedó en hueco de this (útil para mostrar cuál moved)
        for (int i=0;i<9;i++) if (board[i] != other.board[i]) {
            if (board[i]==0) return other.board[i]; // rarely useful
        }
        return -1;
    }
};

/*
  Node usado por A*:
    - almacena estado (Puzzle), g (costo real), f (g + h), puntero a padre para reconstruir la ruta.
*/
struct Node {
    Puzzle state;
    int g;
    int f;
    shared_ptr<Node> parent;
    Node(const Puzzle &s, int g_, int f_, shared_ptr<Node> parent_)
        : state(s), g(g_), f(f_), parent(parent_) {}
};

// Comparador para priority_queue (menor f tiene mayor prioridad)
struct NodeCmp {
    bool operator()(const shared_ptr<Node> &a, const shared_ptr<Node> &b) const {
        if (a->f == b->f) return a->g < b->g ? false : true; // tie-breaker: prefer mayor g? aquí preferimos menor g
        return a->f > b->f;
    }
};

/*
  Clase AStarSolver:
    - Implementa A* con heurística Manhattan.
    - Permite limitar el número máximo de nodos/expansiones para evitar explosión de memoria.
    - Devuelve camino (vector<Puzzle>) desde inicio hasta meta si lo encuentra.
*/
class AStarSolver {
private:
    Puzzle start;
    Puzzle goal;
    size_t maxExplored;
    size_t maxNodes;

public:
    AStarSolver(const Puzzle &s, const Puzzle &g, size_t maxExpl = 100000, size_t maxN = 1000000)
        : start(s), goal(g), maxExplored(maxExpl), maxNodes(maxN) {}

    // Ejecuta A*, retorna pair(found, path)
    pair<bool, vector<Puzzle>> solve() {
        // estructuras open (pq) y closed (map estado->g)
        priority_queue< shared_ptr<Node>, vector< shared_ptr<Node> >, NodeCmp > open;
        unordered_map<string,int> closed; // estado serializado -> best g seen

        auto serialize = [](const Puzzle &p)->string {
            Board b = p.getBoard();
            string s; s.reserve(9);
            for (int i=0;i<9;i++) s.push_back(char(b[i])); // valores pequeños; ok en string
            return s;
        };

        int h0 = start.manhattan(goal);
        open.push(make_shared<Node>(start, 0, h0, nullptr));
        size_t expansions = 0;

        while (!open.empty()) {
            if (expansions > maxExplored) break;
            auto cur = open.top(); open.pop();
            expansions++;
            string key = serialize(cur->state);
            // si ya vimos con menor g, ignorar
            if (closed.count(key) && closed[key] <= cur->g) continue;
            closed[key] = cur->g;

            if (cur->state.isGoal(goal)) {
                // reconstruir ruta
                vector<Puzzle> path;
                auto it = cur;
                while (it) {
                    path.push_back(it->state);
                    it = it->parent;
                }
                reverse(path.begin(), path.end());
                return {true, path};
            }

            // generar sucesores
            auto succs = cur->state.successors();
            for (auto &s : succs) {
                string skey = serialize(s);
                int ng = cur->g + 1;
                if (closed.count(skey) && closed[skey] <= ng) continue;
                int h = s.manhattan(goal);
                int nf = ng + h;
                open.push(make_shared<Node>(s, ng, nf, cur));
                if (open.size() > maxNodes) { /* control de explosion */ break; }
            }
        }
        return {false, {}}; // no encontrado dentro de límites
    }
};

/*
  Clase ScoreManager:
    - Maneja archivo de puntajes "scores.txt"
    - Guarda: alias | puntos | fecha (ISO)
    - Lectura y reporte ordenado por puntaje descendente.
*/
class ScoreManager {
private:
    string filename;
public:
    ScoreManager(const string &fname="scores.txt") : filename(fname) {}

    void saveScore(const string &alias, int points) {
        ofstream ofs(filename, ios::app);
        if (!ofs) return;
        auto t = Clock::to_time_t(Clock::now());
        // formato ISO simple
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
        ofs << alias << "|" << points << "|" << buf << "\n";
    }

    // muestra los mejores registros
    void showReport() {
        ifstream ifs(filename);
        if (!ifs) {
            cout << "No hay registros aun.\n";
            return;
        }
        struct R { string alias; int pts; string date; };
        vector<R> rows;
        string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            auto p1 = line.find('|');
            auto p2 = line.find('|', p1+1);
            if (p1==string::npos || p2==string::npos) continue;
            R r;
            r.alias = line.substr(0,p1);
            r.pts = stoi(line.substr(p1+1, p2-p1-1));
            r.date = line.substr(p2+1);
            rows.push_back(r);
        }
        sort(rows.begin(), rows.end(), [](const R &a, const R &b){ return a.pts > b.pts; });
        cout << "Alias\tPuntos\tFecha\n";
        cout << "-------------------------------\n";
        for (auto &r: rows) {
            cout << r.alias << "\t" << r.pts << "\t" << r.date << "\n";
        }
    }
};

/*
  Clase Game:
    - Orquesta menú principal, modo manual e inteligente.
    - En modo manual: mover por número de pieza, generar aleatorio, sugerir movimiento.
    - En modo inteligente: el usuario especifica inicio y meta (por ingreso de 9 números), A* resuelve y muestra pasos.
*/
class Game {
private:
    ScoreManager scorer;

    // validate board input: recibe vector<int> de 9 elementos que representan 0..8 uniques
    bool validBoardVec(const vector<int> &v) {
        if (v.size()!=9) return false;
        vector<int> seen(9,0);
        for (int x: v) if (x<0 || x>8) return false; else seen[x]++;
        for (int i=0;i<9;i++) if (seen[i]!=1) return false;
        return true;
    }

    // administra modo manual
    void playManual() {
        Puzzle p;
        string alias;
        cout << "Modo MANUAL. Ingresa tu alias para puntuación (o deja vacío para jugar sin puntaje): ";
        getline(cin, alias);
        p.shuffleRandom(60);
        // asegurar solvable
        while (!p.isSolvable()) p.shuffleRandom(20);

        int moves = 0;
        while (true) {
            p.print();
            cout << "Movimientos: " << moves << "\n";
            cout << "Opciones: [m]over por ficha, [s]ugerir jugada, [n]uevo aleatorio, [q]uit y guardar\n> ";
            string cmd; getline(cin, cmd);
            if (cmd.empty()) continue;
            if (cmd[0]=='m') {
                cout << "Ingresa numero de ficha a mover (1-8) o 'dir' (u/d/l/r): ";
                string arg; getline(cin, arg);
                if (arg.empty()) continue;
                if (arg.size()==1 && strchr("udlr", arg[0])) {
                    if (p.moveDir(arg[0])) moves++;
                    else cout << "Movimiento invalido.\n";
                } else {
                    int val;
                    try { val = stoi(arg); } catch(...) { cout << "Entrada invalida\n"; continue; }
                    // buscar indice de esa ficha
                    Board b = p.getBoard();
                    int idx=-1;
                    for (int i=0;i<9;i++) if (b[i]==val) idx=i;
                    if (idx==-1) { cout << "Ficha no encontrada\n"; continue; }
                    if (!p.moveTileAt(idx)) cout << "La ficha no es adyacente al hueco.\n"; else moves++;
                }
            } else if (cmd[0]=='s') {
                // sugerir: correr A* limitado desde estado actual hacia goal objetivo (1..8,0)
                Puzzle goal;
                // limite controlado para sugerir: expande 2000 nodos max
                AStarSolver solver(p, goal, 5000, 200000);
                auto res = solver.solve();
                if (!res.first || res.second.size()<2) {
                    cout << "No se encontró sugerencia (dentro de límites).\n";
                } else {
                    cout << "Sugerencia: realizar primer movimiento de la ruta encontrada.\n";
                    // aplicar la jugada sugerida (el siguiente estado)
                    Puzzle next = res.second[1];
                    // encontrar cual ficha se movió (el que ahora ocupa el hueco anterior)
                    // para aplicar en el tablero actual, simplemente asignar el estado next
                    p = next;
                    moves++;
                }
            } else if (cmd[0]=='n') {
                p.shuffleRandom(60);
                while (!p.isSolvable()) p.shuffleRandom(20);
                moves = 0;
                cout << "Nuevo juego generado.\n";
            } else if (cmd[0]=='q') {
                cout << "Salir. Guardar puntaje? (s/n): ";
                string r; getline(cin, r);
                if (!alias.empty() && r.size()>0 && (r[0]=='s' || r[0]=='S')) {
                    int pts = max(0, 1000 - moves*10); // ejemplo de cálculo de puntaje
                    scorer.saveScore(alias, pts);
                    cout << "Guardado " << alias << " -> " << pts << " pts\n";
                }
                break;
            } else {
                cout << "Opcion invalida.\n";
            }

            // si llega a meta lo detectamos
            Puzzle goal;
            if (p.isGoal(goal)) {
                p.print();
                cout << "FELICIDADES! Resolviste el puzzle en " << moves << " movimientos.\n";
                if (!alias.empty()) {
                    int pts = max(0, 1000 - moves*10);
                    scorer.saveScore(alias, pts);
                    cout << "Guardado " << alias << " -> " << pts << " pts\n";
                }
                break;
            }
        }
    }

    // administra modo inteligente (usuario ingresa inicio y meta)
    void playIntelligent() {
        cout << "Modo INTELIGENTE. Ingrese tablero de INICIO (9 numeros 0..8 con 0 = hueco) separados por espacios:\n";
        vector<int> vstart;
        while (vstart.empty()) {
            cout << "> ";
            string line; getline(cin, line);
            stringstream ss(line);
            int x; while (ss >> x) vstart.push_back(x);
            if (!validBoardVec(vstart)) { cout << "Entrada invalida. Intenta de nuevo.\n"; vstart.clear(); }
        }
        cout << "Ingrese tablero META (por defecto 1..8 0). Si quieres usar la meta por defecto, escribe 'd'\n> ";
        string line; getline(cin, line);
        vector<int> vgoal;
        if (!line.empty() && (line[0]=='d' || line[0]=='D')) {
            vgoal = {1,2,3,4,5,6,7,8,0};
        } else {
            stringstream ss(line);
            int x; while (ss >> x) vgoal.push_back(x);
            while (!validBoardVec(vgoal)) {
                cout << "Meta invalida. Ingrésala de nuevo:\n> ";
                vgoal.clear();
                string l2; getline(cin, l2);
                stringstream ss2(l2);
                while (ss2 >> x) vgoal.push_back(x);
            }
        }

        Puzzle start, goal;
        Board bstart, bgoal;
        for (int i=0;i<9;i++) { bstart[i] = vstart[i]; bgoal[i] = vgoal[i]; }
        start.setBoard(bstart);
        goal.setBoard(bgoal);

        if (!start.isSolvable()) {
            cout << "El estado inicial NO es resolvible (segun inversions). No se puede volver a la meta dada.\n";
            return;
        }
        // resolver con A*
        cout << "Resolviendo con A* (heuristica Manhattan). Esto puede tardar segun la distancia...\n";
        // límites razonables (puedes ajustar)
        AStarSolver solver(start, goal, 200000, 500000);
        auto t0 = chrono::steady_clock::now();
        auto res = solver.solve();
        auto t1 = chrono::steady_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(t1-t0).count();
        if (!res.first) {
            cout << "No se encontró solución dentro de los limites establecidos (" << ms << " ms)\n";
            return;
        }
        cout << "Solución encontrada en " << res.second.size()-1 << " movimientos. Tiempo: " << ms << " ms\n";
        cout << "Mostrar paso a paso? (s/n): ";
        string r; getline(cin, r);
        if (!r.empty() && (r[0]=='s' || r[0]=='S')) {
            for (size_t i=0;i<res.second.size();i++) {
                cout << "Paso " << i << ":\n";
                res.second[i].print();
                // esperar input para el siguiente paso
                if (i+1 < res.second.size()) {
                    cout << "Presiona Enter para siguiente paso...";
                    string tmp; getline(cin, tmp);
                }
            }
        } else {
            cout << "Ruta (resumen):\n";
            for (size_t i=0;i<res.second.size();i++) cout << i << (i+1<res.second.size()? " -> ": "\n");
        }
    }

public:
    Game() : scorer("scores.txt") {}
    
    // menú principal

    void mainMenu() {
        while (true) {
            cout << "\n=== 8-PUZZLE - MENU PRINCIPAL ===\n";
            cout << "1) Jugar - Modo Manual\n";
            cout << "2) Jugar - Modo Inteligente (A*)\n";
            cout << "3) Reporte de Puntajes\n";
            cout << "4) Instrucciones / Ayuda\n";
            cout << "5) Salir\n";
            cout << "Seleccione una opcion: ";
            string opt; getline(cin, opt);
            if (opt.empty()) continue;
            if (opt[0]=='1') playManual();
            else if (opt[0]=='2') playIntelligent();
            else if (opt[0]=='3') scorer.showReport();
            else if (opt[0]=='4') {
                cout << "Instrucciones:\n- Modo Manual: puedes mover fichas introduciendo el numero de ficha (1-8) o mover por direccion (u/d/l/r).\n- Sugerir jugada usa A* con heuristica Manhattan (limitado).\n- Modo Inteligente: ingresa inicio y meta y A* buscara la solucion optima.\n";
            }
            else if (opt[0]=='5') { cout << "Saliendo...\n"; break; }
            else cout << "Opcion invalida.\n";
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout << "Proyecto 8-Puzzle - Implementacion en OOP (Consola)\n";
    Game g;
    g.mainMenu();
    return 0;
}
