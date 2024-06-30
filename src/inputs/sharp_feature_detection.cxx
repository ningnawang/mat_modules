#include "sharp_feature_detection.h"

void load_local_from_mesh(const GEO::Mesh& mesh, std::vector<Vector3>& points,
                          std::vector<Vector3i>& faces) {
  points.clear();
  points.resize(mesh.vertices.nb());
  for (size_t i = 0; i < points.size(); i++) points[i] = mesh.vertices.point(i);

  faces.clear();
  faces.resize(mesh.facets.nb());
  for (size_t i = 0; i < faces.size(); i++) {
    faces[i][0] = mesh.facets.vertex(i, 0);
    faces[i][1] = mesh.facets.vertex(i, 1);
    faces[i][2] = mesh.facets.vertex(i, 2);
  }
}

// void store_feature_neighbors(const std::set<aint2>& s_edges,
//                              const std::set<aint2>& cc_edges,
//                              std::map<aint2, std::set<aint2>>& se_neighbors,
//                              std::map<aint2, std::set<aint2>>& ce_neighbors)
//                              {
//   auto load_fe_neighbors = [](const std::set<aint2>& f_edges,
//                               std::map<aint2, std::set<aint2>>& fe_neighbors)
//                               {
//     std::map<int, std::set<aint2>> fe_v2se;
//     for (const auto& fe : f_edges) {
//       fe_v2se[fe[0]].insert(fe);
//       fe_v2se[fe[1]].insert(fe);
//     }
//     assert(!fe_v2se.empty());
//     for (auto& pair : fe_v2se) {
//       auto e1 = pair.second.begin();
//       auto en = std::next(pair.second.begin(), 1);
//       for (; en != pair.second.end(); en++) {
//         fe_neighbors[*e1].insert(*en);
//       }
//     }
//   };

//   se_neighbors.clear();
//   ce_neighbors.clear();
//   if (!s_edges.empty()) {
//     load_fe_neighbors(s_edges, se_neighbors);
//   }
//   if (!cc_edges.empty()) {
//     load_fe_neighbors(cc_edges, ce_neighbors);
//   }
//   printf("Stored feature neighboring \n");
// }

// void store_feature_neighbors(const std::set<aint2>& f_edges,
//                              std::map<aint2, std::set<aint2>>& fe_neighbors)
//                              {
//   auto load_fe_neighbors = [](const std::set<aint2>& f_edges,
//                               std::map<aint2, std::set<aint2>>& fe_neighbors)
//                               {
//     std::map<int, std::set<aint2>> fe_v2se;
//     for (const auto& fe : f_edges) {
//       fe_v2se[fe[0]].insert(fe);
//       fe_v2se[fe[1]].insert(fe);
//     }
//     assert(!fe_v2se.empty());
//     for (auto& pair : fe_v2se) {
//       auto e1 = pair.second.begin();
//       auto en = std::next(pair.second.begin(), 1);
//       for (; en != pair.second.end(); en++) {
//         fe_neighbors[*e1].insert(*en);
//       }
//     }
//   };

//   fe_neighbors.clear();
//   if (!f_edges.empty()) {
//     load_fe_neighbors(f_edges, fe_neighbors);
//   }
// }

void mark_feature_attributes(const std::set<aint2>& s_edges,
                             const std::set<aint2>& cc_edges,
                             const std::set<int>& corners, GEO::Mesh& input) {
  // Setup corners
  GEO::Attribute<int> attr_corners(input.vertices.attributes(), "corner");
  for (int i = 0; i < input.vertices.nb(); i++) {
    if (corners.find(i) != corners.end())
      attr_corners[i] = 1;
    else
      attr_corners[i] = 0;
  }

  // Setup sharp edges and concave edges
  GEO::Attribute<int> attr_se(input.edges.attributes(), "se");
  GEO::Attribute<int> attr_cce(input.edges.attributes(), "cce");
  for (int e = 0; e < input.edges.nb(); e++) {
    aint2 edge = {
        {(int)input.edges.vertex(e, 0), (int)input.edges.vertex(e, 1)}};
    std::sort(edge.begin(), edge.end());
    if (s_edges.find(edge) != s_edges.end())
      attr_se[e] = 1;
    else if (cc_edges.find(edge) != cc_edges.end())
      attr_cce[e] = 1;
    else {
      attr_se[e] = 0;
      attr_cce[e] = 0;
    }
  }
  printf("Marked feature attributes in GEO::Mesh \n");
}

