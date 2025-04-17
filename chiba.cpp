#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <string>
using namespace std;
using Clock = chrono::high_resolution_clock;

// Global variables
int n;  // number of vertices (read from file)
unordered_map<int, vector<int>> adj;        // original adjacency list (vertex -> list of neighbors)
vector<set<int>> adj2;                        // re–ordered adjacency list (using new vertex numbers)
vector<int> S;                                // auxiliary array S
vector<int> T;                                // auxiliary array T
unordered_set<int> vert;                      // set of vertices (original labels)
unordered_map<int, int> vertices;             // map: new label index -> original vertex id
set<int> C;                                   // current clique (using new labels)
unordered_map<int, int> vertices_map;         // map: original vertex id -> new label index
int max_clique_size = 0;
int total_maximal_cliques = 0;
unordered_map<int, int> clique_size_distribution;
chrono::high_resolution_clock::time_point start_time;

void update(int i) {
    // Base case: reached beyond the last vertex.
    if (i == n + 1) {
        auto endt = Clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endt - start_time);
        total_maximal_cliques++;
        int cliqueSize = C.size();
        max_clique_size = max(max_clique_size, cliqueSize);
        clique_size_distribution[cliqueSize]++;
        if (total_maximal_cliques % 50000 == 0)
            cout << "Total cliques found: " << total_maximal_cliques
                 << " | Time spent: " << duration.count() / 1000.0 << " s" << endl;
        return;
    } else {
        // Step 1: Partition C relative to N(i)
        set<int> C_intersection_Ni;   // vertices in C that are also in N(i)
        vector<int> C_minus_Ni;       // vertices in C not in N(i)
        vector<int> Ni_minus_C;       // vertices in N(i) that are not in C (only those with value < i)
       
        auto itC = C.begin();
        auto itNi = adj2[i].begin();
        while (itC != C.end() || itNi != adj2[i].end()) {
            if (itC == C.end() || (itNi != adj2[i].end() && *itNi < *itC)) {
                if (*itNi < i)
                    Ni_minus_C.push_back(*itNi);
                ++itNi;
            } else if (itNi == adj2[i].end() || *itC < *itNi) {
                C_minus_Ni.push_back(*itC);
                ++itC;
            } else {
                // Equal: vertex is in both C and N(i)
                C_intersection_Ni.insert(*itC);
                ++itC;
                ++itNi;
            }
        }
       
        // If there is any vertex in C not adjacent to i, first call update(i+1)
        if (!C_minus_Ni.empty()) {
            update(i + 1);
        }
       
        bool flag = true;
        int sizeCIntersectNi = C_intersection_Ni.size();
        int p = C_minus_Ni.size();
       
        // Step 2: For each x in (C ∩ N(i)), update T for neighbors of x not in C and not equal to i.
        vector<int> changedT;
        for (auto x : C_intersection_Ni) {
            unordered_set<int> Nx_minus_C;
            auto itSet = C.begin();
            for (auto nb : adj2[x]) {
                if (itSet == C.end() || nb < *itSet) {
                    Nx_minus_C.insert(nb);
                } else if (*itSet == nb) {
                    ++itSet;
                } else {
                    ++itSet;
                }
            }
            for (auto y : Nx_minus_C) {
                if (y != i) {
                    T[y]++;
                    changedT.push_back(y);
                }
            }
        }
       
        // Step 4: Check for any y in N(i) \ C that is less than i and for which T[y] equals sizeCIntersectNi.
        for (auto y : Ni_minus_C) {
            if (y < i && T[y] == sizeCIntersectNi) {
                flag = false;
                break;
            }
        }
        if (!flag) {
            for (auto y : changedT)
                T[y] = 0;
            return;
        }
       
        // Step 3: For each vertex in (C \ N(i)), update S for neighbors.
        vector<vector<int>> S_updates(p);  // S_updates[j] stores list of vertices updated for j-th vertex in C_minus_Ni.
        for (int j = 0; j < p; j++) {
            int x = C_minus_Ni[j];
            vector<int> Nx_minus_C;
            auto itSet = C.begin();
            for (auto nb : adj2[x]) {
                if (itSet == C.end() || nb < *itSet) {
                    Nx_minus_C.push_back(nb);
                } else if (*itSet == nb) {
                    ++itSet;
                } else {
                    ++itSet;
                }
            }
            for (auto y : Nx_minus_C) {
                S[y]++;
                S_updates[j].push_back(y);
            }
        }
       
        // Step 5 & 6: Lexicographic test on vertices in C \ N(i)
        int jp = (p == 0 ? 0 : C_minus_Ni.back());
        for (int j = 0; j < p; j++) {
            int currentVal = C_minus_Ni[j];
            int k = j + 1;
            for (auto y : S_updates[j]) {
                if (y >= i)
                    break;
                if (y < i && T[y] == sizeCIntersectNi) {
                    bool isFirstJ = false;
                    if (j == 0) {
                        if (y < C_minus_Ni[j])
                            isFirstJ = true;
                    } else {
                        if (y < C_minus_Ni[j] && y >= C_minus_Ni[j - 1])
                            isFirstJ = true;
                    }
                    if (y >= currentVal)
                        S[y] -= 1;
                    else {
                        if (isFirstJ) {
                            if (j == 0) {
                                if (S[y] + k - 1 == p) {
                                    flag = false;
                                    break;
                                }
                            } else if ((S[y] + k - 1 == p) && (y >= C_minus_Ni[j - 1])) {
                                flag = false;
                                break;
                            }
                        }
                    }
                }
            }
            if (!flag)
                break;
        }
        if (!flag) {
            for (auto y : changedT)
                T[y] = 0;
            for (int j = 0; j < p; j++) {
                for (auto y : S_updates[j])
                    S[y] = 0;
            }
            return;
        }
       
        // Step 7: Additional maximality/lexicographic check.
        if (sizeCIntersectNi) {
            for (int y = 1; y < i; y++) {
                if ((S[y] == 0) && (T[y] == sizeCIntersectNi)) {
                    if ((C.count(y) == 0) && (jp < y)) {
                        flag = false;
                        break;
                    }
                }
            }
        } else if (jp < i - 1) {
            flag = false;
        }
       
        // Step 8: Reinitialize T for changed vertices.
        for (auto y : changedT)
            T[y] = 0;
       
        // Step 9: Reinitialize S for the S_updates.
        for (int j = 0; j < p; j++) {
            for (auto y : S_updates[j])
                S[y] = 0;
        }
       
        // Step 10: If all tests pass, update C and recurse.
        if (flag) {
            set<int> originalC = C;
            C = C_intersection_Ni;
            C.insert(i);
            update(i + 1);
            C.erase(i);
            C.insert(C_minus_Ni.begin(), C_minus_Ni.end());
        }
        return;
    }
}

