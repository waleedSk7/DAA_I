#include <iostream>
#include <fstream>
#include <unordered_set>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <list>
#include <algorithm>
#include <chrono>
#include <unordered_map>

using namespace std;

#include <csignal>

// Global variables for signal handling
bool processing_large_dataset = false;
ofstream *global_stats_file = nullptr;
unordered_map<int, int> *global_size_dist = nullptr;
long long *global_clique_count = nullptr;
int *global_max_size = nullptr;

// Signal handler for clean termination
void signal_handler(int signal)
{
    if (processing_large_dataset && global_stats_file && global_stats_file->is_open() &&
        global_size_dist && global_clique_count && global_max_size)
    {

        *global_stats_file << "\n----- PARTIAL RESULTS (PROGRAM INTERRUPTED) -----" << endl;
        *global_stats_file << "Number of maximal cliques found: " << *global_clique_count << endl;
        *global_stats_file << "Maximum clique size: " << *global_max_size << endl;
        *global_stats_file << "Distribution of clique sizes:" << endl;

        for (const auto &pair : *global_size_dist)
        {
            *global_stats_file << "Size " << pair.first << ": " << pair.second << " cliques" << endl;
        }

        *global_stats_file << "----------------------------------------------" << endl;
        global_stats_file->flush();
        cout << "Partial results written to statistics file." << endl;
    }

    exit(signal);
}

