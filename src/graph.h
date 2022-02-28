#pragma once

#include <bits/stdc++.h>
using namespace std;

using uint8 = unsigned __int8;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

// k-PGD, number of vertices in each edge.
constexpr int K = 2;
// Maximum number of vertices in any graph. Note the code makes assumptions
// that N<=8 by using 8-bit bitmasks. If N>8 the data type must change.
constexpr int N = 7;
// Maximum number of possible edges in a graph. Use static allocation to minimize overhead.
constexpr int MAX_EDGES = 20;

// Special value to indicate an edge is undirected.
constexpr uint8 UNDIRECTED = 0xFF;

// Specifies one edge in the graph. The vertex_set is a bitmasks of all vertices in the edge.
// Example 00001011 means vertices {0,1,3}.
// The head_vertex is the id of the head vertex if the edge is directed,
// or UNDIRECTED if the edge is undirected.
struct Edge {
  uint8 vertex_set;
  uint8 head_vertex;

  Edge() : vertex_set(0), head_vertex(0) {}
  Edge(uint8 vset, uint8 head) : vertex_set(vset), head_vertex(head) {}
};

// Represent the characteristics of a vertex, that is invariant under graph isomorphisms.
struct VertexSignature {
  uint8 degree_undirected;  // Number of undirected edges through this vertex.
  uint8 degree_head;        // Number of directed edges using this vertex as the head.
  uint8 degree_tail;        // Number of directed edges through this vertex but not as head.
  uint8 reserved;           // Not used, for memory alignment purpose.

  // Combined hash code of the signatures (excluding hashes) of neighbors. Algorithm:
  // Let N_u, N_h, N_t be the neighboring vertex sets that correspond to the 3 degree counts above.
  // Within each set, sort by the signature values (without neighbor_hash value) of the vertices.
  // Then combine the hash with this given order.
  uint32 neighbor_hash;

  // Returns a 64-bit hash code to represent the data.
  uint64 get_hash() const { return *reinterpret_cast<const uint64*>(this); }
};

// Represents a k-PDG, with the data structure optimized for computing isomorphisms.
struct Graph {
  // n vertices in this graph: 0, 1, ..., n-1.

  // The hash code is invariant under isomorphisms.
  uint64 hash;

  // The signatures of each vertex
  VertexSignature vertices[N];

  // Number of edges in this graph.
  uint8 edge_count;

  // The edge set in this graph.
  Edge edges[MAX_EDGES];

  Graph() : hash(0), vertices{}, edge_count(0), edges{} {}

  // Returns true if the edge specified by the bitmask of the vertices in the edge is allowed
  // to be added to the graph (this vertex set does not yet exist in the edges).
  bool edge_allowed(uint8 vertices) const;

  // Add an edge to the graph. It's caller's responsibility to make sure this is allowed.
  // And the input is consistent (head is inside the vertex set).
  void add_edge(uint8 vset, uint8 head);

  // Initializes everything in this graph from the edge set.
  void init();

  // Returns true if this graph is isomorphic to the other.
  bool is_isomorphic(const Graph& other) const;

  // Print the graph to the console for debugging purpose.
  void print() const;
};
