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

#include <any>
#include <cstring>
#include <iostream>

#include "grin/predefine.h"
#include "grin/test/config.h"

// GRIN headers
#include "common/error.h"
#include "property/property.h"
#include "property/propertylist.h"
#include "property/row.h"
#include "property/topology.h"
#include "property/type.h"
#include "topology/adjacentlist.h"
#include "topology/edgelist.h"
#include "topology/structure.h"
#include "topology/vertexlist.h"

void test_row_float_array(GRIN_GRAPH graph) {
  std::cout << "\n++++ test row: float array ++++" << std::endl;
  // create row
  auto row = grin_create_row(graph);

  // create float array
  float float_array[3] = {1.1, 2.2, 3.3};

  // insert float array: not support
  auto status = grin_insert_float_array_to_row(graph, row, float_array, 3);
  ASSERT(status == false);

  // destroy row
  grin_destroy_row(graph, row);

  std::cout << "---- test row: float array completed ----" << std::endl;
}

void test_vertex_property_float_array(GRIN_GRAPH graph) {
  std::cout << "\n++++ test vertex property: float array ++++" << std::endl;

  // get vertex type list
  auto vertex_type_list = grin_get_vertex_type_list(graph);
  size_t n = grin_get_vertex_type_list_size(graph, vertex_type_list);
  std::cout << "size of vertex type list = " << n << std::endl;

  // get vertex type from vertex type list
  auto vertex_type = grin_get_vertex_type_from_list(graph, vertex_type_list, 0);

  // select type for vertex list
  auto select_vertex_list = grin_get_vertex_list_by_type(graph, vertex_type);
  auto vertex_size = grin_get_vertex_list_size(graph, select_vertex_list);

  // get property list by vertex type
  auto property_list =
      grin_get_vertex_property_list_by_type(graph, vertex_type);
  auto property_size = grin_get_vertex_property_list_size(graph, property_list);
  // get property from property list
  auto property = grin_get_vertex_property_from_list(graph, property_list, 0);

  for (auto i = 0; i < vertex_size; i++) {
    // get vertex from vertex list
    auto vertex = grin_get_vertex_from_list(graph, select_vertex_list, i);
    // get value of float array
    auto value =
        grin_get_vertex_property_value_of_float_array(graph, vertex, property);
    ASSERT(value != NULL);
    for (auto j = 0; j < property_size; j++) {
      std::cout << "value[" << j << "] = " << value[j] << " ";
    }
    std::cout << std::endl;
    // destroy
    grin_destroy_float_array_value(graph, value);
    grin_destroy_vertex(graph, vertex);
  }

  // destroy
  grin_destroy_vertex_property(graph, property);
  grin_destroy_vertex_property_list(graph, property_list);
  grin_destroy_vertex_list(graph, select_vertex_list);
  grin_destroy_vertex_type(graph, vertex_type);
  grin_destroy_vertex_type_list(graph, vertex_type_list);

  std::cout << "---- test vertex property: float array completed ----"
            << std::endl;
}

void test_edge_property_float_array(GRIN_GRAPH graph) {
  std::cout << "\n++++ test edge property: float array ++++" << std::endl;

  // get edge type list
  auto edge_type_list = grin_get_edge_type_list(graph);
  size_t n = grin_get_edge_type_list_size(graph, edge_type_list);
  std::cout << "size of edge type list = " << n << std::endl;

  // get edge type from edge type list
  auto edge_type = grin_get_edge_type_from_list(graph, edge_type_list, 0);

  // select type for edge list
  auto select_edge_list = grin_get_edge_list_by_type(graph, edge_type);

  // get property list by edge type
  auto property_list = grin_get_edge_property_list_by_type(graph, edge_type);
  auto property_size = grin_get_edge_property_list_size(graph, property_list);
  // get property from property list
  auto property = grin_get_edge_property_from_list(graph, property_list, 0);

  auto it = grin_get_edge_list_begin(graph, select_edge_list);
  while (grin_is_edge_list_end(graph, it) == false) {
    // get edge from edge list iter
    auto edge = grin_get_edge_from_iter(graph, it);
    // get value of float array
    auto value =
        grin_get_edge_property_value_of_float_array(graph, edge, property);
    ASSERT(value != NULL);
    for (auto j = 0; j < property_size; j++) {
      std::cout << "value[" << j << "] = " << value[j] << " ";
    }
    std::cout << std::endl;
    grin_get_next_edge_list_iter(graph, it);
    // destroy
    grin_destroy_float_array_value(graph, value);
    grin_destroy_edge(graph, edge);
  }

  // destroy
  grin_destroy_edge_list_iter(graph, it);
  grin_destroy_edge_property(graph, property);
  grin_destroy_edge_property_list(graph, property_list);
  grin_destroy_edge_list(graph, select_edge_list);
  grin_destroy_edge_type(graph, edge_type);
  grin_destroy_edge_type_list(graph, edge_type_list);

  std::cout << "---- test edge property: float array completed ----"
            << std::endl;
}

int main(int argc, char* argv[]) {
  // get graph from graph info of GraphAr
  std::string path = "graphar://" + TEST_DATA_GLE_PATH;
  std::cout << "graph uri = " << path << std::endl;

  char* uri = new char[path.length() + 1];
  snprintf(uri, path.length() + 1, "%s", path.c_str());
  GRIN_GRAPH graph = grin_get_graph_from_storage(uri);
  ASSERT(graph != GRIN_NULL_GRAPH);
  delete[] uri;

  // test row: float array
  test_row_float_array(graph);

  // test vertex property: float array
  test_vertex_property_float_array(graph);

  // test edge property: float array
  test_edge_property_float_array(graph);

  // destroy graph
  grin_destroy_graph(graph);

  return 0;
}