void find_feature_edges(const Parameter& args,
                        const std::vector<Vector3>& input_vertices,
                        const std::vector<Vector3i>& input_faces,
                        std::set<aint2>& s_edges, std::set<aint2>& cc_edges,
                        std::set<int>& corners_se, std::set<int>& corners_ce,
                        std::set<int>& corners_fake,
                        std::set<aint2>& fe_sf_fs_pairs,
                        std::set<aint2>& fe_sf_fs_pairs_se_only,
                        std::set<aint2>& fe_sf_fs_pairs_ce_only) {
  printf("Detecting sharp/concave edges and corners_se using threshold: %f \n",
         args.thres_concave);
  s_edges.clear();
  cc_edges.clear();
  corners_se.clear();
  corners_ce.clear();
  fe_sf_fs_pairs.clear();
  fe_sf_fs_pairs_se_only.clear();
  fe_sf_fs_pairs_ce_only.clear();

  std::vector<aint2> edges;
  std::map<int, std::unordered_set<int>> conn_tris;
  for (int i = 0; i < input_faces.size(); i++) {
    const auto& f = input_faces[i];
    for (int j = 0; j < 3; j++) {
      aint2 e = {{f[j], f[(j + 1) % 3]}};
      if (e[0] > e[1]) std::swap(e[0], e[1]);
      edges.push_back(e);
      conn_tris[input_faces[i][j]].insert(i);
    }
  }
  vector_unique(edges);

  // find sharp edges and concave edges
  for (const auto& e : edges) {
    std::vector<int> n12_f_ids;
    set_intersection(conn_tris[e[0]], conn_tris[e[1]], n12_f_ids);

    if (n12_f_ids.size() == 1) {  // open boundary
      printf("Detect open boundary!!! edge (%d,%d) has only 1 face: %ld\n",
             e[0], e[1], n12_f_ids.size());
      printf("ERROR: we don't know how to handle open boundary!!\n");
      assert(false);
    }
    int f_id = n12_f_ids[0];
    int j = 0;
    for (; j < 3; j++) {
      if ((input_faces[f_id][j] == e[0] &&
           input_faces[f_id][mod3(j + 1)] == e[1]) ||
          (input_faces[f_id][j] == e[1] &&
           input_faces[f_id][mod3(j + 1)] == e[0]))
        break;
    }
    Vector3 n = get_normal(input_vertices[input_faces[f_id][0]],
                           input_vertices[input_faces[f_id][1]],
                           input_vertices[input_faces[f_id][2]]);
    Vector3 c_n = get_triangle_centroid(input_vertices[input_faces[f_id][0]],
                                        input_vertices[input_faces[f_id][1]],
                                        input_vertices[input_faces[f_id][2]]);

    for (int k = 0; k < n12_f_ids.size(); k++) {
      if (n12_f_ids[k] == f_id) continue;
      Vector3 n1 = get_normal(input_vertices[input_faces[n12_f_ids[k]][0]],
                              input_vertices[input_faces[n12_f_ids[k]][1]],
                              input_vertices[input_faces[n12_f_ids[k]][2]]);
      Vector3 c_n1 =
          get_triangle_centroid(input_vertices[input_faces[n12_f_ids[k]][0]],
                                input_vertices[input_faces[n12_f_ids[k]][1]],
                                input_vertices[input_faces[n12_f_ids[k]][2]]);

      aint2 ref_fs_pair = {{f_id, n12_f_ids[k]}};
      std::sort(ref_fs_pair.begin(), ref_fs_pair.end());
      // std::array<Vector3, 2> ref_fs_normals = {{n, n1}};
      bool is_debug = false;

      //////////
      // Since cosine can only measure dihedral angle from (0, 180)
      // but concave has angle larger than 180
      // therefore we use different measurement for concave, and sharp edges
      //////////
      // Concave edges
      // c_n is a random vertex on plane A, c_n1 is a random vertex on plane B
      // n is normal of A
      //
      // 2021-09-04 ninwang:
      // If na and nb are the normals of the both adjacent faces,
      // and pa and pb vertices of the both faces that are not connected to
      // the edge, wherein na and pa belongs to the face A, and nb and pb to
      // the face B, then ( pb - pa ) . na > 0 => concave edge
      double tmp_concave = GEO::dot(GEO::normalize(c_n1 - c_n), n);     // A, B
      double tmp_concave_2 = GEO::dot(GEO::normalize(c_n - c_n1), n1);  // B, A
      if (tmp_concave > args.thres_concave ||
          tmp_concave_2 > args.thres_concave) {  // SCALAR_ZERO is too small
        // if (is_debug)
        //   printf("edge e (%d,%d) is a concave edge, tmp_concave: %f\n", e[0],
        //          e[1], tmp_concave);
        cc_edges.insert(e);  // once concave, never sharp
        fe_sf_fs_pairs.insert(ref_fs_pair);
        fe_sf_fs_pairs_ce_only.insert(ref_fs_pair);
      } else {
        // Sharp edges (when it's convex)
        // angle between two normals of convex faces: theta
        // => cos(theta) = n1.dot(n)
        // acosine() range in [0, pi]
        // sharp edges => theta in (angle_sharp, pi)
        // here angle_sharp = 30
        // Note that, using theta CANNOT differentiate concave or convex,
        // so the concave detection must run first
        double tmp_convex = std::acos(GEO::dot(n1, n));
        double angle_sharp = PI * (args.thres_convex / 180.);
        if (angle_sharp < tmp_convex && tmp_convex < PI) {
          // logger().debug("sharp edge: theta is {}", tmp_convex);
          s_edges.insert(e);
          fe_sf_fs_pairs.insert(ref_fs_pair);
          fe_sf_fs_pairs_se_only.insert(ref_fs_pair);
        }
      }
    }  // for n12_f_ids
  }  // for edges

  // vector_unique(s_edges);

  // for finding corners
  std::map<int, std::set<int>> neighbor_v_se, neighbor_v_ce;
  for (const auto& se : s_edges) {
    neighbor_v_se[se[0]].insert(se[1]);
    neighbor_v_se[se[1]].insert(se[0]);
  }
  for (const auto& ce : cc_edges) {
    neighbor_v_ce[ce[0]].insert(ce[1]);
    neighbor_v_ce[ce[1]].insert(ce[0]);
  }

  // find all fake corners
  // connect to >=3 sharp edges or concave edges
  // including corners_se
  for (const auto& pair : neighbor_v_se) {
    int vid = pair.first;
    // 1. regular corner (connect to > 2 sharp edges)
    if (pair.second.size() > 2) {
      corners_se.insert(pair.first);
    }

    // to avoid segfault
    if (neighbor_v_ce.find(vid) == neighbor_v_ce.end()) continue;
    // 2. non-regular corner (> 0 sharp edges and > 0 concave edges)
    // just to make sure we will add a zero-radius medial sphere
    if (pair.second.size() > 0 && neighbor_v_ce.at(vid).size() > 0) {
      corners_se.insert(pair.first);
    }

    // 3. other fake corners_se
    // only used for splitting sharp edges into different connected groups
    // to avoid curved sharp edges
    // (including corners_se)
    int adj_size = pair.second.size() + neighbor_v_ce.at(vid).size();
    if (adj_size > 2) corners_fake.insert(pair.first);
  }

  for (const auto& pair : neighbor_v_ce) {
    int vid = pair.first;
    if (pair.second.size() <= 2) continue;
    if (neighbor_v_se.find(vid) != neighbor_v_se.end()) continue;
    // adjacent to > 2 concave edges only
    corners_ce.insert(vid);
  }
  // corners_fake includes corners_se and corners_ce
  corners_fake.insert(corners_se.begin(), corners_se.end());
  corners_fake.insert(corners_ce.begin(), corners_ce.end());

  printf("#concave_edges = %ld \n", cc_edges.size());
  printf("#sharp_edges = %ld \n", s_edges.size());
  printf("#corners_se  = %ld \n", corners_se.size());
  printf("#corners_ce  = %ld \n", corners_ce.size());
  printf("#corners_fake  = %ld \n", corners_fake.size());
}

