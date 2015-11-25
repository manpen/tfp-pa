/**
 * @file
 * @brief Generator following "Directed Scale-Free Graphs"
 *
 * by B Bollobas, C. Borgs, J. Chayes, O. Riordan
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
#include <stxxl/random>
#include <stxxl/sorter>

class ModelBBCR {
public:
   using value_type = Token64;
   using sorter_type = stxxl::sorter<value_type, value_type::ComparatorAsc>;

protected:
   // generator parameters
   uint64_t _number_of_edges;
   uint64_t _vertex_id;
   uint64_t _token_id;

   // model parameters
   double _alpha;
   double _beta;
   double _degree_offset_in;
   double _degree_offset_out;

   // sorter and random sources
   sorter_type _sorter;
   stxxl::random_number64 _rand64;
   stxxl::random_uniform_slow _randdbl;

   inline value_type _random_edge(double offset) {
      value_type result;

      if (_randdbl() < ((_vertex_id*offset) / (_vertex_id*offset + _token_id/2))) {
         // uniform selection
         result = value_type(false, _token_id++, _rand64(++_vertex_id));
      } else {
         // pa selection
         result = value_type(true, _rand64(_token_id), _token_id);
         _token_id++;
      }

      return result;
   }

   void _populate() {
      const uint64_t max_token_id = _token_id + 2*_number_of_edges;
      while(_token_id < max_token_id) {
         double mode = _randdbl();

         assert(_token_id ^ 1);

         if (mode < _alpha) {
            // construct new vertex with out-going edge
            _sorter.push(value_type(false, _token_id++, _vertex_id));
            _sorter.push(_random_edge(_degree_offset_in));
            _vertex_id++;
         } else if (mode < _alpha + _beta) {
            // link to random nodes
            _sorter.push(_random_edge(_degree_offset_out));
            _sorter.push(_random_edge(_degree_offset_in));
         } else {
            // construct new vertex with in-coming edge
            _sorter.push(_random_edge(_degree_offset_out));
            _sorter.push(value_type(false, _token_id++, _vertex_id++));
         }
      }
   }

public:
   ModelBBCR(
         uint64_t number_of_edges, uint64_t first_vertex_id, uint64_t first_edge_id,
         double alpha, double beta,
         double degree_offset_in, double degree_offset_out,
         stxxl::unsigned_type sorter_size
   )
         : _number_of_edges(number_of_edges)
         , _vertex_id(first_vertex_id), _token_id(2*first_edge_id)
         , _alpha(alpha), _beta(beta)
         , _degree_offset_in(degree_offset_in), _degree_offset_out(degree_offset_out)
         , _sorter(value_type::ComparatorAsc(), sorter_size)
   {
      _populate();
      _sorter.sort();
   }

   sorter_type & sorter() {
      return _sorter;
   }
};
