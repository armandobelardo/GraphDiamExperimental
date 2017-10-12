# ifndef DIAMRALLEL_H
# define DIAMRALLEL_H

#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/time.h>

using namespace std;

namespace Diameter {
  int GetFastDiamParallel(const vector <vector<int> > &adjlist);

  int GetBruteDiamParallel(const vector <vector<int> > &adjlist);

} // end namespace Diameter
# endif
