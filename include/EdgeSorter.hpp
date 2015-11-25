/**
 * @file
 * @brief Receives a stream of vertices, combines neighbors to edges and sorts them lexicographically
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

#include <utility>
#include <stxxl/sorter>

//! @brief Receives a stream of vertices, combines neighbors to edges and sorts them lexicographically
template <class InputStream>
class EdgeSorter {
public:
   using vertex_type = typename InputStream::value_type;
   using edge_type = std::pair<vertex_type, vertex_type>;
   using value_type = edge_type;

protected:
   struct Compare {
      bool operator()(const edge_type & a, const edge_type & b) const {return a < b;}
      edge_type min_value() const {return edge_type(std::numeric_limits<vertex_type>::min(),
                                                    std::numeric_limits<vertex_type>::min());}
      edge_type max_value() const {return edge_type(std::numeric_limits<vertex_type>::max(),
                                                    std::numeric_limits<vertex_type>::max());}
   };

   stxxl::sorter<edge_type, Compare> _sorter;

public:
   EdgeSorter(InputStream & stream, stxxl::unsigned_type mem_for_sorter = 1u << 31)
      : _sorter(Compare(), mem_for_sorter)
   {
      for(; !stream.empty(); ++stream) {
         edge_type edge;

         // Extract first vertex
         edge.first = *stream;

         // Advance and ensure their's another vertex
         ++stream;
         assert(!stream.empty());
         edge.second = *stream;

         // Push it into the sorter
         _sorter.push(edge);
      }

      _sorter.sort();
   }

//! @name STXXL Streaming Interface
//! @{
// straight-forward copy of the sorter interface !
   bool empty() const {return _sorter.empty();}
   EdgeSorter &operator++() {++_sorter; return *this;}
   const value_type & operator*() const {return *_sorter;}
//! @}
};