// mapping feature edges from surface vertex ids to tet mesh ids
void map_feature_edge_sf2tet(const std::vector<int> sf2tet_vs_mapping,
                             const std::set<aint2>& s_edges_sf,
                             std::set<aint2>& s_edges_tet_tmp) {
  s_edges_tet_tmp.clear();
  for (const auto& se : s_edges_sf) {
    aint2 se_tvs = {{
        sf2tet_vs_mapping[se[0]],
        sf2tet_vs_mapping[se[1]],
    }};
    se_tvs = get_sorted(se_tvs);
    s_edges_tet_tmp.insert(se_tvs);
    // if (se_tvs[0] == 64 && se_tvs[1] == 152) {
    //   printf(
    //       "!!!!! ERROR: sharp edge on surface (%d,%d) detect on tet mesh "
    //       "(%d,%d) \n",
    //       se[0], se[1], se_tvs[0], se_tvs[1]);
    // }
  }
}

void map_corners_sf2tet(const std::vector<int> sf2tet_vs_mapping,
                        const std::set<int>& corners_sf,
                        std::set<int>& corners_tet) {
  corners_tet.clear();
  for (const auto& c : corners_sf) {
    corners_tet.insert(sf2tet_vs_mapping.at(c));
  }
}

/**
 * @brief For each tet, convert sharp edge representation (if exists)
 *
 * @param tet_indices
 * @param fe_tet
 * @param tet_es2fe_map mapping sharp edge from <tid, lfid_min, lfid_max> ->
 * <fe_type, fe_id, fe_line_id>, detail see TetMesh::tet_es2fe_map
 */
