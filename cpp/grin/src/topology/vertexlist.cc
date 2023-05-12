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

extern "C" {
#include "grin/include/topology/vertexlist.h"
}
#include "grin/src/predefine.h"

#ifdef GRIN_ENABLE_VERTEX_LIST
GRIN_VERTEX_LIST grin_get_vertex_list(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto vl = new GRIN_VERTEX_LIST_T(0, _g->vertex_type_num);
  return vl;
}

void grin_destroy_vertex_list(GRIN_GRAPH g, GRIN_VERTEX_LIST vl) {
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  delete _vl;
}
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST_ARRAY
size_t grin_get_vertex_list_size(GRIN_GRAPH g, GRIN_VERTEX_LIST vl) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);

  if (_vl->partition_type == ALL_PARTITION) {  // all partition
    return _g->vertex_offsets[_vl->type_end] -
           _g->vertex_offsets[_vl->type_begin];
  }

  if (_vl->partition_type == ONE_PARTITION) {  // one partition
    auto partition_id = _vl->partition_id;
    auto tot_size = 0;
    for (auto i = _vl->type_begin; i < _vl->type_end; i++) {
      auto partitioned_vertex_num = __grin_get_paritioned_vertex_num(
          _g, i, partition_id, _g->partition_strategy);
      tot_size += partitioned_vertex_num;
    }
    return tot_size;
  }

  if (_vl->partition_type == ALL_BUT_ONE_PARTITION) {  // all but one
    auto partition_id = _vl->partition_id;
    auto tot_size = 0;
    for (auto i = _vl->type_begin; i < _vl->type_end; i++) {
      auto vertex_num = _g->vertex_offsets[i + 1] - _g->vertex_offsets[i];
      auto partitioned_vertex_num = __grin_get_paritioned_vertex_num(
          _g, i, partition_id, _g->partition_strategy);
      tot_size += vertex_num - partitioned_vertex_num;
    }
    return tot_size;
  }

  return 0;  // undefined
}

