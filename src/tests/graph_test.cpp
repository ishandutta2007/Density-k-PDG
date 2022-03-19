#include "../graph.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "iso_stress_test.h"

using namespace testing;

// Helper used in parse_edges().
#define throw_assert(c) \
  if (!(c)) throw std::invalid_argument(#c);

// Returns the text representation of the edges in the graph.
std::string serialize_edges(const Graph& g) {
  std::stringstream oss;
  g.print_concise(oss, false);
  std::string text = oss.str();
  throw_assert(text[text.length() - 1] == '\n');
  return text.substr(0, text.length() - 1);
}

// Returns a graph, constructed from the text representation of the edges. For example from input
//    "{123>2, 013}"
// A 3-graph with 2 edges will be constructed, the first edge is directed with 2 as the head,
// the second edge is undirected.
//
// This is used to facilitate unit tests.
Graph parse_edges(const std::string& text) {
  throw_assert(text[0] == '{');
  throw_assert(text[text.length() - 1] == '}');
  std::string s = text.substr(1, text.length() - 2);
  Graph g;
  size_t prev_pos = 0;
  while (s.length() != 0) {
    size_t pos = s.find(',', prev_pos);
    std::string e = pos == s.npos ? s.substr(prev_pos) : s.substr(prev_pos, pos - prev_pos);
    uint8 vertex_set = 0;
    uint8 head = UNDIRECTED;
    for (size_t i = 0; i < e.length(); i++) {
      char c = e[i];
      if (c == ' ') continue;
      if ('0' <= c && c <= '6') {
        vertex_set |= (1 << (c - '0'));
      }
      if (c == '>') {
        throw_assert(i == e.length() - 2);
        c = e[i + 1];
        throw_assert('0' <= c && c <= '6');
        head = c - '0';
        throw_assert((vertex_set & (1 << head)) != 0);
      }
    }
    throw_assert(__builtin_popcount(vertex_set) == Graph::K);
    g.add_edge(Edge(vertex_set, head));
    if (pos == s.npos) break;
    prev_pos = pos + 1;
  }
  // Verify the parse produced the same graph as the input.
  throw_assert(serialize_edges(g) == text);
  return g;
}

TEST(GraphTest, GraphDataStructure) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{234, 156>5, 123>2, 013}");

  EXPECT_FALSE(g.is_canonical);

  EXPECT_EQ(4, g.edge_count);
  EXPECT_EQ(2, g.undirected_edge_count);
  EXPECT_EQ(g.edges[0].vertex_set, 0b11100);
  EXPECT_EQ(g.edges[0].head_vertex, UNDIRECTED);
  EXPECT_EQ(g.edges[1].vertex_set, 0b1100010);
  EXPECT_EQ(g.edges[1].head_vertex, 5);
  EXPECT_EQ(g.edges[2].vertex_set, 0b1110);
  EXPECT_EQ(g.edges[2].head_vertex, 2);
  EXPECT_EQ(g.edges[3].vertex_set, 0b1011);
  EXPECT_EQ(g.edges[3].head_vertex, UNDIRECTED);
  EXPECT_EQ(serialize_edges(g), "{234, 156>5, 123>2, 013}");

  g.canonicalize();
  EXPECT_EQ(g.vertices[0].degree_tail, 2);
  EXPECT_EQ(g.vertices[0].degree_head, 0);
  EXPECT_EQ(g.vertices[0].degree_undirected, 1);

  EXPECT_EQ(g.vertices[1].degree_tail, 1);
  EXPECT_EQ(g.vertices[1].degree_head, 0);
  EXPECT_EQ(g.vertices[1].degree_undirected, 2);

  EXPECT_EQ(g.vertices[2].degree_tail, 1);
  EXPECT_EQ(g.vertices[2].degree_head, 0);
  EXPECT_EQ(g.vertices[2].degree_undirected, 0);

  EXPECT_EQ(g.vertices[3].degree_tail, 0);
  EXPECT_EQ(g.vertices[3].degree_head, 1);
  EXPECT_EQ(g.vertices[3].degree_undirected, 1);

  EXPECT_EQ(g.vertices[4].degree_tail, 0);
  EXPECT_EQ(g.vertices[4].degree_head, 1);
  EXPECT_EQ(g.vertices[4].degree_undirected, 0);

  EXPECT_EQ(g.vertices[5].degree_tail, 0);
  EXPECT_EQ(g.vertices[5].degree_head, 0);
  EXPECT_EQ(g.vertices[5].degree_undirected, 1);

  EXPECT_EQ(g.vertices[6].degree_tail, 0);
  EXPECT_EQ(g.vertices[6].degree_head, 0);
  EXPECT_EQ(g.vertices[6].degree_undirected, 1);
}

