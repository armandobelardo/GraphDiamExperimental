#include <cstdlib>
#include <deque>
#include <vector>
#include <algorithm>
#include <sys/time.h>
#include "diameter.h"

#define MAX(a,b) a > b? a : b

namespace {
  int BFSHeight(const vector <vector<int> > &adjlist, int source) {
    deque<int> lineup {source};
    bool *visited = new bool[adjlist.size()];
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

  // vector<vector<int> > Transpose(const vector <vector<int> > &adjlist) {
  //   return adjlist;
  // }
} // end namespace

namespace Diameter {
  int GetFastDiam(const vector <vector<int> > &adjlist) {
    return 1;
  }
  int GetBruteDiam(const vector <vector<int> > &adjlist) {
    int diameter = 0;

    for (int i = 0; i < adjlist.size(); i++) {
      diameter = MAX(diameter, BFSHeight(adjlist, i));
    }
    return diameter;
  }
  void PrintGraph(const vector <vector<int> > &adjlist) {
    for (int i = 0; i < adjlist.size(); i++) {
      for (int neighbor : adjlist[i]) {
          printf("%d --> %d\n", i, neighbor);
      }
    }
  }
} // end namespace Diameter
