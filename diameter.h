# ifndef DIAMETER_H
# define DIAMETER_H

#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/time.h>

using namespace std;

namespace Diameter {
  int GetBruteDiam(const vector <pair<int,int> > &edges);

  int GetFastDiam(const vector <pair<int,int> > &edges);

  void PrintGraph(const vector <pair<int,int> > &edges);
} // end namespace Diameter
# endif
