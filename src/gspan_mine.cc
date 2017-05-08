#include <gspan.h>
#include <graph.h>
#include <common.h>
#include <sstream>

namespace gspan {

void GSpan::find_frequent_nodes(const vector<Graph> &graphs) {
  unordered_map<size_t, size_t> vertex_labels;
  unordered_map<size_t, size_t> edge_labels;

  for (size_t i = 0; i < graphs.size(); ++i) {
    unordered_set<size_t> vertex_set;
    unordered_set<size_t> edge_set;
    for (size_t j = 0; j < graphs[i].size(); ++j) {
      const struct vertex_t *vertex = graphs[i].get_p_vertex(j);
      vertex_set.insert(vertex->label);
      for (size_t k = 0; k < (vertex->edges).size(); ++k) {
        edge_set.insert(vertex->edges[k].label);
      }
    }
    for (unordered_set<size_t>::iterator it = vertex_set.begin(); it != vertex_set.end(); ++it) {
      ++vertex_labels[*it];
    }
    for (unordered_set<size_t>::iterator it = edge_set.begin(); it != edge_set.end(); ++it) {
      ++edge_labels[*it];
    }
  }
  for (unordered_map<size_t, size_t>::iterator it = vertex_labels.begin();
    it != vertex_labels.end(); ++it) {
    if (it->second >= nsupport_) {
      frequent_vertex_labels_.insert(std::make_pair(it->first, it->second));
    }
  }
  for (unordered_map<size_t, size_t>::iterator it = edge_labels.begin();
    it != edge_labels.end(); ++it) {
    if (it->second >= 2 * nsupport_) {  // undirected edges
      frequent_edge_labels_.insert(std::make_pair(it->first, it->second));
    }
  }
}

void GSpan::report(const Projection &projection, int prev_id, size_t nsupport) {
  std::stringstream ss;
  Graph graph;
  build_graph(graph);

  for (size_t i = 0; i < graph.size(); ++i) {
    const struct vertex_t *vertex = graph.get_p_vertex(i);
    ss << "v " << vertex->id << " " << vertex->label << std::endl;
  }
  for (size_t i = 0; i < dfs_codes_.size(); ++i) {
    ss << "e " << dfs_codes_[i].from << " " << dfs_codes_[i].to
      << " " << dfs_codes_[i].edge_label << std::endl;
  }
  ss << "x: ";
  size_t prev = 0;
  for (size_t i = 0; i < projection.size(); ++i) {
    if (i == 0 || projection[i].id != prev) {
      prev = projection[i].id;
      ss << prev << " ";
    }
  }
  output_.push_back(ss.str(), nsupport, output_.size(), prev_id);
}

void GSpan::save() {
  output_.save();
}

void GSpan::mine_subgraph(const vector<Graph> &graphs, int prev_id, Projection &projection) {
  size_t nsupport = count_support(projection);
  if (nsupport < nsupport_ || !is_min()) {
    return;
  }
  report(projection, prev_id, nsupport);
  prev_id = output_.size() - 1;

  // Find right most path
  vector<size_t> right_most_path;
  build_right_most_path(dfs_codes_, right_most_path);
  size_t min_label = dfs_codes_[0].from_label;

  // Enumerate backward paths and forward paths by different rules
  ProjectionMapBackward projection_map_backward;
  ProjectionMapForward projection_map_forward;
  enumerate(graphs, projection, right_most_path, min_label,
    projection_map_backward, projection_map_forward);
  // Recursive mining: first backward, last backward, and then last forward to the first forward
  for (ProjectionMapBackward::iterator it = projection_map_backward.begin();
    it != projection_map_backward.end(); ++it) {
    dfs_codes_.push_back(dfs_code_t((it->first).from, (it->first).to,
      (it->first).from_label, (it->first).edge_label, (it->first).to_label));
    mine_subgraph(graphs, prev_id, it->second);
    dfs_codes_.pop_back();
  }
  for (ProjectionMapForward::reverse_iterator it = projection_map_forward.rbegin();
    it != projection_map_forward.rend(); ++it) {
    dfs_codes_.push_back(dfs_code_t((it->first).from, (it->first).to,
      (it->first).from_label, (it->first).edge_label, (it->first).to_label));
    mine_subgraph(graphs, prev_id, it->second);
    dfs_codes_.pop_back();
  }
}

}  // namespace gspan