GRIN_VERTEX grin_get_vertex_from_list(GRIN_GRAPH g, GRIN_VERTEX_LIST vl,
                                      size_t idx) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  if (_vl->partition_type == ALL_PARTITION) {  // all parititon
    for (auto i = _vl->type_begin; i < _vl->type_end; i++) {
      if (idx <
          _g->vertex_offsets[i + 1] - _g->vertex_offsets[_vl->type_begin]) {
        auto _idx =
            idx + _g->vertex_offsets[_vl->type_begin] - _g->vertex_offsets[i];
        auto v = new GRIN_VERTEX_T(_idx, i);
        return v;
      }
    }
  }

  if (_vl->partition_type == ONE_PARTITION) {  // one partition
    auto partition_id = _vl->partition_id;
    auto cur = 0;
    for (auto i = _vl->type_begin; i < _vl->type_end; i++) {
      auto partitioned_vertex_num = __grin_get_paritioned_vertex_num(
          _g, i, partition_id, _g->partition_strategy);
      if (idx < cur + partitioned_vertex_num) {
        auto _idx = __grin_get_vertex_id_from_partitioned_vertex_id(
            _g, i, partition_id, _g->partition_strategy, idx - cur);
        auto v = new GRIN_VERTEX_T(_idx, i);
        return v;
      }
      cur += partitioned_vertex_num;
    }
    return GRIN_NULL_VERTEX;
  }

  if (_vl->partition_type == ALL_BUT_ONE_PARTITION) {  // all but one
    auto partition_id = _vl->partition_id;
    auto cur = 0;
    for (auto i = _vl->type_begin; i < _vl->type_end; i++) {
      auto vertex_num = _g->vertex_offsets[i + 1] - _g->vertex_offsets[i];
      auto partitioned_vertex_num = __grin_get_paritioned_vertex_num(
          _g, i, partition_id, _g->partition_strategy);
      if (idx < cur + vertex_num - partitioned_vertex_num) {
        auto tmp = 0;
        for (auto j = 0; j < _g->partition_num; j++) {
          if (j == partition_id)
            continue;
          auto j_vertex_num = __grin_get_paritioned_vertex_num(
              _g, i, j, _g->partition_strategy);
          if (idx < cur + tmp + j_vertex_num) {
            auto _idx = __grin_get_vertex_id_from_partitioned_vertex_id(
                _g, i, j, _g->partition_strategy, idx - cur - tmp);
            auto v = new GRIN_VERTEX_T(_idx, i);
            return v;
          }
          tmp += j_vertex_num;
        }
      }
      cur += vertex_num - partitioned_vertex_num;
    }
    return GRIN_NULL_VERTEX;
  }

  return GRIN_NULL_VERTEX;  // undefined
}
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST_ITERATOR
GRIN_VERTEX_LIST_ITERATOR grin_get_vertex_list_begin(GRIN_GRAPH g,
                                                     GRIN_VERTEX_LIST vl) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  if (_vl->type_begin >= _g->vertex_type_num)
    return GRIN_NULL_LIST_ITERATOR;

  if (_vl->partition_type == ALL_PARTITION) {  // all partition
    auto& vertices = _g->vertices_collections[_vl->type_begin];
    auto vli = new GRIN_VERTEX_LIST_ITERATOR_T(
        _vl->type_begin, _vl->type_end, _vl->partition_type, _vl->partition_id,
        _vl->type_begin, 0, vertices.begin());
    return vli;
  }

  if (_vl->partition_type == ONE_PARTITION) {  // one partition
    // find first non-empty valid type & partition
    auto vtype = _vl->type_begin;
    while (vtype < _vl->type_end) {
      if (__grin_get_paritioned_vertex_num(_g, vtype, _vl->partition_id,
                                           _g->partition_strategy) == 0)
        vtype++;
      else
        break;
    }
    if (vtype == _vl->type_end)
      return GRIN_NULL_LIST_ITERATOR;

    // find first vertex in this partition
    auto partition_id = _vl->partition_id;
    auto& vertices = _g->vertices_collections[vtype];
    auto idx = __grin_get_first_vertex_id_in_partition(_g, vtype, partition_id,
                                                       _g->partition_strategy);
    auto vli = new GRIN_VERTEX_LIST_ITERATOR_T(
        _vl->type_begin, _vl->type_end, _vl->partition_type, _vl->partition_id,
        vtype, idx, vertices.begin() + idx);
    return vli;
  }

  if (_vl->partition_type == ALL_BUT_ONE_PARTITION) {  // all but one
    // find first non-empty valid type & partition
    auto vtype = _vl->type_begin;
    auto partition_id = 0;
    while (vtype < _vl->type_end) {
      if (partition_id == _vl->partition_id ||
          __grin_get_paritioned_vertex_num(_g, vtype, partition_id,
                                           _g->partition_strategy) == 0) {
        partition_id++;
        if (partition_id == _g->partition_num) {
          vtype++;
          partition_id = 0;
        }
      } else {
        break;
      }
    }
    if (vtype == _vl->type_end)
      return GRIN_NULL_LIST_ITERATOR;
    // find first vertex in this partition
    auto& vertices = _g->vertices_collections[_vl->type_begin];
    auto idx = __grin_get_first_vertex_id_in_partition(
        _g, _vl->type_begin, partition_id, _g->partition_strategy);
    auto vli = new GRIN_VERTEX_LIST_ITERATOR_T(
        _vl->type_begin, _vl->type_end, _vl->partition_type, _vl->partition_id,
        _vl->type_begin, idx, vertices.begin() + idx);
    return vli;
  }

  return GRIN_NULL_LIST_ITERATOR;  // undefined
}

void grin_destroy_vertex_list_iter(GRIN_GRAPH g,
                                   GRIN_VERTEX_LIST_ITERATOR vli) {
  auto _vli = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(vli);
  delete _vli;
}

