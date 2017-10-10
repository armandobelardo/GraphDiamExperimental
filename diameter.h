# ifndef DIAMETER_H
# define DIAMETER_H

#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/time.h>

using namespace std;

namespace Diameter {
  int GetFastDiam(const vector <vector<int> > &adjlist);

  int GetBruteDiam(const vector <vector<int> > &adjlist);

  int GetFastDiamParallel(const vector <vector<int> > &adjlist);

  void PrintGraph(const vector <vector<int> > &adjlist);
} // end namespace Diameter
# endif
