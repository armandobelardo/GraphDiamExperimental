# ifndef DIAMETER_H
# define DIAMETER_H

#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/time.h>

using namespace std;

namespace Diameter {
  int GetBruteDiam(const vector <vector<int> > &adjlist);

  int GetFastDiam(const vector <vector<int> > &adjlist);

  // See if changing the mode of BFS is any quicker in practice
  int GetPaperMyBFSDiam(const vector <vector<int> > &adjlist);

  void PrintGraph(const vector <vector<int> > &adjlist);
} // end namespace Diameter
# endif