void convert_fe_tfs2vs(const std::vector<int> tet_indices,
                       const EdgeType& fe_type, const std::set<aint4>& fe_tet,
                       std::map<aint3, aint3>& tet_es2fe_map) {
  std::map<aint2, aint3> fe_tet_map;
  for (const auto& se : fe_tet) {
    aint2 key = {{se[0], se[1]}};
    fe_tet_map[key] = {{fe_type, se[2], se[3]}};
  }

  // ninwang: do not clear here, we want to accumulate
  //          please clear before calling
  // tet_es2fe_map.clear();
  assert(tet_indices.size() % 4 == 0);
  for (uint tid = 0; tid < tet_indices.size() / 4; tid++) {
    // check if edge is on sharp edge
    for (uint le = 0; le < 6; le++) {
      // edge in 2 tet vids
      int tvid1 = tet_indices[tid * 4 + tet_edges_lvid_host[le][0]];
      int tvid2 = tet_indices[tid * 4 + tet_edges_lvid_host[le][1]];
      aint2 edge_tvids = {{tvid1, tvid2}};
      std::sort(edge_tvids.begin(), edge_tvids.end());
      // if (tid == 75) {
      //   printf("tid %d has edge_tvids: (%d,%d)\n", tid, edge_tvids[0],
      //          edge_tvids[1]);
      // }

      if (fe_tet_map.find(edge_tvids) == fe_tet_map.end()) continue;
      // found sharp edge
      // edge in 2 local fids
      int lfid1 = tet_edges_lfid_host[le][0];
      int lfid2 = tet_edges_lfid_host[le][1];
      aint3 tlfs = {{UNK_INT, UNK_INT, UNK_INT}};
      if (lfid1 < lfid2)
        tlfs = {{(int)tid, lfid1, lfid2}};
      else
        tlfs = {{(int)tid, lfid2, lfid1}};
      tet_es2fe_map[tlfs] = fe_tet_map.at(edge_tvids);
      // printf("found sharp edge: tlfs (%d,%d,%d), edge_tvids: (%d,%d) \n",
      //        tlfs[0], tlfs[1], tlfs[2], edge_tvids[0], edge_tvids[1]);
    }
  }
  // std::cout << "Found #tet on sharp edges: " << tet_es2fe_map.size()
  //           << std::endl;
}

