#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include "diameter.h"
#include "diamrallel.h"

using namespace std;

namespace {
  // Work around for lack of pvector constructor disallowing function ptr call
  enum FuncEnum {SLOW_PARA, PAPER_PARA};

  void GenPath(int pathlen) {
    ofstream pathFile;
    pathFile.open("graphs/path.edges");
    for (int i = 0; i < pathlen; i++) {
      pathFile << i << " " << i + 1 << "\n";
    }
    pathFile.close();
  }

  // Note that if a vertex has no neighbors we must include an empty vector
  // since we use index in outer vector to determine vertex number.
  vector <vector<int> > GenGraph(const vector <pair<int, int> > &edges) {
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

  // TODO(iamabel): normalize how we call functions (enum vs. func ptr)
  pair<int, double> RunTrials(const vector <vector<int> > &adjlist, const function<int(
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

  pair<int, double> RunTrials(const pvector <pvector<int> > &padjlist, FuncEnum func, const int trials) {
    double total_time = 0;
    int diam = 0;

    double start = GetTime();
    for (int i = 0; i < trials; i++) {
      switch (func) {
        case SLOW_PARA:
          diam = Diameter::GetBruteDiamParallel(padjlist);
          break;
        case PAPER_PARA:
          diam = Diameter::GetFastDiamParallel(padjlist);
          break;
        default:
          return make_pair(-1,-1);
      }
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
  bool run_paper = false, run_slow = false, run_para_slow = false, run_para_paper = false;
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
      else if (string(argv[i]) == "--para_paper") run_para_paper = true;
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
  const vector <vector<int> > adjlist = GenGraph(edges);
  pvector< pvector<int> > padjlist;
  if (run_para_slow || run_para_paper) {
     padjlist = Diameter::BuildTSGraph(edges);
  }

  {
    // Return of format (diameter, average time)
    printf("Our graph is from file:  %s\n", filename);

    pair<int, double> fast_diam_time, brute_para_diam_time, brute_diam_time, paper_para_diam_time;
    if (run_paper) {
      fast_diam_time = RunTrials(adjlist, &Diameter::GetFastDiam, trials);
      printf("\nAccording to the solution by @kawatea,"
             " the diameter of the graph is: %d \n\n", fast_diam_time.first);
      printf("This operation from the paper was completed in:               %f seconds \n\n",
             fast_diam_time.second);
    }
    if (run_slow) {
      brute_diam_time = RunTrials(adjlist, &Diameter::GetBruteDiam, trials);
      printf("A trivial, yet exact, solution says"
             " the diameter of the graph is: %d \n\n", brute_diam_time.first);
      printf("This brute force operation was completed in:                  %f seconds \n\n",
             brute_diam_time.second);
    }
    if (run_para_slow) {
      brute_para_diam_time = RunTrials(padjlist, SLOW_PARA, trials);
      printf("The experimental, yet trivial solution says"
             " the diameter of the graph is: %d \n\n", brute_para_diam_time.first);
      printf("This parallelized brute force operation was completed in:     %f seconds \n\n",
             brute_para_diam_time.second);
    }
    if (run_para_paper) {
      paper_para_diam_time = RunTrials(padjlist, PAPER_PARA, trials);
      printf("The experimental, paper-modifying solution says"
             " the diameter of the graph is: %d \n\n", paper_para_diam_time.first);
      printf("This parallelized paper-modifying operation was completed in: %f seconds \n\n",
             paper_para_diam_time.second);
    }
  }
  return 0;
}
