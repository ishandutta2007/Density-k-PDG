#include "edge_gen.h"

#include "counters.h"
constexpr uint8 NOT_IN_SET = 0xEE;

// Initializes the generator for the given new vertex count.
// vertex_count = number of vertices to grow to in each new graph.
EdgeGenerator::EdgeGenerator(int vertex_count, const Graph& base_graph)
    : base(base_graph),
      n(vertex_count),
      high_idx_non_zero_enum_state(0),
      edge_candidate_count(0),
      stats_tk_skip(0),
      stats_tk_skip_bits(0),
      stats_theta_edges_skip(0),
      stats_theta_directed_edges_skip(0),
      stats_edge_sets(0) {
  for (uint8 mask = 0; mask < (1 << (n - 1)); mask++) {
    if (__builtin_popcount(mask) == Graph::K - 1) {
      edge_candidates[edge_candidate_count] = (mask | (1 << (n - 1)));
      int vidx = 0;
      edge_candidates_vidx[edge_candidate_count][vidx++] = NOT_IN_SET;
      edge_candidates_vidx[edge_candidate_count][vidx++] = UNDIRECTED;
      for (uint8 i = 0; i <= n - 1; i++) {
        if ((edge_candidates[edge_candidate_count] & (1 << i)) != 0) {
          edge_candidates_vidx[edge_candidate_count][vidx++] = i;
        }
      }
      assert(vidx == Graph::K + 2);
      ++edge_candidate_count;
    }
  }
  assert(edge_candidate_count == compute_binom(n - 1, Graph::K - 1));
  for (uint8 i = 0; i < edge_candidate_count; i++) {
    enum_state[i] = 0;
  }
}
void EdgeGenerator::clear_stats() {
  stats_tk_skip = stats_tk_skip_bits = stats_theta_edges_skip = stats_theta_directed_edges_skip =
      stats_edge_sets = 0;
}

// Generates the next edge set. Returns true enumeration should proceed,
// in which case `copy` is the newly generated graph. (`copy` doesn't need to be clean before
// calling this function, all its state will be reset.)
// Returns false if all possibilities have already been enumerated.
//
// use_known_min_theta_opt = whether min_theta optimization should be used. If false,
//    the rest of the paramters are all ignored.
// base_edge_count = number of edges in the base graph.
// base_directed_edge_count = number of directed edges in the base graph.
// known_min_theta = the currently know min_theta value.
//
// The min_theta optimization: this should only be used in the final enumeration phase, not
// in the growth phase before the final step.
//
// The idea is, if the graph is too sparse, then its theta is guaranteed to be larger than
// the currently known min_theta value, in which case we don't care about this graph since
// it won't give us a better min_theta value regardless whether the graph is T_k free.
bool EdgeGenerator::next(Graph& copy, bool use_known_min_theta_opt, int base_edge_count,
                         int base_directed_edge_count, Fraction known_min_theta) {
  if (use_known_min_theta_opt) {
    // Assert that we are using min_theta optimization only in the final enumeration phase.
    assert(n == Graph::N);
    assert(base_edge_count >= 0);
    assert(base_directed_edge_count >= 0);
    assert(known_min_theta >= Fraction(1, 1));
  }

  while (true) {
    bool has_valid_candidate = false;
    for (uint8 i = 0; i < edge_candidate_count; i++) {
      ++enum_state[i];
      high_idx_non_zero_enum_state = std::max(high_idx_non_zero_enum_state, i);
      if (enum_state[i] != Graph::K + 2) {
        has_valid_candidate = true;
        break;
      }
      enum_state[i] = 0;
    }
    // If we didn't find a valid candidate, the enumeration of all possible
    // edge combinations are done, we can return false.
    if (!has_valid_candidate) return false;

    // If we are not using min_theta optimization, since we already have a candidate, break out
    // of the while loop to return the current candidate.
    if (!use_known_min_theta_opt) break;

    // Next we perform the min_theta optimization.
    OptResult opt =
        perform_min_theta_optimization(base_edge_count, base_directed_edge_count, known_min_theta);
    if (opt == OptResult::FOUND_CANDIDATE)
      break;  // Passed the min_theta optimization check, we have a winner.
    else if (opt == OptResult::DONE)
      return false;
    else
      continue;
  }

  // We found a new valid enumeration state. Generate a new graph into `copy`.
  generate_graph(copy, 0);
  ++stats_edge_sets;
  return true;
}
void EdgeGenerator::generate_graph(Graph& copy, int skip_front) const {
  base.copy_edges(copy);
  for (uint8 j = skip_front; j <= high_idx_non_zero_enum_state; j++) {
    if (enum_state[j] != 0) {
      copy.add_edge(Edge(edge_candidates[j], edge_candidates_vidx[j][enum_state[j]]));
    }
  }
}

