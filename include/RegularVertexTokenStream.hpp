/**
 * @file
 * @brief Produces a regular sequence of vertex tokens
 *
 * @author Manuel Penschuck
 * @copyright
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * @copyright
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

//! @brief Produces a regular sequence of vertex tokens
class RegularVertexTokenStream {
public:
   using value_type = Token64;

protected:
   // Model parameter
   const uint64_t _vertex_end;
   const uint64_t _edges_per_vertex;

   uint64_t _current_vertex;
   uint64_t _current_edge;
   uint64_t _edge_list_idx;

   value_type _current_token;

   bool _empty;

public:
   /**
    * @param first_vertex         The first vertex id emitted
    * @param first_edge_list_idx  The position of the first vertex in edge list,
    *                             the next token will target first_edge_list_idx+2
    * @param number_of_vertices   The number of vertices to be produced
    * @param edges_per_vertex     The multiplicity a vertex before its id is increased
    */

   RegularVertexTokenStream(
         uint64_t first_vertex, uint64_t first_edge_list_idx,
         uint64_t number_of_vertices, uint64_t edges_per_vertex
   )
         : _vertex_end(first_vertex + number_of_vertices)
         , _edges_per_vertex(edges_per_vertex)
         , _current_vertex(first_vertex)
         , _current_edge(0)
         , _edge_list_idx(first_edge_list_idx)
         , _empty(false)
   {++(*this);}

//! @name STXXL Streaming Interface
//! @{
   RegularVertexTokenStream& operator++() {
      _empty = _empty || (_current_vertex >= _vertex_end);

      _current_token = Token64(false, _edge_list_idx, _current_vertex );
      _edge_list_idx += 2;

      if (++_current_edge >= _edges_per_vertex) {
         _current_vertex++;
         _current_edge = 0;
      }

      return *this;
   }

   bool empty() const {
      return _empty;
   }

   const value_type& operator*() const {
      return _current_token;
   }

//! @}
};