// Utility function to create and initialize T_3.
Graph get_T3() {
  Graph::set_global_graph_info(3, 5);
  Graph g = parse_edges("{013, 123>2, 023, 234>2}");
  g.canonicalize();
  return g;
}

TEST(GraphTest, T3) {
  Graph g = get_T3();

  EXPECT_EQ(4, g.edge_count);
  // Canonicalization: 1->1, 2->3, 3-> 0, 4->2, 0->4.
  EXPECT_EQ(g.edges[0].vertex_set, 0b1011);  // 013>3
  EXPECT_EQ(g.edges[0].head_vertex, 3);
  EXPECT_EQ(g.edges[1].vertex_set, 0b1101);  // 023>3
  EXPECT_EQ(g.edges[1].head_vertex, 3);
  EXPECT_EQ(g.edges[2].vertex_set, 0b10011);  // 014
  EXPECT_EQ(g.edges[2].head_vertex, UNDIRECTED);
  EXPECT_EQ(g.edges[3].vertex_set, 0b11001);  // 034
  EXPECT_EQ(g.edges[3].head_vertex, UNDIRECTED);
  EXPECT_EQ(serialize_edges(g), "{013>3, 023>3, 014, 034}");

  g.canonicalize();

  EXPECT_EQ(g.vertices[0].degree_undirected, 2);
  EXPECT_EQ(g.vertices[0].degree_head, 0);
  EXPECT_EQ(g.vertices[0].degree_tail, 2);

  EXPECT_EQ(g.vertices[1].degree_undirected, 1);
  EXPECT_EQ(g.vertices[1].degree_head, 0);
  EXPECT_EQ(g.vertices[1].degree_tail, 1);

  EXPECT_EQ(g.vertices[2].degree_undirected, 0);
  EXPECT_EQ(g.vertices[2].degree_head, 0);
  EXPECT_EQ(g.vertices[2].degree_tail, 1);

  EXPECT_EQ(g.vertices[3].degree_undirected, 1);
  EXPECT_EQ(g.vertices[3].degree_head, 2);
  EXPECT_EQ(g.vertices[3].degree_tail, 0);

  EXPECT_EQ(g.vertices[4].degree_undirected, 2);
  EXPECT_EQ(g.vertices[4].degree_head, 0);
  EXPECT_EQ(g.vertices[4].degree_tail, 0);
}

TEST(GraphTest, IsomorphicSlow) {
  Graph::set_global_graph_info(3, 5);
  Graph g = parse_edges("{013>3, 023>3, 014, 034}");
  g.finalize_edges();
  Graph h = parse_edges("{014>0, 034>0, 124, 024}");
  h.finalize_edges();
  EXPECT_TRUE(g.is_isomorphic_slow(h));
  EXPECT_TRUE(h.is_isomorphic_slow(g));
}

TEST(GraphTest, PermuteIsomorphic) {
  Graph g = get_T3();
  Graph h;
  int p[5]{0, 1, 2, 3, 4};
  do {
    g.permute_for_testing(p, h);
    EXPECT_TRUE(g.is_isomorphic_slow(h));
    EXPECT_TRUE(h.is_isomorphic_slow(g));

    EXPECT_EQ(g.get_graph_hash(), h.get_graph_hash());
    h.canonicalize();
    EXPECT_TRUE(h.is_isomorphic(g));
    EXPECT_EQ(g.edge_count, 4);
    EXPECT_EQ(g.undirected_edge_count, 2);
  } while (std::next_permutation(p, p + 5));
}

TEST(GraphTest, PermuteCanonical) {
  Graph g = get_T3();
  g.canonicalize();
  Graph h;
  int p[5]{0, 1, 2, 3, 4};
  g.permute_canonical(p, h);
  EXPECT_TRUE(g.is_identical(h));
  EXPECT_EQ(h.edge_count, 4);
  EXPECT_EQ(h.undirected_edge_count, 2);
}

