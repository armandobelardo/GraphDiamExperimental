#include <algorithm>
#include <cstdlib>
#include <deque>
#include <omp.h>
#include <stack>
#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include "ForParallelFromBeamer/bitmap.h"
#include "ForParallelFromBeamer/pvector.h"
#include "ForParallelFromBeamer/sliding_queue.h"
#include "diamrallel.h"

using namespace std;

namespace Parallel { // Collection of necessary helper functions from @sbeamer
  // Bottom Up step in BFS from @sbeamer, variable names changed for continuity
  int BottomUp(const pvector <pvector<int> > &radjlist, pvector<int> &distance,
               Bitmap &queue, Bitmap &next) {
    int awake_count = 0;
    next.reset();
    #pragma omp parallel for reduction(+ : awake_count) schedule(dynamic, 1024)
    for (int u=0; u < radjlist.size(); u++) {
      if (distance[u] < 0) { // find unvisited
        for (int v : radjlist[u]) {
          if (queue.get_bit(v)) { // if parent is in the queue
            distance[u] = distance[v] + 1;
            awake_count++;
            next.set_bit(u);
            break;
          }
        }
      }
    }
    return awake_count;
  }

  // Top Down step in BFS from @sbeamer, variable names changed for continuity
  int TopDown(const pvector <pvector<int> > &adjlist, pvector<int> &distance,
              SlidingQueue<int> &queue) {
    int scout_count = 0;
    #pragma omp parallel
    {
      QueueBuffer<int> lqueue(queue);
      #pragma omp for reduction(+ : scout_count)
      for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) {
        int u = *q_iter;
        for (int v : adjlist[u]) {
          int curr_val = distance[v];
          if (curr_val < 0) {
            if (compare_and_swap(distance[v], curr_val, (distance[u] + 1))) {
              lqueue.push_back(v);
              scout_count += -curr_val;
            }
          }
        }
      }
      lqueue.flush();
    }
    return scout_count;
  }

  void QueueToBitmap(const SlidingQueue<int> &queue, Bitmap &bm) {
    #pragma omp parallel for
    for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) {
      int u = *q_iter;
      bm.set_bit_atomic(u);
    }
  }

  void BitmapToQueue(const pvector <pvector<int> > &adjlist, const Bitmap &bm,
                     SlidingQueue<int> &queue) {
    #pragma omp parallel
    {
      QueueBuffer<int> lqueue(queue);
      #pragma omp for
      for (int n=0; n < adjlist.size(); n++)
        if (bm.get_bit(n))
          lqueue.push_back(n);
      lqueue.flush();
    }
    queue.slide_window();
  }
} // end namespace parallel

namespace {
  int NumEdges(const pvector <pvector<int> > &adjlist) {
    int count = 0;
    for (size_t i = 0; i < adjlist.size(); i++) {
      count += adjlist[i].size();
    }
    return count;
  }

  pair<int,int> BFSHeightParallel(const pvector <pvector<int> > &adjlist,
                                  const pvector <pvector<int> > &radjlist,
                                  int source) {
    int alpha = 15, beta = 18;

    pvector<int> distance(adjlist.size(), -1);
    distance[source] = 0;
    SlidingQueue<int> queue(adjlist.size());
    queue.push_back(source);
    queue.slide_window();
    Bitmap curr(adjlist.size());
    curr.reset();
    Bitmap front(adjlist.size());
    front.reset();
    int edges_to_check = NumEdges(adjlist);
    int scout_count = adjlist[source].size();
    while (!queue.empty()) {
      if (scout_count > edges_to_check / alpha) {
        int awake_count, old_awake_count;
        Parallel::QueueToBitmap(queue, front);
        awake_count = queue.size();
        queue.slide_window();
        do {
          old_awake_count = awake_count;
          awake_count = Parallel::BottomUp(radjlist, distance, front, curr);
          front.swap(curr);
        } while ((awake_count >= old_awake_count) ||
                 (awake_count > adjlist.size() / beta));
        Parallel::BitmapToQueue(adjlist, front, queue);
        scout_count = 1;
      } else {
        edges_to_check -= scout_count;
        scout_count = Parallel::TopDown(adjlist, distance, queue);
        queue.slide_window();
      }
    }
    int dist = 0, last_node = 0;
    for (int n = 0; n < distance.size(); n++) {
      dist = max(dist, distance[n]);
      if (distance[n] > dist) {
        last_node = n;
        dist = distance[n];
      }
    }
    return make_pair(dist, last_node);
  }

  pvector<pvector<int> > Transpose(const pvector <pvector<int> > &adjlist) {
    pvector< pvector<int> > transposed(adjlist.size());
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

namespace Diameter{
  pvector <pvector<int> > BuildTSGraph(const vector <pair<int, int> > &edges) {
    int max_node = 0;
    for (pair<int, int> edge : edges) {
      max_node = max({max_node, edge.first + 1, edge.second + 1});
    }
    pvector <pvector<int> > adjlist(max_node);

    for (pair<int, int> edge : edges) {
      adjlist[edge.first].push_back(edge.second);
    }
    return adjlist;
  }

  int GetFastDiamParallel(const pvector <pvector<int> > &adjlist) {
    // Prepare the adjacency list
    pvector <pvector <int> > radjlist = Transpose(adjlist);
    int num_double_sweep = 10, diameter = 0, V = adjlist.size();

    // Decompose the graph into strongly connected components
    pvector <int> scc(V);
    {
        int num_visit = 0, num_scc = 0;
        pvector <int> ord(V, -1);
        pvector <int> low(V);
        pvector <bool> in(V, false);
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
    {
        for (size_t i = 0; i < num_double_sweep; i++) {
            int start = GetRandom(V);

            // forward BFS
            pair<int,int> dist_node = BFSHeightParallel(adjlist, radjlist, start);

            // backward BFS
            start = dist_node.second;
            diameter = dist_node.first;

            diameter = max(diameter, BFSHeightParallel(radjlist, adjlist, start).first);
        }
    }

    // Order vertices
    pvector <pair<long long, int> > order(V);
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
    int qs, qt;
    pvector <int> dist(V, -1);
    pvector <int> queue(V);
    pvector <int> ecc(V, V);
    {
        for (size_t i = 0; i < V; i++) {
            int u = order[i].second;

            if (ecc[u] <= diameter) continue;

            // Refine the eccentricity upper bound
            int ub = 0;
            pvector <pair<int, int> > neighbors;

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
            pair<int,int> dist_node = BFSHeightParallel(adjlist, radjlist, u);
            ecc[u] = dist_node.first;
            diameter = max(diameter, ecc[u]);

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

  int GetBruteDiamParallel(const pvector <pvector<int> > &adjlist) {
    int diameter = 0;
    const pvector <pvector<int> > radjlist = Transpose(adjlist);

    #pragma omp parallel for reduction(max: diameter)
    for (size_t i = 0; i < adjlist.size(); i++) {
      diameter = max(diameter, BFSHeightParallel(adjlist, radjlist, i).first);
    }
    return diameter;
  }
} // end namespace Diameter