void grin_get_next_vertex_list_iter(GRIN_GRAPH g,
                                    GRIN_VERTEX_LIST_ITERATOR vli) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _vli = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(vli);

  if (_vli->partition_type == ALL_PARTITION) {  // all partition
    ++_vli->current_offset;
    ++_vli->iter;
    // go to next type
    while (_vli->current_type < _vli->type_end &&
           _vli->current_offset >= _g->vertex_offsets[_vli->current_type + 1] -
                                       _g->vertex_offsets[_vli->current_type]) {
      _vli->current_type++;
      _vli->current_offset = 0;
      if (_vli->current_type < _vli->type_end) {
        auto& vertices = _g->vertices_collections[_vli->current_type];
        _vli->iter = vertices.begin();
      }
    }
  }

  if (_vli->partition_type == ONE_PARTITION) {  // one partition
    auto idx = __grin_get_next_vertex_id_in_partition(
        _g, _vli->current_type, _vli->partition_id, _g->partition_strategy,
        _vli->current_offset);
    if (idx != -1) {  // next vertex in this partition
      _vli->iter += idx - _vli->current_offset;
      _vli->current_offset = idx;
    } else {  // go to next type
      // find first non-empty valid type & partition
      while (_vli->current_type < _vli->type_end) {
        _vli->current_type++;
        _vli->current_offset = 0;
        if (__grin_get_paritioned_vertex_num(_g, _vli->current_type,
                                             _vli->partition_id,
                                             _g->partition_strategy) == 0)
          continue;
        else
          break;
      }
      // find first vertex in this partition
      if (_vli->current_type < _vli->type_end) {
        auto& vertices = _g->vertices_collections[_vli->current_type];
        auto idx = __grin_get_first_vertex_id_in_partition(
            _g, _vli->current_type, _vli->partition_id, _g->partition_strategy);
        _vli->current_offset = idx;
        _vli->iter = vertices.begin() + idx;
      }
    }
  }

  if (_vli->partition_type == ALL_BUT_ONE_PARTITION) {  // all but one
    auto idx = __grin_get_next_vertex_id_in_partition(
        _g, _vli->current_type, _vli->partition_id, _g->partition_strategy,
        _vli->current_offset);
    if (idx != -1) {  // next vertex in this partition
      _vli->iter += idx - _vli->current_offset;
      _vli->current_offset = idx;
    } else {
      // find next valid parititon in this type
      auto partition_id = __grin_get_master_partition_id(
          _g, _vli->current_offset, _vli->current_type);
      while (partition_id < _g->partition_num) {
        partition_id++;
        if (partition_id == _vli->partition_id)
          continue;  // skip invalid partition
        if (__grin_get_paritioned_vertex_num(_g, _vli->current_type,
                                             partition_id,
                                             _g->partition_strategy) == 0)
          continue;  // skip empty partition
        break;
      }
      // next valid partition in this type exists
      if (partition_id < _g->partition_num) {
        auto idx = __grin_get_first_vertex_id_in_partition(
            _g, _vli->current_type, partition_id, _g->partition_strategy);
        _vli->iter += idx - _vli->current_offset;
        _vli->current_offset = idx;
      } else {  // go to next type
        // find first non-empty valid type
        while (_vli->current_type < _vli->type_end) {
          _vli->current_type++;
          _vli->current_offset = 0;
          auto vertex_num = _g->vertex_offsets[_vli->current_type + 1] -
                            _g->vertex_offsets[_vli->current_type];
          auto partitioned_vertex_num = __grin_get_paritioned_vertex_num(
              _g, _vli->current_type, _vli->partition_id,
              _g->partition_strategy);
          if (vertex_num - partitioned_vertex_num == 0)
            continue;
          else
            break;
        }
        // non-empty valid type exists
        if (_vli->current_type < _vli->type_end) {
          // find first non-empty valid partition
          auto partition_id = 0;
          while (partition_id < _g->partition_num) {
            if (partition_id == _vli->partition_id)
              continue;  // skip invalid partition
            if (__grin_get_paritioned_vertex_num(_g, _vli->current_type,
                                                 partition_id,
                                                 _g->partition_strategy) == 0)
              continue;  // skip empty partition
            break;
            partition_id++;
          }
          if (partition_id == _g->partition_num)
            return;
          // find first vertex in this partition
          auto& vertices = _g->vertices_collections[_vli->type_begin];
          auto idx = __grin_get_first_vertex_id_in_partition(
              _g, _vli->current_type, partition_id, _g->partition_strategy);
          _vli->current_offset = idx;
          _vli->iter = vertices.begin() + idx;
        }
      }
    }
  }
}

bool grin_is_vertex_list_end(GRIN_GRAPH g, GRIN_VERTEX_LIST_ITERATOR vli) {
  if (vli == GRIN_NULL_LIST_ITERATOR)
    return true;
  auto _vli = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(vli);
  return _vli->current_type >= _vli->type_end;
}

GRIN_VERTEX grin_get_vertex_from_iter(GRIN_GRAPH g,
                                      GRIN_VERTEX_LIST_ITERATOR vli) {
  auto _vli = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(vli);
  auto v =
      new GRIN_VERTEX_T(_vli->current_offset, _vli->current_type, *_vli->iter);
  return v;
}
#endif