TEST(GraphTest, PermuteCanonical2) {
  Graph::set_global_graph_info(2, 4);
  Graph h, f;
  Graph g = parse_edges("{03, 12, 02>2, 13>3}");
  g.canonicalize();

  int p[4]{0, 1, 3, 2};
  g.permute_canonical(p, h);
  EXPECT_TRUE(g.is_isomorphic(h));
  h.permute_canonical(p, f);
  EXPECT_TRUE(g.is_identical(f));
}

TEST(GraphTest, Canonicalize) {
  Graph g = get_T3();
  for (int v = 0; v < 4; v++) {
    EXPECT_GE(g.vertices[v].get_hash(), g.vertices[v + 1].get_hash());
  }

  Graph h = get_T3();

  EXPECT_TRUE(g.is_canonical);
  EXPECT_TRUE(h.is_canonical);
  EXPECT_EQ(g.get_graph_hash(), h.get_graph_hash());
  EXPECT_TRUE(h.is_isomorphic(g));
  EXPECT_TRUE(g.is_isomorphic(h));
  EXPECT_TRUE(h.is_identical(g));
  EXPECT_TRUE(g.is_identical(h));

  // Canonicalization should be idempotent.
  h.canonicalize();
  EXPECT_TRUE(h.is_canonical);
  EXPECT_EQ(g.get_graph_hash(), h.get_graph_hash());
  EXPECT_TRUE(h.is_isomorphic(g));
}

TEST(GraphTest, Canonicalize2) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{235, 345>4, 245, 456>4}");
  g.canonicalize();

  EXPECT_EQ(g.vertices[0].get_degrees(), 0x020002);
  EXPECT_EQ(g.vertices[1].get_degrees(), 0x010001);
  EXPECT_EQ(g.vertices[2].get_degrees(), 0x010000);
  EXPECT_EQ(g.vertices[3].get_degrees(), 0x000201);
  EXPECT_EQ(g.vertices[4].get_degrees(), 0x000002);

  Graph h = g;
  h.canonicalize();
  EXPECT_EQ(g.get_graph_hash(), h.get_graph_hash());
  EXPECT_TRUE(h.is_canonical);

  Graph f = get_T3();
  Graph::set_global_graph_info(3, 7);
  f.canonicalize();
  EXPECT_EQ(h.get_graph_hash(), f.get_graph_hash());
}

TEST(GraphTest, Canonicalize3) {
  Graph::set_global_graph_info(2, 7);
  Graph g, h;
  g.add_edge(Edge(0b0101, UNDIRECTED));
  g.copy_edges(h);

  h.canonicalize();
  EXPECT_TRUE(h.is_canonical);
}

Graph get_G4() {
  Graph::set_global_graph_info(4, 7);
  Graph g = parse_edges(
      "{0125>5, 0135>5, 0235>5, 0145>5, 1245>1, 0345>5, 2345, 0126>6, 0136>6, 1236>6, 0146>6, "
      "0246>6, 1246>6, 0346>6, 2356>2}");
  g.canonicalize();
  return g;
}

TEST(GraphTest, Copy) {
  Graph g = get_T3();
  g.add_edge(Edge(0b0111, UNDIRECTED));

  g.canonicalize();
  Graph h;
  g.copy_edges(h);
  h.canonicalize();

  EXPECT_EQ(g.get_graph_hash(), h.get_graph_hash());
  EXPECT_TRUE(h.is_isomorphic(g));
  EXPECT_EQ(g.edge_count, h.edge_count);
  EXPECT_EQ(g.undirected_edge_count, 3);
  EXPECT_EQ(g.undirected_edge_count, h.undirected_edge_count);
}

TEST(GraphTest, NonIsomorphic) {
  Graph g = get_T3();

  Graph h;
  g.copy_edges(h);
  h.add_edge(Edge(0b10110, UNDIRECTED));  // 124
  h.canonicalize();

  Graph f;
  g.copy_edges(f);
  f.add_edge(Edge(0b10110, 1));  // 124
  f.canonicalize();

  EXPECT_NE(g.get_graph_hash(), f.get_graph_hash());
  EXPECT_FALSE(f.is_isomorphic(g));
  EXPECT_NE(h.get_graph_hash(), f.get_graph_hash());
  EXPECT_FALSE(f.is_isomorphic(h));
}