/**
 * @brief For each tet, convert vertex representation
 *
 * @param tet_indices
 * @param tet_vs_lfs2tvs_map mapping vertex from <tid, lfid1, lfid2, lfid3> ->
 * tvid
 */
void convert_vs_tlfs2lvs(const std::vector<int> tet_indices,
                         std::map<aint4, int>& tet_vs_lfs2tvs_map) {
  tet_vs_lfs2tvs_map.clear();
  assert(tet_indices.size() % 4 == 0);
  for (uint tid = 0; tid < tet_indices.size() / 4; tid++) {
    for (uint lv = 0; lv < 4; lv++) {  // local vertices
      aint3 f = {{tet_faces_lvid_host[lv][0], tet_faces_lvid_host[lv][1],
                  tet_faces_lvid_host[lv][2]}};
      std::sort(f.begin(), f.end());
      aint4 v_key = {{tid, f[0], f[1], f[2]}};
      tet_vs_lfs2tvs_map[v_key] = tet_indices[tid * 4 + lv];
    }
  }
  // std::cout << "converted #tet_vs: " << tet_vs_lfs2tvs_map.size() <<
  // std::endl;
}

void update_allow_to_merge_ce_line_ids(
    const std::vector<ConcaveCorner>& cc_corners,
    std::map<int, std::set<int>>& allow_to_merge_ce_line_ids) {
  // loop cc_corners
  for (const auto& cc : cc_corners) {
    // cc.print_info();
    for (const auto& fl_id : cc.adj_fl_ids) {
      allow_to_merge_ce_line_ids[fl_id].insert(cc.adj_fl_ids.begin(),
                                               cc.adj_fl_ids.end());
    }

  }  // for a concave  corner
}

void convert_fe_groups_to_set(std::vector<std::vector<aint3>>& fe_groups,
                              std::set<aint3>& fe_set) {
  fe_set.clear();
  for (const auto& one_group : fe_groups) {
    fe_set.insert(one_group.begin(), one_group.end());
  }
}

/**
 * @brief Get the grouped feature edges, grouped in geometric sorted order
 *
 * @param fe_to_visit
 * @param corners
 * @param fe_groups return, format <vs_min, vs_max>
 * @return int
 */
int get_grouped_feature_edges(const std::set<aint2>& fe_to_visit,
                              const std::set<int>& corners,
                              std::vector<std::vector<aint2>>& fe_groups) {
  std::set<aint2> fe_unvisited = fe_to_visit;  // copy
  std::set<int> corners_unvisited = corners;
  fe_groups.clear();

  std::map<int, std::set<aint2>> vs_fe_neighbors;
  for (const auto& fe : fe_to_visit) {
    vs_fe_neighbors[fe[0]].insert(fe);
    vs_fe_neighbors[fe[1]].insert(fe);
  }
  std::set<aint2> corners_fe_unvisited;
  for (const auto& cid : corners) {
    // assert(vs_fe_neighbors.find(cid) != vs_fe_neighbors.end());
    // concave line may not pass any corner
    if (vs_fe_neighbors.find(cid) == vs_fe_neighbors.end()) continue;
    corners_fe_unvisited.insert(vs_fe_neighbors[cid].begin(),
                                vs_fe_neighbors[cid].end());
  }

  // printf("fe_unvisited: %ld, vs_fe_neighbors size: %ld \n",
  //        fe_unvisited.size(), vs_fe_neighbors.size());
  // int fe_line_id = 0;
  std::set<aint2> fe_visited;
  std::vector<aint2> one_fe_group;
  // calculate the number of CCs
  std::queue<aint2> queue;
  while (!fe_unvisited.empty()) {
    // start from a corner
    if (!corners_fe_unvisited.empty())
      queue.push(*corners_fe_unvisited.begin());
    else {  // or start from one end
      for (const auto& npair : vs_fe_neighbors) {
        if (npair.second.size() == 1) {
          aint2 se_tmp = *npair.second.begin();
          if (fe_visited.find(se_tmp) != fe_visited.end()) continue;
          queue.push(se_tmp);
          break;
        }
      }
    }
    if (queue.empty()) queue.push(*fe_unvisited.begin());

    // start queueing
    while (!queue.empty()) {
      aint2 one_fe = queue.front();
      queue.pop();

      if (fe_visited.find(one_fe) != fe_visited.end()) continue;
      fe_visited.insert(one_fe);   // all visited cells
      fe_unvisited.erase(one_fe);  // all unvisited cells
      if (corners_fe_unvisited.find(one_fe) != corners_fe_unvisited.end())
        corners_fe_unvisited.erase(one_fe);
      // visited cells in one CC
      // one_fe_group.push_back({{one_fe[0], one_fe[1], fe_line_id}});
      one_fe_group.push_back({{one_fe[0], one_fe[1]}});
      // push only one neighbors if not corner
      // depth-first search, not BFS
      for (int one_vs : one_fe) {
        bool is_pushed = false;
        // if corner then do not add its neighbors
        if (corners.find(one_vs) != corners.end()) continue;
        auto& neighbors = vs_fe_neighbors.at(one_vs);
        for (auto& neigh_se : neighbors) {
          if (fe_visited.find(neigh_se) != fe_visited.end()) continue;
          // only push one neighbor edge if not visited
          // for loop edge without corners
          queue.push(neigh_se);
          is_pushed = true;
          break;
        }
        if (is_pushed) break;
      }  // for one_fe
    }  // for while
    // fe_line_id++;
    fe_groups.push_back(one_fe_group);  // store cell ids in one CC
    one_fe_group.clear();
  }

  return fe_groups.size();
}