EdgeGenerator::OptResult EdgeGenerator::perform_min_theta_optimization(int base_edge_count,
                                                                       int base_directed_edge_count,
                                                                       Fraction known_min_theta) {
  // The number of new edges must satisfy the following inequality in order
  // for it to be interesting:
  //    base_edge_count + known_min_theta * (base_directed_edges + new_edges) > binom_nk
  // because otherwise, even if all new edges are directed, the theta produced would still
  // be >= min_theta. Rewrite the inequility as
  //    new_edges > (binom_nk - base_edge_count) / known_min_theta - base_directed_edges
  // Thus
  //    new_edges > floor((binom_nk - base_edge_count) / known_min_theta) - base_directed_edges
  int new_edge_threshold =
      (Graph::TOTAL_EDGES - base_edge_count) * known_min_theta.d / known_min_theta.n -
      base_directed_edge_count;
  // bool debug_print = true;
  // if (debug_print) {
  //   std::cout << "\n**** threshold=" << new_edge_threshold << ", binom=" << (int)binom_nk
  //             << ", t=" << known_min_theta.n << "/" << known_min_theta.d
  //             << ", b=" << base_edge_count << ", " << base_directed_edge_count << "\n";
  // }

  std::tuple<uint8, uint8, uint8, uint8> new_edge_info = count_edges();
  uint8 new_edges = std::get<0>(new_edge_info);
  uint8 new_directed_edges = std::get<1>(new_edge_info);
  uint8 low_non_edge_idx = std::get<2>(new_edge_info);
  uint8 low_non_directed_idx = std::get<3>(new_edge_info);

  // if (debug_print) {
  //   std::cout << "New Edges " << (int)new_edges << ", " << (int)new_directed_edges << ", "
  //             << (int)low_non_edge_idx << ", " << (int)low_non_directed_idx << "\n";
  // }

  // First step: check number of new edges. Details of this inequality check are described above
  // in the `new_edge_threshold` calculation.
  if (new_edges <= new_edge_threshold) {
    ++stats_theta_edges_skip;
    // Here we can jump forward, to the state where it's ready to add another edge.
    // For example if the current enum state is [3, 0, 0, 1, 1, 1], and there are not
    // enough new edges, then the next enum state that may have enough new edges
    // is NOT [3, 0, 0, 1, 1, 2] (the normal increment), but rather, [3, 0, 1, 1, 1, 1].
    // Since the begin of the for loop above does increment, we put the enum state to
    // [3, 0, 1, 1, 1, 0], to prepare for the for loop increment to do the job.
    if (low_non_edge_idx >= edge_candidate_count) {
      // In this case, all edges are present and we still don't have enough edges, simply
      // return DONE to terminate the generation.
      return OptResult::DONE;
    }
    for (uint8 i = 1; i <= low_non_edge_idx; i++) {
      enum_state[i] = 1;
    }
    enum_state[0] = 0;
    return OptResult::CONTINUE_SEARCH;
    // if (low_non_edge_idx > 0) {
    //   enum_state[low_non_edge_idx] = 1;
    //   for (uint8 i = 0; i < low_non_edge_idx; i++) {
    //     enum_state[i] = 0;
    //   }
    // }
    // return OptResult::FOUND_CANDIDATE;
  }
  // If we get here, we have enough number of edges. But there still may not be enough number
  // of directed edges. So just compute.
  int total_directed = new_directed_edges + base_directed_edge_count;
  int total_undirected =
      new_edges - new_directed_edges + base_edge_count - base_directed_edge_count;
  if (total_directed == 0 ||
      known_min_theta <= Fraction(Graph::TOTAL_EDGES - total_undirected, total_directed)) {
    ++stats_theta_directed_edges_skip;
    // Here we can jump forward similar to above when we don't have enough total edges,
    // to the state where it's ready to add another directed edge.
    // For example if the current enum state is [3, 0, 0, 2, 2, 2], and there are not
    // enough directed edges, then the next enum state that may have enough new edges
    // is NOT [3, 0, 0, 2, 2, 3] (the normal increment), but rather, [3, 0, 2, 2, 2, 2].
    // Since the begin of the for loop above does increment, we put the enum state to
    // [3, 0, 2, 2, 2, 1], to prepare for the for loop increment to do the job.
    if (low_non_directed_idx >= edge_candidate_count) {
      // In this case, all edges are present and directed and we still don't have enough directed
      // edges, simply return DONE to terminate the generation.
      return OptResult::DONE;
    }
    for (uint8 i = 1; i <= low_non_directed_idx; i++) {
      enum_state[i] = 2;
    }
    enum_state[0] = 1;
    return OptResult::CONTINUE_SEARCH;
    // if (low_non_directed_idx > 0) {
    //   enum_state[low_non_directed_idx] = 2;
    //   for (uint8 i = 0; i < low_non_directed_idx; i++) {
    //     enum_state[i] = 0;
    //   }
    // }
    // return OptResult::FOUND_CANDIDATE;
  }

  // Reaching this point means we passed the check and have a valid candidate.
  return OptResult::FOUND_CANDIDATE;
}