TEST(GraphTest, NonIsomorphicWithSameHash) {
  Graph::set_global_graph_info(3, 5);
  Graph g = parse_edges("{012>0, 013>1, 024, 134, 234}");
  Graph h = parse_edges("{012>1, 013>0, 024, 134, 234}");

  // The two graphs have the same hash, but not isomorphic
  g.canonicalize();
  h.canonicalize();
  EXPECT_FALSE(g.is_isomorphic(h));
  EXPECT_FALSE(h.is_isomorphic(g));
  EXPECT_EQ(g.get_graph_hash(), h.get_graph_hash());
}

TEST(GraphTest, IsomorphicWithSameHash) {
  Graph::set_global_graph_info(2, 6);
  Graph g = parse_edges("{02, 12>1, 04>0, 05>5, 15>5, 35>5}");
  Graph h = parse_edges("{02, 12>1, 03>0, 05>5, 15>5, 45>5}");
  g.canonicalize();
  h.canonicalize();
  EXPECT_EQ(g.get_graph_hash(), h.get_graph_hash());
  EXPECT_TRUE(g.is_identical(h));
  EXPECT_TRUE(h.is_isomorphic(g));
}

TEST(GraphTest, IsomorphicNotIdentical) {
  Graph::set_global_graph_info(3, 5);
  Graph g = parse_edges("{013>3, 023, 123, 014, 024>4, 124}");
  Graph h = parse_edges("{013, 023>3, 123, 014>4, 024, 124}");
  g.canonicalize();
  h.canonicalize();

  EXPECT_FALSE(g.is_identical(h));
  EXPECT_FALSE(h.is_identical(g));
  EXPECT_TRUE(g.is_isomorphic(h));
  EXPECT_TRUE(h.is_isomorphic(g));
}

TEST(GraphTest, Isomorphic_B) {
  Graph::set_global_graph_info(2, 3);
  Graph g = parse_edges("{01>0, 02>2, 12>1}");
  Graph h = parse_edges("{01>1, 02>0, 12>2}");
  EXPECT_TRUE(g.is_isomorphic_slow(h));
}

TEST(GraphTest, Isomorphic_C) {
  Graph::set_global_graph_info(5, 6);
  Graph g = parse_edges("{01234>4, 01245>5, 01345>4}");
  Graph h = parse_edges("{01234>4, 01245>4, 01345>5}");
  g.finalize_edges();
  h.finalize_edges();
  EXPECT_TRUE(g.is_isomorphic_slow(h));
  EXPECT_TRUE(h.is_isomorphic_slow(g));

  g.canonicalize();
  h.canonicalize();
  EXPECT_TRUE(g.is_isomorphic(h));
  EXPECT_TRUE(h.is_isomorphic(g));
}

TEST(GraphTest, ContainsT3) {
  Graph g = get_T3();
  Graph h;

  int p[5]{0, 1, 2, 3, 4};
  do {
    g.permute_for_testing(p, h);
    EXPECT_TRUE(h.contains_Tk(p[0]));
    EXPECT_TRUE(h.contains_Tk(p[1]));
    EXPECT_FALSE(h.contains_Tk(p[2]));
    EXPECT_TRUE(h.contains_Tk(p[3]));
    EXPECT_TRUE(h.contains_Tk(p[4]));
  } while (std::next_permutation(p, p + 5));
}

TEST(GraphTest, ContainsT3_B) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{012>0, 013>3, 024>4, 025>0, 045>4, 145>5, 245>4, 345>4}");
  EXPECT_TRUE(g.contains_Tk(5));
  EXPECT_TRUE(g.contains_Tk(4));
  EXPECT_TRUE(g.contains_Tk(2));
  EXPECT_TRUE(g.contains_Tk(0));
  EXPECT_FALSE(g.contains_Tk(1));
  EXPECT_FALSE(g.contains_Tk(3));
  EXPECT_FALSE(g.contains_Tk(6));
}

TEST(GraphTest, ContainsT3_C) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{013>3, 035, 135>3}");
  for (int v = 0; v < 7; v++) {
    EXPECT_FALSE(g.contains_Tk(0));
  }
}