/**
 * @brief [No use] Get the grouped feature edges in set, no sorted order
 *
 * @param fe_to_visit
 * @param corners
 * @param fe_groups return, in set format of <vs_min, vs_max>
 * @return int
 */
int get_grouped_feature_edges_no_order(const std::set<aint2>& fe_to_visit,
                                       const std::set<int>& corners,
                                       std::set<aint2>& fe_groups) {
  std::set<aint2> fe_unvisited = fe_to_visit;  // copy
  std::set<int> corners_unvisited = corners;
  fe_groups.clear();

  std::map<int, std::set<aint2>> vs_fe_neighbors;
  for (const auto& fe : fe_to_visit) {
    vs_fe_neighbors[fe[0]].insert(fe);
    vs_fe_neighbors[fe[1]].insert(fe);
  }

  std::set<aint2> fe_visited;
  // calculate the number of CCs
  std::queue<aint2> queue;
  while (!fe_unvisited.empty()) {
    queue.push(*fe_unvisited.begin());

    // start queueing
    while (!queue.empty()) {
      aint2 one_fe = queue.front();
      queue.pop();

      if (fe_visited.find(one_fe) != fe_visited.end()) continue;
      fe_visited.insert(one_fe);   // all visited cells
      fe_unvisited.erase(one_fe);  // all unvisited cells
      // visited cells in one CC
      fe_groups.insert({{one_fe[0], one_fe[1]}});
      // push only one neighbors if not corner
      // depth-first search, not BFS
      for (int one_vs : one_fe) {
        bool is_pushed = false;
        // if corner then do not add its neighbors
        if (corners.find(one_vs) != corners.end()) continue;
        auto& neighbors = vs_fe_neighbors.at(one_vs);
        for (auto& neigh_se : neighbors) {
          if (fe_visited.find(neigh_se) != fe_visited.end()) continue;
          // only push one neighbor edge if not visited
          // for loop edge without corners
          queue.push(neigh_se);
          is_pushed = true;
          break;
        }
        if (is_pushed) break;
      }  // for one_fe
    }
  }

  return fe_groups.size();
}

void print_corners(const std::set<int>& corners) {
  printf("corner: [");
  for (const auto& c : corners) {
    printf("%d, ", c);
  }
  printf("]\n");
}

void print_corner2fl(const std::map<int, std::set<int>>& corner2fl) {
  printf("corner2fl:\n");
  for (const auto& pair : corner2fl) {
    printf("%d: [", pair.first);
    for (const auto& fl_id : pair.second) printf("%d, ", fl_id);
    printf("]\n");
  }
}

