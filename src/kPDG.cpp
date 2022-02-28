#include "graph.h"

int main() {
  cout << sizeof(Graph) << "\n";

  Graph g;
  g.add_edge(0b11100, UNDIRECTED);
  g.add_edge(0b1100010, 5);
  g.add_edge(0b1110, 2);
  g.add_edge(0b1011, UNDIRECTED);
  g.init();
  g.print();
  return 0;
}