TEST(GraphTest, ContainsT3_D) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{012>0, 013>3, 024>4, 125, 035, 135>3, 245>5, 345>4}");
  for (int v = 0; v < 7; v++) {
    EXPECT_FALSE(g.contains_Tk(0));
  }
}

TEST(GraphTest, ContainsT3_E) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{012>1, 013>3, 024>0, 134>4, 015>1, 125, 235, 045>0, 345>5}");
  for (int v = 0; v < 7; v++) {
    EXPECT_FALSE(g.contains_Tk(0));
  }
}

TEST(GraphTest, ContainsT3_F) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{013>3, 014>4, 024>4, 124, 234>4, 135>5, 045>4, 245>4, 345>4}");
  for (int v = 0; v < 7; v++) {
    EXPECT_FALSE(g.contains_Tk(0));
  }
}

TEST(GraphTest, ContainsT3_G) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{012>1, 013>3, 134, 125>1, 135>5, 235>5, 045>4, 245>4}");
  EXPECT_TRUE(g.contains_Tk(1));
  EXPECT_TRUE(g.contains_Tk(2));
  EXPECT_TRUE(g.contains_Tk(3));
  EXPECT_TRUE(g.contains_Tk(5));
  EXPECT_FALSE(g.contains_Tk(0));
  EXPECT_FALSE(g.contains_Tk(4));
  EXPECT_FALSE(g.contains_Tk(6));
}

TEST(GraphTest, ContainsT3_H) {
  Graph::set_global_graph_info(3, 7);
  Graph g = parse_edges("{012>1, 013>3, 134, 125>1, 135, 235, 045>4, 245>4}");
  EXPECT_TRUE(g.contains_Tk(1));
  EXPECT_TRUE(g.contains_Tk(2));
  EXPECT_TRUE(g.contains_Tk(3));
  EXPECT_TRUE(g.contains_Tk(5));
  EXPECT_FALSE(g.contains_Tk(0));
  EXPECT_FALSE(g.contains_Tk(4));
  EXPECT_FALSE(g.contains_Tk(6));
}

TEST(GraphTest, NotContainsT3) {
  Graph h;
  Graph g = parse_edges("{013, 123, 023}");
  g.canonicalize();

  int p[5]{0, 1, 2, 3, 4};
  do {
    g.permute_for_testing(p, h);
    for (int i = 0; i < 5; i++) {
      EXPECT_FALSE(h.contains_Tk(i));
    }
  } while (std::next_permutation(p, p + 5));
}

TEST(GraphTest, ContainsT2) {
  Graph::set_global_graph_info(2, 5);
  Graph g = parse_edges("{01>0, 12>1, 03>3, 13>3, 04>4, 24>4, 34>4}");
  EXPECT_TRUE(g.contains_Tk(4));
  EXPECT_TRUE(g.contains_Tk(3));
  EXPECT_TRUE(g.contains_Tk(0));
  EXPECT_TRUE(g.contains_Tk(1));
  EXPECT_FALSE(g.contains_Tk(2));
}

TEST(GraphTest, Theta) {
  Graph g = get_T3();
  EXPECT_EQ(g.get_theta(), Fraction(4, 1));

  Graph::set_global_graph_info(2, 5);
  Graph h = parse_edges("{01>0, 12>1, 03>3, 13>3, 04>4, 24>4, 34>4}");
  EXPECT_EQ(h.get_theta(), Fraction(10, 7));

  Graph::set_global_graph_info(2, 5);
  Graph j = parse_edges("{}");
  EXPECT_EQ(j.get_theta(), Fraction(1E8, 1));

  Graph::set_global_graph_info(3, 4);
  Graph k = parse_edges("{012>2, 123}");
  EXPECT_EQ(k.get_theta(), Fraction(3, 1));

  Graph l = parse_edges("{012>2, 123>1, 023>2, 013>0}");
  EXPECT_EQ(l.get_theta(), Fraction(1, 1));
}

TEST(GraphTest, IsomorphicStress) {
  for (int diff = 0; diff <= 3; diff++) {
    for (int n = diff + 2; n <= 3; n++) {
      int k = n - diff;
      IsomorphismStressTest t(k, n);
      t.run();
    }
  }
}