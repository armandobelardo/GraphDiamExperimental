#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include "diameter.h"
#include "diamrallel.h"

using namespace std;

namespace {
  vector <vector<int> > GenPath(int pathlen) {
    vector <vector<int> > path;
    for (int i = 0; i < pathlen; i++) {
      path.push_back({i+1});
    }
    path.push_back({}); // in path last vertex has no edges
    return path;
  }

  // Note that if a vertex has no neighbors we must include an empty vector
  // since we use index in outer vector to determine vertex number.
  vector <vector<int> > GenGraph(vector <pair<int, int> > edges) {
    int max_node = 0;
    vector <vector<int> > adjlist;
    for (pair<int, int> edge : edges) {
      max_node = max({max_node, edge.first + 1, edge.second + 1});
    }
    adjlist.resize(max_node);

    for (pair<int, int> edge : edges) {
      adjlist[edge.first].push_back(edge.second);
    }
    return adjlist;
  }

  double GetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
  }

  pair<int, double> RunTrials(vector <vector<int> > adjlist, const function<int(
                              const vector <vector<int> >)>& func, const int trials) {
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
  int trials = 10; // attempt to normalize runs
  char *filename = (char *)"graphs/simple.edges";
  bool run_paper = false, run_slow = false, run_para_slow = false;
  for (int i = 1; i < argc; ++i) {
      if (string(argv[i]) == "--trials") {
          if (i + 1 < argc) {
              trials = atoi(argv[++i]);
          } else { // Trial flag called but unspecified
                cerr << "--trials option requires one argument." << endl;
              return 1;
          }
      } else if (string(argv[i]) == "--graph") {
        if (i + 1 < argc) {
            filename = argv[++i];
        } else { // Graph flag called but unspecified
              cerr << "--graph option requires one argument." << endl;
            return 1;
        }
      } else if (string(argv[i]) == "--paper") run_paper = true;
      else if (string(argv[i]) == "--slow") run_slow = true;
      else if (string(argv[i]) == "--para_slow") run_para_slow = true;
  }

  vector <pair<int, int> > edges;
  {
    FILE *in = fopen(filename, "r");

    if (in == NULL) {
        fprintf(stderr, "Can't open edges file\n");
        return -1;
    }

    for (int from, to; fscanf(in, "%d %d", &from, &to) != EOF; ) {
      edges.push_back(std::make_pair(from, to));
    }
    fclose(in);
  }

  const vector <vector<int> > adjlist = GenPath(1000);
  {
    // Return of format (diameter, average time)
    printf("Our graph is from file:  %s\n", filename);

    pair<int, double> fast_diam_time;
    pair<int, double> brute_para_diam_time;
    pair<int, double> brute_diam_time;
    if (run_paper) {
      fast_diam_time = RunTrials(adjlist, &Diameter::GetFastDiam, trials);
      printf("\nAccording to the solution by @kawatea,"
             " the diameter of the graph is: %d \n\n", fast_diam_time.first);
      printf("This operation from the paper was completed in:              %f seconds \n",
             fast_diam_time.second);
    }
    if (run_slow) {
      brute_diam_time = RunTrials(adjlist, &Diameter::GetBruteDiam, trials);
      printf("A trivial, yet exact, solution says"
             " the diameter of the graph is: %d \n\n", brute_diam_time.first);
      printf("This brute force operation was completed in:                 %f seconds \n\n",
             brute_diam_time.second);
    }
    if (run_para_slow) {
      brute_para_diam_time =
                     RunTrials(adjlist, &Diameter::GetBruteDiamParallel, trials);
      printf("The experimental, yet trivial solution says"
             " the diameter of the graph is: %d \n\n", brute_para_diam_time.first);
      printf("This parallelized brute force operation was completed in:    %f seconds \n\n",
             brute_para_diam_time.second);
    }
  }
  return 0;
}
