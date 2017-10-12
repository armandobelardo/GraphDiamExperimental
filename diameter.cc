#include <algorithm>
#include <cstdlib>
#include <deque>
#include <omp.h>
#include <stack>
#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include "diameter.h"

namespace {
  int BFSHeight(const vector <vector<int> > &adjlist, int source) {
    deque<int> lineup {source};
    vector<bool> visited(adjlist.size(), false);
    visited[source] = true;
    // init height to -1 since the algo counts the root as a level though it is
    // technically at height 0.
    int height = -1, lastInLevel = source;

    while (!lineup.empty()) {
      int curr = lineup.front();
      lineup.pop_front();

      for (int neighbor : adjlist[curr]) {
        if (!visited[neighbor]) {
          visited[neighbor] = true;
          lineup.push_back(neighbor);
        }
      }
      if (lastInLevel == curr) {
        height++;
        lastInLevel = lineup.back();
      }
    }
    return height;
  }

  vector<vector<int> > Transpose(const vector <vector<int> > &adjlist) {
    vector< vector<int> > transposed(adjlist.size());
    for (size_t i = 0; i < adjlist.size(); i++) {
      for (int neighbor : adjlist[i]) {
        transposed[neighbor].push_back(i);
      }
    }
    return transposed;
  }

  int GetRandom(int V) {
      static unsigned long long x = 123456789;
      static unsigned long long y = 362436039;
      static unsigned long long z = 521288629;
      static unsigned long long w = 88675123;
      unsigned long long t;

      t = x ^ (x << 11);
      x = y;
      y = z;
      z = w;
      w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));

      return  w % V;
  }
} // end namespace

