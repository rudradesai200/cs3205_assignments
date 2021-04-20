// A C++ Program to generate test cases for
// an unweighted undirected graph
#include <bits/stdc++.h>
using namespace std;

// Define the number of runs for the test data
// generated
#define RUN 2

// Define the maximum number of vertices of the graph
#define MAX_VERTICES 8

#define MIN_WEIGHTS 3
#define MAX_WEIGHTS 7

// Define the maximum number of edges
#define MAX_EDGES 30

int main()
{
  map<pair<int, int>, pair<int, int>> container;

  // Uncomment the below line to store
  // the test data in a file

  // For random values every time
  srand(time(NULL));

  int NUM;     // Number of Vertices
  int NUMEDGE; // Number of Edges

  for (int i = 1; i <= RUN; i++)
  {
    freopen(("files/inputs/infile" + to_string(i) + ".txt").c_str(), "w", stdout);
    NUM = MAX_VERTICES;

    // Define the maximum number of edges of the graph
    // Since the most dense graph can have N*(N-1)/2 edges
    // where N = nnumber of vertices in the graph
    NUMEDGE = max(20, 1 + rand() % MAX_EDGES);

    // while (NUMEDGE > NUM * (NUM - 1) / 2)
    //   NUMEDGE = 1 + rand() % MAX_EDGES;

    // First print the number of vertices and edges
    printf("%d,%d\n", NUM, NUMEDGE);

    // Then print the edges of the form (a b)
    // where 'a' is connected to 'b'
    for (int j = 1; j <= NUMEDGE; j++)
    {
      int a = rand() % NUM;
      int b = rand() % NUM;
      pair<int, int> p = make_pair(a, b);
      pair<int, int> reverse_p = make_pair(b, a);

      // Search for a random "new" edge everytime
      // Note - In a tree the edge (a, b) is same
      // as the edge (b, a)
      while (container.find(p) != container.end() ||
             container.find(reverse_p) != container.end())
      {
        a = rand() % NUM;
        b = rand() % NUM;
        p = make_pair(a, b);
        reverse_p = make_pair(b, a);
      }
      int w1 = MIN_WEIGHTS + rand() % (MAX_WEIGHTS - MIN_WEIGHTS);
      int w2 = MIN_WEIGHTS + rand() % (MAX_WEIGHTS - MIN_WEIGHTS);
      container[p] = make_pair(min(w1, w2), max(w1, w2));
    }

    for (auto it = container.begin(); it != container.end(); ++it)
      printf("%d,%d,%d,%d\n", it->first.first, it->first.second, it->second.first, it->second.second);

    container.clear();
    printf("\n");
    fclose(stdout);
  }

  // Uncomment the below line to store
  // the test data in a file
  return (0);
}