// My code presumes that the dataset is 0-indexed, and undirected, so for every edge (v,u), the edge (u,v) is also present in the dataset
// Add starting at line 16, replacing your current processGraphFile
void processGraphFile(const string &inputFilename, vector<vector<int>> &adjacency, int &maxNodeId,
                      unordered_map<int, int> &nodeMapping, unordered_map<int, int> &reverseMapping)
{
    // First pass to identify all unique nodes and create mapping
    ifstream preFile(inputFilename);
    string line;
    maxNodeId = 0;
    int nextId = 0;
    unordered_map<int, int> originalToCompressed;

    while (getline(preFile, line))
    {
        stringstream ss(line);
        int fromNode, toNode;
        if (ss >> fromNode >> toNode)
        {
            if (originalToCompressed.find(fromNode) == originalToCompressed.end())
            {
                originalToCompressed[fromNode] = nextId++;
            }
            if (originalToCompressed.find(toNode) == originalToCompressed.end())
            {
                originalToCompressed[toNode] = nextId++;
            }
        }
    }
    preFile.close();

    // Save mapping for future reference
    nodeMapping = originalToCompressed;
    // Create reverse mapping for efficient lookup
    reverseMapping.clear();
    for (const auto &pair : originalToCompressed)
    {
        int originalId = pair.first;
        int compressedId = pair.second;
        reverseMapping[compressedId] = originalId;
    }

    // Use compressed IDs for adjacency list
    adjacency.clear();
    adjacency.resize(nextId);
    maxNodeId = nextId - 1;

    // Second pass to build adjacency list with compressed IDs
    ifstream inFile(inputFilename);
    while (getline(inFile, line))
    {
        stringstream ss(line);
        int fromNode, toNode;
        if (!(ss >> fromNode >> toNode))
        {
            continue;
        }
        if (fromNode >= 0 && toNode >= 0)
        {
            int mappedFrom = originalToCompressed[fromNode];
            int mappedTo = originalToCompressed[toNode];
            adjacency[mappedFrom].push_back(mappedTo);
            adjacency[mappedTo].push_back(mappedFrom);
        }
    }
    inFile.close();

    // Sort adjacency lists for binary search
    for (auto &neighbors : adjacency)
    {
        sort(neighbors.begin(), neighbors.end());
        // Remove duplicates if needed
        neighbors.erase(unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }
}

// used printing during testing of processGraphFile function
void printAdjacencyList(const vector<vector<int>> &adjacency, int limit = 3)
{
    cout << "First " << limit << " entries of adjacency list:" << endl;
    int count = 0;
    for (int i = 0; i < adjacency.size() && count < limit; i++)
    {
        if (!adjacency[i].empty())
        {
            cout << i << ": ";
            for (int neighbor : adjacency[i])
            {
                cout << neighbor << " ";
            }
            cout << endl;
            count++;
        }
    }
}

// Function to compute degeneracy ordering and the degeneracy value
int computeDegeneracy(const vector<vector<int>> &adjacency, vector<int> &ordering, int maxNodeId)
{
    int n = maxNodeId + 1;

    vector<int> degree(n, 0);
    int maxDegree = 0;
    // calculating degree of all vertices at the start, before removing any vertices
    for (int i = 0; i < n; i++)
    {
        degree[i] = adjacency[i].size();
        maxDegree = max(maxDegree, degree[i]);
    }

    // bucket[d] will have all vertices of degree d
    vector<vector<int>> bucket(maxDegree + 1);
    for (int i = 0; i < n; i++)
    {
        bucket[degree[i]].push_back(i);
    }

    ordering.clear();
    ordering.reserve(n);

    vector<bool> removed(n, false); // to keep track of removed vertices

    int degeneracy = 0;

    // Finally to find the vertices in the degeneracy ordering
    // We do this for all vertices
    // one vertex is removed in each iteration of this outermost for loop
    // I am not removing the nodes in order of their values
    // but the number of nodes at max that will finally be removed will be n

    // Track minimum bucket to check to avoid repeatedly scanning empty buckets
    int minDegree = 0;
    for (int i = 0; i < n; i++)
    {
        int d = minDegree;
        while (d <= maxDegree && bucket[d].empty())
        {
            d++;
        }
        // This will happen after the last iteration of the outer loop when all vertices are removed
        if (d > maxDegree)
        {
            break;
        }
        minDegree = d;
        degeneracy = max(degeneracy, d);

        int v = bucket[d].back();
        bucket[d].pop_back();
        removed[v] = true;
        ordering.push_back(v); // this will store the final degeneracy ordering

        // We will have to update degrees of neighbors of v
        // We do this by decreasing the degree of each neighbor of v by 1, assuming that there exists only one edge between v and each of its neighbors
        for (int neighbor : adjacency[v])
        {
            if (neighbor < n && !removed[neighbor])
            {
                // So at this step, I need to remove the neighbor from the bucket of its current degree, and add it to the bucket of degree-1
                // Again assuming that there exists only one edge between v and each of its neighbors
                auto &currentBucket = bucket[degree[neighbor]];
                auto it = find(currentBucket.begin(), currentBucket.end(), neighbor);
                if (it != currentBucket.end())
                {
                    // Swap with last element and pop for O(1) removal
                    *it = currentBucket.back();
                    currentBucket.pop_back();
                }

                minDegree = min(minDegree, degree[neighbor] - 1);

                // Add to new bucket
                degree[neighbor]--;
                bucket[degree[neighbor]].push_back(neighbor);
            }
        }
    }

    return degeneracy;
}

vector<int> intersect(const vector<int> &A, const vector<vector<int>> &adjacency, int v)
{
    vector<int> result;
    result.reserve(min(A.size(), adjacency[v].size()));

    for (int u : A)
    {
        if (binary_search(adjacency[v].begin(), adjacency[v].end(), u))
        {
            result.push_back(u);
        }
    }
    return result;
}

// Calculate elements in A that are not neighbors of the pivot
vector<int> nonNeighbors(const vector<int> &A, const vector<vector<int>> &adjacency, int pivot)
{
    vector<int> result;
    result.reserve(A.size());

    for (int u : A)
    {
        if (!binary_search(adjacency[pivot].begin(), adjacency[pivot].end(), u))
        {
            result.push_back(u);
        }
    }
    return result;
}

// This Pivot selection algorithm is from the paper by Tomita et al.
// More efficient pivot selection
int choosePivot(const vector<int> &P, const vector<int> &X, const vector<vector<int>> &adjacency)
{
    int bestPivot = -1;
    size_t bestCount = 0;

    auto countIntersectionWithP = [&](int u)
    {
        size_t c = 0;
        for (int nbr : adjacency[u])
        {
            if (binary_search(P.begin(), P.end(), nbr))
            {
                ++c;
            }
        }
        return c;
    };

    // Check vertices in P
    for (int u : P)
    {
        size_t c = countIntersectionWithP(u);
        if (c > bestCount || bestPivot == -1)
        {
            bestCount = c;
            bestPivot = u;
        }
    }

    // Check vertices in X
    for (int u : X)
    {
        size_t c = countIntersectionWithP(u);
        if (c > bestCount || bestPivot == -1)
        {
            bestCount = c;
            bestPivot = u;
        }
    }

    return bestPivot;
}

// The implementation follows the algorithm from the given research paper
// The implementation follows the algorithm from the given research paper
void bronKerboschPivot(
    const vector<vector<int>> &adjacency,
    vector<int> &R,
    vector<int> &P,
    vector<int> &X,
    ofstream &statsFile,
    ofstream &cliquesFile,
    long long &cliqueCount,
    int &maxCliqueSize,
    unordered_map<int, int> &sizeDistribution,
    bool writeToFile,
    const unordered_map<int, int> &reverseMapping)
{
    // If P U X = Null, then R is a maximal clique
    if (P.empty() && X.empty())
    {
        // We found a maximal clique
        cliqueCount++;

        // Update statistics
        int currentCliqueSize = R.size();
        maxCliqueSize = max(maxCliqueSize, currentCliqueSize);
        sizeDistribution[currentCliqueSize]++;

        // I will write one clique per line
        // Format: "v1 v2 v3 ..."
        if (writeToFile)
        {
            cliquesFile << "Clique found with " << currentCliqueSize << " vertices: {";
            for (auto v : R)
            {
                // Map back to original node IDs if needed
                // Efficient O(1) lookup of original node ID
                cliquesFile << reverseMapping.at(v) << " ";
            }
            cliquesFile << "}\n";
        }

        return;
    }

    // Choose pivot
    // The algorithm is from the paper by Tomita et al. which has been referenced in the ELS paper to maximize |P intersection neighbors(pivot)|
    int pivot = choosePivot(P, X, adjacency);

    // Explore vertices in P \ neighbor(pivot)
    // Optimization: When creating toExplore, we  reserve space based on size of P
    vector<int> toExplore = nonNeighbors(P, adjacency, pivot);

    for (auto it = toExplore.begin(); it != toExplore.end(); ++it)
    {
        int v = *it;

        // R' = R ∪ {v}
        R.push_back(v);

        // P' = P intersect neighbors(v)
        vector<int> Pprime = intersect(P, adjacency, v);
        // X' = X intersect neighbors(v)
        vector<int> Xprime = intersect(X, adjacency, v);

        // Recursive call
        bronKerboschPivot(adjacency, R, Pprime, Xprime, statsFile, cliquesFile, cliqueCount,
                          maxCliqueSize, sizeDistribution, writeToFile, reverseMapping);

        // Backtrack
        R.pop_back(); // Remove v from R

        // Remove v from P and add to X - optimize by finding position in original loop
        auto pIt = find(P.begin(), P.end(), v);
        if (pIt != P.end())
        {
            P.erase(pIt);
        }
        X.push_back(v);
    }
}

// The implementation follows the algorithm from the given research paper
//  Optimized version of bronKerboschDegeneracy
// Optimized bronKerboschDegeneracy
// Starting at line 264, update function signature and implementation
// The implementation follows the algorithm from the given research paper
// Optimized version of bronKerboschDegeneracy
void bronKerboschDegeneracy(
    const vector<vector<int>> &adjacency,
    const vector<int> &degeneracyOrdering,
    ofstream &statsFile,
    ofstream &cliquesFile,
    const unordered_map<int, int> &reverseMapping,
    bool writeToFile)
{
    long long cliqueCount = 0;
    int maxCliqueSize = 0;
    unordered_map<int, int> sizeDistribution;

    // Set global pointers for signal handler
    global_stats_file = &statsFile;
    global_size_dist = &sizeDistribution;
    global_clique_count = &cliqueCount;
    global_max_size = &maxCliqueSize;
    processing_large_dataset = !writeToFile;

    int n = degeneracyOrdering.size();

    // Progress tracking
    auto startTime = chrono::high_resolution_clock::now();
    auto lastReportTime = startTime;

    // Create reverse lookup for ordering positions
    vector<int> orderingPosition(adjacency.size(), -1);
    for (int i = 0; i < n; i++)
    {
        orderingPosition[degeneracyOrdering[i]] = i;
    }

    for (int i = 0; i < n; i++)
    {
        int v = degeneracyOrdering[i];

        // Report progress
        auto currentTime = chrono::high_resolution_clock::now();
        double elapsedSinceLastReport = chrono::duration<double>(currentTime - lastReportTime).count();

        if (elapsedSinceLastReport >= 300.0 || i % 10000 == 0 || i == 0 || i == n - 1)
        {
            double elapsed = chrono::duration<double>(currentTime - startTime).count();
            cout << "Processing vertex " << i << "/" << n << " ("
                 << (100.0 * i / n) << "%), cliques: " << cliqueCount
                 << ", max size: " << maxCliqueSize
                 << ", elapsed: " << elapsed << "s" << endl;

            // For as-skitter, write the current statistics to file periodically
            if (i > 0 && !writeToFile)
            { // Only do this for as-skitter (when writeToFile is false)
                statsFile << "--- PROGRESS UPDATE (" << (100.0 * i / n) << "% complete) ---" << endl;
                statsFile << "Processed " << i << " of " << n << " vertices" << endl;
                statsFile << "Current number of maximal cliques found: " << cliqueCount << endl;
                statsFile << "Current maximum clique size: " << maxCliqueSize << endl;
                statsFile << "Current distribution of clique sizes:" << endl;
                for (const auto &pair : sizeDistribution)
                {
                    statsFile << "Size " << pair.first << ": " << pair.second << " cliques" << endl;
                }
                statsFile << "Time elapsed: " << elapsed << " seconds" << endl;
                statsFile << "----------------------------------------------" << endl
                          << endl;
                statsFile.flush(); // Ensure data is written to disk
            }

            lastReportTime = currentTime;
        }

        vector<int> P, X, R;
        P.reserve(adjacency[v].size());
        X.reserve(adjacency[v].size());

        for (int neighbor : adjacency[v])
        {
            int neighborPos = orderingPosition[neighbor];
            if (neighborPos == -1)
                continue;

            if (neighborPos > i)
            {
                P.push_back(neighbor);
            }
            else if (neighborPos < i)
            {
                X.push_back(neighbor);
            }
        }

        sort(P.begin(), P.end());
        sort(X.begin(), X.end());

        R.push_back(v);
        bronKerboschPivot(adjacency, R, P, X, statsFile, cliquesFile, cliqueCount,
                          maxCliqueSize, sizeDistribution, writeToFile, reverseMapping);
    }

    // Format the execution time nicely
    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = endTime - startTime;
    double elapsedSeconds = duration.count();
    double elapsedMs = elapsedSeconds * 1000.0;

    // Write final statistics
    statsFile << "\n----- FINAL RESULTS -----" << endl;
    statsFile << "Found " << cliqueCount << " maximal cliques in "
              << elapsedMs << " ms (" << elapsedSeconds << " seconds)" << endl;
    statsFile << "Largest clique size: " << maxCliqueSize << endl;
    statsFile << "Distribution of clique sizes:" << endl;

    // Sort the distribution by clique size for nicer output
    vector<pair<int, int>> sortedDistribution(sizeDistribution.begin(), sizeDistribution.end());
    sort(sortedDistribution.begin(), sortedDistribution.end());

    for (const auto &pair : sortedDistribution)
    {
        statsFile << "Clique size " << pair.first << ": " << pair.second << " cliques" << endl;
    }

    statsFile << "Total execution time: " << (int)elapsedSeconds << " seconds" << endl;
    statsFile << "-----------------------" << endl;

    // Also output to console
    cout << "Found " << cliqueCount << " maximal cliques in " << elapsedSeconds << " seconds" << endl;
    cout << "Largest clique size: " << maxCliqueSize << endl;

    // Reset global pointers
    global_stats_file = nullptr;
    global_size_dist = nullptr;
    global_clique_count = nullptr;
    global_max_size = nullptr;
    processing_large_dataset = false;
}

int main()
{
    // Register signal handler for clean termination
    signal(SIGINT, signal_handler);

    vector<string> inputFilenames = {
        "test_case.txt",
        "Wiki-Vote-clean.txt",
        "Email-Enron-Clean.txt",
        "as-skitter-clean.txt"};

    vector<string> outputFilenames = {
        "maximal_cliques_testcase.txt",
        "maximal_cliques_wiki.txt",
        "maximal_cliques_emailenron.txt",
        "maximal_cliques_asskitter.txt"};

    bool writeToFile = true;

    for (int fileIndex = 0; fileIndex < inputFilenames.size(); fileIndex++)
    {
        string inputFilename = inputFilenames[fileIndex];
        cout << "\nProcessing file: " << inputFilename << endl;

        // Special case for large datasets
        if (inputFilename == "as-skitter-clean.txt")
        {
            //writeToFile = false;
            //cout << "Large dataset detected. Only statistics will be collected (no individual cliques output)." << endl;
        }
        else
        {
            writeToFile = true;
        }

        vector<vector<int>> adjacency;
        int maxNodeId = 0;
        unordered_map<int, int> nodeMapping;
        unordered_map<int, int> reverseMapping;

        processGraphFile(inputFilename, adjacency, maxNodeId, nodeMapping, reverseMapping);

        cout << "Graph loaded. Node count: " << adjacency.size() << endl;

        // Compute stats about the graph
        long edgeCount = 0;
        int maxDegree = 0;
        for (const auto &neighbors : adjacency)
        {
            edgeCount += neighbors.size();
            maxDegree = max(maxDegree, (int)neighbors.size());
        }
        cout << "Edge count: " << edgeCount / 2 << ", Max degree: " << maxDegree << endl;

        vector<int> degeneracyOrdering;
        int degeneracy = computeDegeneracy(adjacency, degeneracyOrdering, maxNodeId);

        cout << "Degeneracy of the graph: " << degeneracy << endl;
        cout << "First 10 vertices in the degeneracy ordering: ";
        for (int i = 0; i < min(10, (int)degeneracyOrdering.size()); i++)
        {
            cout << reverseMapping.at(degeneracyOrdering[i]) << " ";
        }
        cout << endl;

        // Create two separate output files for each dataset
        string statsFilename = "statistics_" + inputFilenames[fileIndex];
        string cliquesFilename = "cliques_" + outputFilenames[fileIndex];

        ofstream statsFile(statsFilename);
        ofstream cliquesFile;

        if (writeToFile)
        {
            cliquesFile.open(cliquesFilename);
            if (!cliquesFile.is_open())
            {
                cerr << "Error: could not open cliques file for writing.\n";
                continue;
            }
        }

        if (!statsFile.is_open())
        {
            cerr << "Error: could not open statistics file for writing.\n";
            continue;
        }

        // Write initial statistics to stats file
        statsFile << "Reading graph from: " << inputFilename << endl;
        statsFile << "Graph loaded with " << adjacency.size() << " vertices" << endl;
        statsFile << "Total edges: " << edgeCount / 2 << endl;
        statsFile << "Maximum degree: " << maxDegree << endl;
        statsFile << "Degeneracy of the graph: " << degeneracy << endl
                  << endl;
        statsFile << "First 10 vertices in the degeneracy ordering: ";

        for (int i = 0; i < min(10, (int)degeneracyOrdering.size()); i++)
        {
            statsFile << reverseMapping.at(degeneracyOrdering[i]) << " ";
        }
        statsFile << endl
                  << endl;
        statsFile << "Finding all maximal cliques..." << endl
                  << endl;

        auto startTime = chrono::high_resolution_clock::now();
        bronKerboschDegeneracy(adjacency, degeneracyOrdering, statsFile, cliquesFile,
                               reverseMapping, writeToFile);
        auto endTime = chrono::high_resolution_clock::now();

        double elapsedSeconds = chrono::duration<double>(endTime - startTime).count();
        statsFile << "\nBronKerboschDegeneracy completed in " << elapsedSeconds << " seconds.\n";

        if (writeToFile)
        {
            cliquesFile.close();
        }
        statsFile.close();

        cout << "BronKerboschDegeneracy completed in " << elapsedSeconds << " seconds.\n";
        cout << "Results written to " << statsFilename;
        if (writeToFile)
        {
            cout << " and " << cliquesFilename;
        }
        cout << endl;
    }

    return 0;
}