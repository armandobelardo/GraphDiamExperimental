#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include "diameter.h"

using namespace std;

namespace {
  vector <vector<int> > GenPath(int pathlen) {
    vector <vector<int> > path;
    for (int i = 0; i < pathlen; i++) {
      path.push_back({i+1});
    }
    return path;
  }

  // TODO(iamabel): Make graph choice a command line option
  const vector <vector<int> > adjlist = {{1,2}, {0,2}, {0,1,3}, {}};
  // const vector <vector<int> > adjlist = {{1,4}, {2, 4}, {1,3}, {2}, {1}};
  // const vector <vector<int> > adjlist = GenPath(1000);

  double GetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
  }

  pair<int, double> RunTrials(const function<int(const vector <vector<int> >)>&
                              func, const int trials) {
    double total_time = 0;
    int diam = 0;

    double start = GetTime();
    for (int i = 0; i < trials; i++) {
      diam = func(adjlist);
      double end = GetTime();
      total_time += end - start;
      start = end;
    }

    return make_pair(diam, total_time/trials);
  }
} // end namespace

int main(int argc, char** argv) {
  int trials = 10;
  for (int i = 1; i < argc; ++i) {
      if (string(argv[i]) == "--trials") {
          if (i + 1 < argc) {
              trials = atoi(argv[i++]);
          } else { // Trial flag called but unspecified
                cerr << "--trials option requires one argument." << endl;
              return 1;
          }
      }
  }

  // Return of format (diameter, average time)
  const pair<int, double> fast_diam_time =
                                    RunTrials(&Diameter::GetFastDiam, trials);
  const pair<int, double> brute_diam_time =
                                    RunTrials(&Diameter::GetBruteDiam, trials);

  printf("Our graph has edges:\n");
  Diameter::PrintGraph(adjlist);

  printf("According to the solution by @kawatea,"
         " the diameter of the graph is: %d \n", fast_diam_time.first);
  printf("A trivial, yet exact, solution says"
         " the diameter of the graph is: %d \n", brute_diam_time.first);
  printf("Both results were the same: %s\n",
         fast_diam_time.first == brute_diam_time.first ? "true" : "false");
  printf("This brute force operation was completed in:    %f seconds \n",
         brute_diam_time.second);
  printf("This operation from the paper was completed in: %f seconds \n",
         fast_diam_time.second);


  return 0;
}