void print_corner2fe(std::map<int, std::set<int>>& corner2fe) {
  printf("corner2fe:\n");
  for (const auto& pair : corner2fe) {
    printf("%d: [", pair.first);
    for (const auto& fl_id : pair.second) printf("%d, ", fl_id);
    printf("]\n");
  }
}

void print_fl2corner(std::map<int, std::set<int>>& fl2corner) {
  printf("fl2corner:\n");
  for (const auto& pair : fl2corner) {
    printf("%d: [", pair.first);
    for (const auto& fl_id : pair.second) printf("%d, ", fl_id);
    printf("]\n");
  }
}

void store_concave_corners(const std::set<int>& corners_ce_tet,
                           const std::vector<FeatureEdge>& feature_edges,
                           std::vector<ConcaveCorner>& cc_corners,
                           bool is_debug) {
  if (corners_ce_tet.empty()) return;
  cc_corners.clear();

  // get corners -> adj_fes
  std::map<int, std::vector<int>> corner_adj_fes;
  for (const auto& one_fe : feature_edges) {
    // only cares about concave edges
    if (one_fe.type == EdgeType::SE) continue;
    for (int i = 0; i < 2; i++) {
      // only fetch the first vs_pair
      const int end_tvs_id = one_fe.t2vs_group[i];
      // for concave corners
      if (corners_ce_tet.find(end_tvs_id) != corners_ce_tet.end())
        corner_adj_fes[end_tvs_id].push_back(one_fe.id);
    }
  }
  assert(!corner_adj_fes.empty());

  // create new concave corners
  for (const auto& pair : corner_adj_fes) {
    int corner_tvid = pair.first;
    // create new ConcaveCorner
    ConcaveCorner cc_corner(corner_tvid);
    const std::vector<int>& adj_fes = pair.second;
    if (is_debug)
      printf("[ConcaveCorner] corner %d has adj_fes size: %ld \n", corner_tvid,
             adj_fes.size());

    for (const auto& one_fe_id : adj_fes) {
      cc_corner.adj_fe_ids.push_back(one_fe_id);
      cc_corner.adj_fl_ids.push_back(feature_edges.at(one_fe_id).get_fl_id());
      const auto& one_fe = feature_edges.at(one_fe_id);
      Vector3 pert_dir = one_fe.t2vs_group[0] == corner_tvid
                             ? get_direction(one_fe.t2vs_pos[0],
                                             one_fe.t2vs_pos[1])  // from 0 to 1
                             : get_direction(one_fe.t2vs_pos[1],
                                             one_fe.t2vs_pos[0]);  // for 1 to 0
      cc_corner.adj_fe_dir.push_back(pert_dir);

      double one_fe_len = GEO::distance(one_fe.t2vs_pos[0], one_fe.t2vs_pos[1]);
      cc_corner.adj_fe_len.push_back(one_fe_len);
    }
    cc_corners.push_back(cc_corner);

    if (is_debug) cc_corner.print_info();
  }

  if (is_debug)
    printf("[ConcaveCorner] stored %ld concave corners \n", cc_corners.size());
}