// Notify the generator about the fact that adding the current edge set to the graph
// makes it contain T_k, and therefore we can skip edge sets that are supersets of the current.
void EdgeGenerator::notify_contain_tk_skip() {
  ++stats_tk_skip;
  if (enum_state[0] == 0) {
    // Find the lowest non-zero enum state, and change everything below
    // it to the final state. Then the next() call will bump the lowest non-zero enum state.
    // For example, if the enum state is [3,0,0,1,0,0,0], update it to [3,0,0,1,k+1,k+1,k+1]
    // then the next call will get to [3,0,0,2,0,0,0].
    for (uint8 i = 0; i < edge_candidate_count; i++) {
      if (enum_state[i] != 0) return;
      enum_state[i] = Graph::K + 1;
      ++stats_tk_skip_bits;
    }
  } else if (n == Graph::N) {
    Graph copy;
    for (int skip_front = 1; skip_front < edge_candidate_count; skip_front++) {
      generate_graph(copy, skip_front);
      if (copy.get_edge_count() == base.get_edge_count()) return;
      if (copy.contains_Tk(Graph::N - 1)) {
        enum_state[skip_front - 1] = Graph::K + 1;
        ++stats_tk_skip_bits;
      }
    }
  }
}

// Returns a tuple:
//    first = number of edges in the current state,
//    second = number of directed edges in the current state,
//    third = lowest index in enum_state where the edge is not in the set.
//    forth = lowest index in enum_state where the edge is undirected or not in the set.
std::tuple<uint8, uint8, uint8, uint8> EdgeGenerator::count_edges() const {
  uint8 edges = 0;
  uint8 directed = 0;
  uint8 first_non_edge = edge_candidate_count;
  uint8 first_non_directed_edge = edge_candidate_count;
  for (int i = 0; i < edge_candidate_count; i++) {
    if (enum_state[i] != 0) {
      ++edges;
      if (enum_state[i] != 1) {
        ++directed;
      }
    } else if (first_non_edge == edge_candidate_count) {
      first_non_edge = i;
    }

    if (first_non_directed_edge == edge_candidate_count &&
        (enum_state[i] == 0 || enum_state[i] == 1)) {
      first_non_directed_edge = i;
    }
  }
  return std::make_tuple(edges, directed, first_non_edge, first_non_directed_edge);
}

void EdgeGenerator::print_debug(bool print_candidates) const {
  std::cout << "    EdgeGen[" << static_cast<int>(n)
            << ", cand_count=" << static_cast<int>(edge_candidate_count)
            << ", high_idx=" << static_cast<int>(high_idx_non_zero_enum_state) << ", state=";
  for (int e = 0; e < edge_candidate_count; e++) {
    std::cout << static_cast<int>(enum_state[e]);
  }
  if (print_candidates) {
    std::cout << "\n      EC={";
    for (int e = 0; e < edge_candidate_count; e++) {
      if (e > 0) std::cout << ", ";
      std::cout << std::bitset<MAX_VERTICES>(edge_candidates[e]);
    }
    std::cout << "}";
  }
  std::cout << "]\n";
}