int main() {
    start_time = Clock::now();
    string file;
    cout << "Enter the path of the dataset relative to this file: ";
    cin >> file;
    ifstream infile(file);
    int m;
    infile >> n >> m;
    for (int i = 0; i < m; i++) {
        int u, v;
        infile >> u >> v;
        vert.insert(u);
        vert.insert(v);
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
    infile.close();
   
    // Initialize auxiliary arrays (1-indexed for convenience)
    S.resize(n + 1, 0);
    T.resize(n + 1, 0);
    adj2.resize(n + 1);
   
    // Compute degree for each vertex and sort vertices by degree (nondecreasing order)
    unordered_map<int, int> degree;
    for (auto it : vert) {
        degree[it] = adj[it].size();
    }
   
    vector<int> verticesList;
    for (auto it : vert) {
        verticesList.push_back(it);
    }
    sort(verticesList.begin(), verticesList.end(), [&](int a, int b) {
        return degree[a] < degree[b];
    });
   
    // Build mapping: new label (1-indexed) -> original vertex id
    int idx = 1;
    for (auto v : verticesList) {
        vertices[idx] = v;
        idx++;
    }
    // Build reverse mapping: original id -> new label
    for (int i = 1; i <= n; i++) {
        vertices_map[vertices[i]] = i;
    }
   
    // Build new adjacency list (adj2) using new labels
    for (auto i : vert) {
        for (auto x : adj[i]) {
            adj2[vertices_map[i]].insert(vertices_map[x]);
        }
    }
    cout << "Prepared the vertices according to degree" << endl;
   
    // Start the algorithm with C = {1} and call update starting from vertex 2.
    C = {1};
    update(2);
   
    auto end_time = Clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "\n============= REPORT for Arboricity " << file << " =============\n";
    cout << "1. Largest size of the clique: " << max_clique_size << endl;
    cout << "2. Total number of maximal cliques: " << total_maximal_cliques << endl;
    cout << "3. Execution time: " << duration.count() << " milliseconds" << endl;
    cout << "4. Distribution of different size cliques:" << endl;
   
    // Print distribution in ascending order of clique size
    vector<pair<int, int>> distributionVec(clique_size_distribution.begin(), clique_size_distribution.end());
    sort(distributionVec.begin(), distributionVec.end());
    for (const auto &pair : distributionVec) {
        cout << "   Size " << pair.first << ": " << pair.second << " cliques" << endl;
    }
    cout << "==================================" << endl;
   
    return 0;
}
