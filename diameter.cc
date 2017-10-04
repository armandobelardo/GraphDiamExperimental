#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/time.h>
#include "diameter.h"

namespace Diameter {
  int GetFastDiam(const vector <pair<int,int> > &edges) {
    return 1;
  }
  int GetBruteDiam(const vector <pair<int,int> > &edges) {
    return 1;
  }
  void PrintGraph(const vector <pair<int,int> > &edges) {
    for (pair<int, int> edge : edges) {
      printf("%d --> %d\n", edge.first, edge.second);
    }
  }
} // end namespace Diameter
