/**
 * @file
 * @brief Stream generator producing edges of a circle graph with n vertices.
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

#include <stxxl/bits/common/utils.h>
#include <Token.hpp>

//! @brief Stream generator producing edges of a circle graph with n vertices.
class InitialCircle {
public:
   using value_type = Token64;

protected:
   uint64_t _number_of_tokens;
   uint64_t _first_vertex_id;
   uint64_t _current_token_id;

   value_type _current_token;

public:
   explicit InitialCircle(uint64_t number_of_vertices, uint64_t first_id = 0)
      : _number_of_tokens(2*number_of_vertices)
      , _first_vertex_id(first_id)
      , _current_token_id(0)
   {++(*this);}

   //! Highest vertex id that will be used by this generator
   uint64_t maxVertexId() const {
      return _first_vertex_id + _number_of_tokens / 2 - 1;
   }

   //! Number of edges that will be produced in total
   uint64_t numberOfEdges() const {
      return _number_of_tokens / 2;
   }

//! @name STXXL Streaming Interface
//! @{
   //! Compute next token
   InitialCircle & operator++() {
      if (UNLIKELY(_current_token_id >=_number_of_tokens - 1)) {
         // the last's edges neighbor point back to the first edge
         _current_token = Token64(false, _current_token_id, _first_vertex_id);
      } else {
         _current_token = Token64(false, _current_token_id, _first_vertex_id + (_current_token_id + 1) / 2);
      }

      _current_token_id++;

      return *this;
   }

   //! Indicates whether the last increment operation generated a valid value
   bool empty() const {
      return _current_token_id > _number_of_tokens;
   }

   //! Constant reference to the current token (valid only in case !empty)
   const value_type & operator*() const {
      return _current_token;
   }
//! @}
};
