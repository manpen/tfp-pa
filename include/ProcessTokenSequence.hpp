/**
 * @file
 * @brief Main loop of TFP processing, i.e. materialize edge and answer queries
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

/**
 * @brief Main loop of TFP processing, i.e. materialize edge and answer queries
 *
 * Given a stream of tokens, create tokens cause that a vertex is written into the
 * edge list while query tokens look-up the last value written and get reinserted
 * into the priority queue. For more details see
 * "Generating Massive Scale-Free Networks under Resource Constraints" by U.Meyer/M.Penschuck
 */
template <class InputStream, class PriorityQueue, class Token = Token64>
class ProcessTokenSequence {
public:
   using value_type = uint64_t;
   using token_type = Token;

protected:
   InputStream & _stream;
   PriorityQueue & _prio_queue;

   uint64_t _current_idx;
   bool _empty;
   uint64_t _current_vertex;

   //! Returns false if a new vertex was "produced"
   bool _processToken(const Token & token) {
      std::cout << token << std::endl;
      if (token.query()) {
         assert(_current_idx -1 == token.id());
         Token64 new_token(false, token.value(), _current_vertex);
         _prio_queue.push(new_token);
         return true;

      } else {
         _current_vertex = token.value();
         _current_idx++;
         return false;

      }
   }

public:
   ProcessTokenSequence(InputStream& stream, PriorityQueue & pq)
      : _stream(stream)
      , _prio_queue(pq)
      , _current_idx(0)
      , _empty(false)
   {++(*this);}

//! @name STXXL Streaming Interface
//! @{
   //! Compute next token
   ProcessTokenSequence & operator++() {
      bool repeat = true;
      while(repeat) {
         if (_prio_queue.empty() && _stream.empty()) {
            _empty = true;
            repeat = false;
         } else if (_prio_queue.empty()) {
            repeat = _processToken(*_stream);
            ++_stream;
         } else if (_stream.empty()) {
            repeat = _processToken(_prio_queue.top());
            _prio_queue.pop();
         } else if (*_stream < _prio_queue.top()) {
            repeat = _processToken(*_stream);
            ++_stream;
         } else {
            repeat = _processToken(_prio_queue.top());
            _prio_queue.pop();
         }
      }

      return *this;
   }

   //! Indicates whether the last increment operation generated a valid value
   bool empty() const {
      return _empty;
   }

   //! Constant reference to the current token (valid only in case !empty)
   const value_type & operator*() const {
      return _current_vertex;
   }
//! @}
};