//--------------------------------------------------------------------------
//--------------------------- Main Function --------------------------------
//--------------------------------------------------------------------------
// mark feature attributes of GEO::Mesh
// NOTE: here we store edges as 2 neighboring tet vertices instead of surface
void detect_mark_sharp_features(const Parameter& args, SurfaceMesh& sf_mesh,
                                TetMesh& tet_mesh) {
  const auto& sf2tet_vs_mapping = sf_mesh.sf2tet_vs_mapping;
  assert(!sf2tet_vs_mapping.empty());
  std::vector<Vector3> points;
  std::vector<Vector3i> faces;
  load_local_from_mesh(sf_mesh, points, faces);

  // find feature edges and corners on GEO::Mesh
  std::set<aint2> s_edges_sf, cc_edges_sf;
  std::set<int> corners_se_sf, corners_ce_sf, corners_fake_sf;
  find_feature_edges(args, points, faces, s_edges_sf, cc_edges_sf,
                     corners_se_sf, corners_ce_sf, corners_fake_sf,
                     sf_mesh.fe_sf_fs_pairs, sf_mesh.fe_sf_fs_pairs_se_only,
                     sf_mesh.fe_sf_fs_pairs_ce_only);
  mark_feature_attributes(s_edges_sf, cc_edges_sf, corners_se_sf, sf_mesh);

  // map back to tet mesh
  std::set<aint2> s_edges_tet_tmp, cc_edges_tet_tmp;
  map_feature_edge_sf2tet(sf2tet_vs_mapping, s_edges_sf, s_edges_tet_tmp);
  map_feature_edge_sf2tet(sf2tet_vs_mapping, cc_edges_sf, cc_edges_tet_tmp);
  map_corners_sf2tet(sf2tet_vs_mapping, corners_se_sf, tet_mesh.corners_se_tet);
  map_corners_sf2tet(sf2tet_vs_mapping, corners_ce_sf, tet_mesh.corners_ce_tet);
  map_corners_sf2tet(sf2tet_vs_mapping, corners_fake_sf,
                     tet_mesh.corner_fake_tet);
  print_corners(tet_mesh.corner_fake_tet);

  // here we use TetMesh::corner_fake_tet
  // to split SE and CE into different groups
  //
  // 1. vector index matching FeatureEdge::t2vs_group[2]/FeatureLine::id
  // in either TetMesh::se_lines or TetMesh::ce_lines
  // 2. aint2 format <tet_vs_min, tet_vs_max>
  std::vector<std::vector<aint2>> se_tet_groups, ce_tet_groups;
  get_grouped_feature_edges(s_edges_tet_tmp, tet_mesh.corner_fake_tet,
                            se_tet_groups);
  get_grouped_feature_edges(cc_edges_tet_tmp, tet_mesh.corner_fake_tet,
                            ce_tet_groups);

  // update TetMesh::feature_edges and TetMesh::se_lines/TetMesh::ce_lines
  tet_mesh.feature_edges.clear();
  // format <tet_vs_min, tet_vs_max, fe_id, fe_line_id>
  std::set<aint4> se_tet_info, ce_tet_info;
  store_feature_line(tet_mesh, sf_mesh, EdgeType::SE, se_tet_groups,
                     tet_mesh.feature_edges, tet_mesh.se_lines, se_tet_info,
                     tet_mesh.corners_se_tet, tet_mesh.corner2fl,
                     tet_mesh.corner2fe, false /*is_debug*/);
  printf("[Feature] stored SphereType::SE line size: %ld \n",
         tet_mesh.se_lines.size());
  store_feature_line(tet_mesh, sf_mesh, EdgeType::CE, ce_tet_groups,
                     tet_mesh.feature_edges, tet_mesh.ce_lines, ce_tet_info,
                     tet_mesh.corners_se_tet, tet_mesh.corner2fl,
                     tet_mesh.corner2fe, false /*is_debug*/);
  printf("[Feature] stored SphereType::CE line size: %ld \n",
         tet_mesh.ce_lines.size());
  // print_corner2fl(tet_mesh.corner2fl);
  // print_corner2fe(tet_mesh.corner2fe);

  // convert feature edges and vertices to another representation
  // for sharp lines (TetMesh::se_lines)
  tet_mesh.tet_es2fe_map.clear();
  convert_fe_tfs2vs(tet_mesh.tet_indices, EdgeType::SE, se_tet_info,
                    tet_mesh.tet_es2fe_map);
  // for concave lines (TetMesh::ce_lines)
  convert_fe_tfs2vs(tet_mesh.tet_indices, EdgeType::CE, ce_tet_info,
                    tet_mesh.tet_es2fe_map);
  convert_vs_tlfs2lvs(tet_mesh.tet_indices, tet_mesh.tet_vs_lfs2tvs_map);

  // for concave corners
  store_concave_corners(tet_mesh.corners_ce_tet, tet_mesh.feature_edges,
                        tet_mesh.cc_corners, false);
  // update FeatureLine::allow_to_merge_fl_ids
  update_allow_to_merge_ce_line_ids(tet_mesh.cc_corners,
                                    tet_mesh.allow_to_merge_ce_line_ids);
}