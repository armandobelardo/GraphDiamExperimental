# ifndef DIAMRALLEL_H
# define DIAMRALLEL_H

#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/time.h>
#include "ForParallelFromBeamer/pvector.h"

using namespace std;

namespace Diameter {
  int GetFastDiamParallel(const vector <vector<int> > &adjlist);

  int GetBruteDiamParallel(const pvector <pvector<int> > &adjlist);

  // Build thread-safe graph
  pvector <pvector<int> > BuildTSGraph(const vector <pair<int, int> > &edges);

} // end namespace Diameter
# endif
