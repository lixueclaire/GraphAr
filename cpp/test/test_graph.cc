/** Copyright 2022 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <iostream>

#include "./util.h"
#include "gar/graph.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE("test_vertices_collection") {
  std::string root;
  REQUIRE(GetTestResourceRoot(&root).ok());

  // read file and construct graph info
  std::string path = root + "/ldbc_sample/parquet/ldbc_sample.graph.yml";
  auto maybe_graph_info = GAR_NAMESPACE::GraphInfo::Load(path);
  REQUIRE(maybe_graph_info.status().ok());
  auto graph_info = maybe_graph_info.value();

  // construct vertices collection
  std::string label = "person", property = "firstName";
  auto maybe_vertices_collection =
      GAR_NAMESPACE::ConstructVerticesCollection(graph_info, label);
  REQUIRE(!maybe_vertices_collection.has_error());
  auto& vertices = maybe_vertices_collection.value();
  auto count = 0;
  for (auto it = vertices.begin(); it != vertices.end(); ++it) {
    // access data through iterator directly
    std::cout << it.id() << ", id=" << it.property<int64_t>("id").value()
              << ", firstName=" << it.property<std::string>("firstName").value()
              << std::endl;
    // access data through vertex
    auto vertex = *it;
    std::cout << vertex.id()
              << ", id=" << vertex.property<int64_t>("id").value()
              << ", firstName="
              << vertex.property<std::string>("firstName").value() << std::endl;
    count++;
  }
  auto it_last = vertices.begin() + (count - 1);
  std::cout << it_last.id()
            << ", id=" << it_last.property<int64_t>("id").value()
            << ", firstName="
            << it_last.property<std::string>("firstName").value() << std::endl;
}

TEST_CASE("test_edges_collection", "[Slow]") {
  std::string root;
  REQUIRE(GetTestResourceRoot(&root).ok());

  std::string path = root + "/ldbc_sample/parquet/ldbc_sample.graph.yml";
  std::string src_label = "person", edge_label = "knows", dst_label = "person";
  auto graph_info = GAR_NAMESPACE::GraphInfo::Load(path).value();

  // iterate edges of vertex chunk 0
  auto expect = GAR_NAMESPACE::ConstructEdgesCollection(
      graph_info, src_label, edge_label, dst_label,
      GAR_NAMESPACE::AdjListType::ordered_by_source, 0, 1);
  REQUIRE(!expect.has_error());
  auto& edges = std::get<GAR_NAMESPACE::EdgesCollection<
      GAR_NAMESPACE::AdjListType::ordered_by_source>>(expect.value());
  auto end = edges.end();
  size_t count = 0;
  for (auto it = edges.begin(); it != end; ++it) {
    // access data through iterator directly
    std::cout << "src=" << it.source() << ", dst=" << it.destination()
              << std::endl;
    auto edge = *it;
    std::cout << "src=" << edge.source() << ", dst=" << edge.destination()
              << std::endl;
    count++;
  }
  std::cout << "edge_count=" << count << std::endl;
  REQUIRE(edges.size() == count);

  // iterate edges of vertex chunk [2, 4)
  auto expect1 = GAR_NAMESPACE::ConstructEdgesCollection(
      graph_info, src_label, edge_label, dst_label,
      GAR_NAMESPACE::AdjListType::ordered_by_dest, 2, 4);
  REQUIRE(!expect1.has_error());
  auto& edges1 = std::get<GAR_NAMESPACE::EdgesCollection<
      GAR_NAMESPACE::AdjListType::ordered_by_dest>>(expect1.value());
  auto end1 = edges1.end();
  size_t count1 = 0;
  for (auto it = edges1.begin(); it != end1; ++it) {
    count1++;
  }
  std::cout << "edge_count=" << count1 << std::endl;
  REQUIRE(edges1.size() == count1);

  // iterate all edges
  auto expect2 = GAR_NAMESPACE::ConstructEdgesCollection(
      graph_info, src_label, edge_label, dst_label,
      GAR_NAMESPACE::AdjListType::ordered_by_source);
  REQUIRE(!expect2.has_error());
  auto& edges2 = std::get<GAR_NAMESPACE::EdgesCollection<
      GAR_NAMESPACE::AdjListType::ordered_by_source>>(expect2.value());
  auto end2 = edges2.end();
  size_t count2 = 0;
  for (auto it = edges2.begin(); it != end2; ++it) {
    auto edge = *it;
    std::cout << "src=" << edge.source() << ", dst=" << edge.destination()
              << std::endl;
    count2++;
  }
  std::cout << "edge_count=" << count2 << std::endl;
  REQUIRE(edges2.size() == count2);

  // empty collection
  auto expect3 = GAR_NAMESPACE::ConstructEdgesCollection(
      graph_info, src_label, edge_label, dst_label,
      GAR_NAMESPACE::AdjListType::unordered_by_source, 5, 5);
  REQUIRE(!expect2.has_error());
  auto& edges3 = std::get<GAR_NAMESPACE::EdgesCollection<
      GAR_NAMESPACE::AdjListType::unordered_by_source>>(expect3.value());
  auto end3 = edges3.end();
  size_t count3 = 0;
  for (auto it = edges3.begin(); it != end3; ++it) {
    count3++;
  }
  std::cout << "edge_count=" << count3 << std::endl;
  REQUIRE(count3 == 0);
  REQUIRE(edges3.size() == 0);

  // invalid adjlist type
  auto expect4 = GAR_NAMESPACE::ConstructEdgesCollection(
      graph_info, src_label, edge_label, dst_label,
      GAR_NAMESPACE::AdjListType::unordered_by_dest);
  REQUIRE(expect4.status().IsInvalid());
}