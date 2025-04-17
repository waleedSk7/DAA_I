#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <algorithm>
using namespace std;
int currCliqueDepth;
unordered_map<int, int> originalToIndex;
unordered_map<int, int> indexToOriginal;
vector<unordered_set<int>> neighborList;
int vertexCount, edgeCount;
int largestCliqueSize = 0;
int totalMaximalCliques = 0;
unordered_map<int, int> cliqueSizeStats;
unordered_set<int> allVertices;
chrono::high_resolution_clock::time_point beginTime;

void searchClique(unordered_set<int> &subgraph, unordered_set<int> &candidateSet, int pivot)
{
    if (subgraph.empty())
    {
        totalMaximalCliques++;
        int cliqueSize = currCliqueDepth;
        largestCliqueSize = max(largestCliqueSize, cliqueSize);
        cliqueSizeStats[cliqueSize]++;

        auto currentTime = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(currentTime - beginTime);
        if (totalMaximalCliques % 100000 == 0)
            cout << "Found " << totalMaximalCliques << " cliques, elapsed time: "
                 << elapsed.count() / 1000.0 << " s" << endl;
        return;
    }

    unordered_set<int> pivotNeighbors;
    if (pivot == -1)
    {
        pivot = 0;
        int maxCommon = 0;
        for (int v : subgraph)
        {
            int commonCount = 0;
            if (neighborList[v].size() > candidateSet.size())
            {
                for (int cand : candidateSet)
                    if (neighborList[v].find(cand) != neighborList[v].end())
                        commonCount++;
            }
            else
            {
                for (int neighbor : neighborList[v])
                    if (candidateSet.count(neighbor))
                        commonCount++;
            }
            if (commonCount >= maxCommon)
            {
                maxCommon = commonCount;
                pivot = v;
            }
        }
        for (int cand : candidateSet)
            if (neighborList[pivot].find(cand) != neighborList[pivot].end())
                pivotNeighbors.insert(cand);
    }

    unordered_set<int> diffCandidates;
    for (int v : candidateSet)
        if (pivotNeighbors.find(v) == pivotNeighbors.end())
            diffCandidates.insert(v);

    while (!diffCandidates.empty())
    {
        vector<int> toRemove;
        for (int v : diffCandidates)
        {
            currCliqueDepth++;

            unordered_set<int> newSubgraph, newCandidates;
            if (neighborList[v].size() > subgraph.size())
            {
                for (int w : subgraph)
                    if (neighborList[v].find(w) != neighborList[v].end())
                        newSubgraph.insert(w);
            }
            else
            {
                for (int w : neighborList[v])
                    if (subgraph.count(w))
                        newSubgraph.insert(w);
            }
            if (neighborList[v].size() > candidateSet.size())
            {
                for (int w : candidateSet)
                    if (neighborList[v].find(w) != neighborList[v].end())
                        newCandidates.insert(w);
            }
            else
            {
                for (int w : neighborList[v])
                    if (candidateSet.count(w))
                        newCandidates.insert(w);
            }

            searchClique(newSubgraph, newCandidates, -1);

            candidateSet.erase(v);
            toRemove.push_back(v);
            currCliqueDepth--;
        }
        for (int rem : toRemove)
            diffCandidates.erase(rem);
    }
}

int main()
{
    largestCliqueSize = 0;
    totalMaximalCliques = 0;

    string datasetFile;
    cout << "Enter the dataset file path (relative to this file): ";
    cin >> datasetFile;

    ifstream inFile(datasetFile);
    if (!inFile)
    {
        cerr << "Error: Unable to open file " << datasetFile << endl;
        return 1;
    }
    inFile >> vertexCount >> edgeCount;
    cout << "Graph has " << vertexCount << " vertices and " << edgeCount << " edges." << endl;
    unordered_map<int, vector<int>> tempAdj;
    for (int i = 0; i < edgeCount; i++)
    {
        int u, v;
        inFile >> u >> v;
        allVertices.insert(u);
        allVertices.insert(v);
        tempAdj[u].push_back(v);
        tempAdj[v].push_back(u);
    }
    inFile.close();
    neighborList.resize(vertexCount);
    int idx = 0;
    for (int vertex : allVertices)
    {
        indexToOriginal[idx] = vertex;
        originalToIndex[vertex] = idx;
        idx++;
    }
    for (auto &entry : tempAdj)
    {
        int origVertex = entry.first;
        for (int neighbor : entry.second)
        {
            neighborList[originalToIndex[origVertex]].insert(originalToIndex[neighbor]);
        }
    }
    beginTime = chrono::high_resolution_clock::now();
    currCliqueDepth = 0;
    unordered_set<int> fullSubgraph, candidates;
    int initPivot = -1;
    int maxDegree = 0;
    for (int i = 0; i < vertexCount; i++)
    {
        fullSubgraph.insert(i);
        candidates.insert(i);
        if (neighborList[i].size() >= maxDegree)
        {
            maxDegree = neighborList[i].size();
            initPivot = i;
        }
    }
    searchClique(fullSubgraph, candidates, initPivot);
    auto finishTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(finishTime - beginTime);
    cout << "\n============= Tomita Clique Report for dataset: " << datasetFile << " =============\n";
    cout << "1. Maximum clique size: " << largestCliqueSize << endl;
    cout << "2. Total maximal cliques found: " << totalMaximalCliques << endl;
    cout << "3. Execution time: " << duration.count() << " milliseconds" << endl;
    cout << "4. Clique size distribution:" << endl;
    vector<pair<int, int>> distribution(cliqueSizeStats.begin(), cliqueSizeStats.end());
    sort(distribution.begin(), distribution.end());
    for (const auto &entry : distribution)
    {
        cout << "   Size " << entry.first << ": " << entry.second << " cliques" << endl;
    }
    cout << "==================================" << endl;

    return 0;
}