namespace Diameter {
  // Code as from @kawatea on GitHub <3
  int GetFastDiam(const vector <vector<int> > &adjlist) {
    // Prepare the adjacency list
    vector <vector <int> > radjlist = Transpose(adjlist);
    int num_double_sweep = 10, diameter = 0, V = adjlist.size();

    // Decompose the graph into strongly connected components
    vector <int> scc(V);
    {
        int num_visit = 0, num_scc = 0;
        vector <int> ord(V, -1);
        vector <int> low(V);
        vector <bool> in(V, false);
        stack <int> s;
        stack <pair<int, int> > dfs;

        for (size_t i = 0; i < V; i++) {
            if (ord[i] != -1) continue;

            dfs.push(make_pair(i, -1));

            while (!dfs.empty()) {
                int v = dfs.top().first;
                size_t index = dfs.top().second;
                dfs.pop();

                if (index == -1) {
                    ord[v] = low[v] = num_visit++;
                    s.push(v);
                    in[v] = true;
                } else {
                    low[v] = min(low[v], low[adjlist[v][index]]);
                }
                for (index++; index < (int)adjlist[v].size(); index++) {
                    int w = adjlist[v][index];

                    if (ord[w] == -1) {
                        dfs.push(make_pair(v, index));
                        dfs.push(make_pair(w, -1));
                        break;
                    } else if (in[w] == true) {
                        low[v] = min(low[v], ord[w]);
                    }
                }
                if (index == (int)adjlist[v].size() && low[v] == ord[v]) {
                    while (true) {
                        int w = s.top();

                        s.pop();
                        in[w] = false;
                        scc[w] = num_scc;

                        if (v == w) break;
                    }
                    num_scc++;
                }
            }
        }
    }

    // Compute the diameter lower bound by the double sweep algorithm
    int qs, qt;
    vector <int> dist(V, -1);
    vector <int> queue(V);
    {
        for (size_t i = 0; i < num_double_sweep; i++) {
            int start = GetRandom(V);

            // forward BFS
            qs = qt = 0;
            dist[start] = 0;
            queue[qt++] = start;

            while (qs < qt) {
                int v = queue[qs++];

                for (size_t j = 0; j < adjlist[v].size(); j++) {
                    if (dist[adjlist[v][j]] < 0) {
                        dist[adjlist[v][j]] = dist[v] + 1;
                        queue[qt++] = adjlist[v][j];
                    }
                }
            }

            for (int j = 0; j < qt; j++) dist[queue[j]] = -1;

            // backward BFS
            start = queue[qt - 1];
            qs = qt = 0;
            dist[start] = 0;
            queue[qt++] = start;

            while (qs < qt) {
                int v = queue[qs++];

                for (size_t j = 0; j < radjlist[v].size(); j++) {
                    if (dist[radjlist[v][j]] < 0) {
                        dist[radjlist[v][j]] = dist[v] + 1;
                        queue[qt++] = radjlist[v][j];
                    }
                }
            }

            diameter = max(diameter, dist[queue[qt - 1]]);

            for (int j = 0; j < qt; j++) dist[queue[j]] = -1;
        }
    }

    // Order vertices
    vector <pair<long long, int> > order(V);
    {
        for (int v = 0; v < V; v++) {
            size_t in = 0, out = 0;

            for (size_t i = 0; i < radjlist[v].size(); i++) {
                if (scc[radjlist[v][i]] == scc[v]) in++;
            }

            for (size_t i = 0; i < adjlist[v].size(); i++) {
                if (scc[adjlist[v][i]] == scc[v]) out++;
            }

            // SCC : reverse topological order
            // inside an SCC : decreasing order of the product of the indegree and outdegree for vertices in the same SCC
            order[v] = make_pair(((long long)scc[v] << 32) - in * out, v);
        }

        sort(order.begin(), order.end());
    }

    // Examine every vertex
    vector <int> ecc(V, V);
    {
        for (size_t i = 0; i < V; i++) {
            int u = order[i].second;

            if (ecc[u] <= diameter) continue;

            // Refine the eccentricity upper bound
            int ub = 0;
            vector <pair<int, int> > neighbors;

            for (size_t j = 0; j < adjlist[u].size(); j++) neighbors.push_back(make_pair(scc[adjlist[u][j]], ecc[adjlist[u][j]] + 1));

            sort(neighbors.begin(), neighbors.end());

            for (size_t j = 0; j < neighbors.size(); ) {
                int component = neighbors[j].first;
                int lb = V;

                for (; j < neighbors.size(); j++) {
                    if (neighbors[j].first != component) break;
                    lb = min(lb, neighbors[j].second);
                }

                ub = max(ub, lb);

                if (ub > diameter) break;
            }

            if (ub <= diameter) {
                ecc[u] = ub;
                continue;
            }

            // Conduct a BFS and update bounds
            qs = qt = 0;
            dist[u] = 0;
            queue[qt++] = u;

            while (qs < qt) {
                int v = queue[qs++];

                for (size_t j = 0; j < adjlist[v].size(); j++) {
                    if (dist[adjlist[v][j]] < 0) {
                        dist[adjlist[v][j]] = dist[v] + 1;
                        queue[qt++] = adjlist[v][j];
                    }
                }
            }

            ecc[u] = dist[queue[qt - 1]];
            diameter = max(diameter, ecc[u]);

            for (int j = 0; j < qt; j++) dist[queue[j]] = -1;

            qs = qt = 0;
            dist[u] = 0;
            queue[qt++] = u;

            while (qs < qt) {
                int v = queue[qs++];

                ecc[v] = min(ecc[v], dist[v] + ecc[u]);

                for (size_t j = 0; j < radjlist[v].size(); j++) {
                    // only inside an SCC
                    if (dist[radjlist[v][j]] < 0 && scc[radjlist[v][j]] == scc[u]) {
                        dist[radjlist[v][j]] = dist[v] + 1;
                        queue[qt++] = radjlist[v][j];
                    }
                }
            }

            for (int j = 0; j < qt; j++) dist[queue[j]] = -1;
        }
    }
    return diameter;
  }

  int GetBruteDiam(const vector <vector<int> > &adjlist) {
    int diameter = 0;

    for (size_t i = 0; i < 1; i++) {
      diameter = max(diameter, BFSHeight(adjlist, i));
    }
    return diameter;
  }

  void PrintGraph(const vector <vector<int> > &adjlist) {
    for (size_t i = 0; i < adjlist.size(); i++) {
      for (int neighbor : adjlist[i]) {
          printf("%d --> %d\n", i, neighbor);
      }
    }
  }
} // end namespace Diameter
