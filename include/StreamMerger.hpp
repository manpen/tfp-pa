/**
 * @file
 * @brief Stream merger
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

//! @brief Merger for multiple asc. sorting streams based on a less comparator (STXXL semantics)
template <typename T, class Compare, class Stream, typename... Streams>
class StreamMerger {
private:
   using StreamMergerOthers = StreamMerger<T, Compare, Streams...>;

   Compare & _compare;
   StreamMergerOthers _others;
   Stream & _my_stream;

   T _current;
   bool _empty;

public:
   StreamMerger(Compare & compare, Stream & stream, Streams &... streams)
      : _compare(compare)
      , _others(compare, streams...)
      , _my_stream(stream)
      , _empty(false)
   {++(*this);}

//! @name STXXL Streaming Interface
//! @{
   bool empty() const {return _empty;}
   const T& operator*() const {return _current;}

   StreamMerger& operator++() {
      if (UNLIKELY(_my_stream.empty() && _others.empty())) {
         _empty = true;

      } else if (UNLIKELY(_my_stream.empty())) {
         // own stream is empty
         _current = *_others;
         ++_others;

      } else if (UNLIKELY(_others.empty())) {
         // other streams are empty
         _current = *_my_stream;
         ++_my_stream;

      } else {
         if (_compare(*_my_stream, *_others)) {
            _current = *_my_stream;
            ++_my_stream;
         } else {
            _current = *_others;
            ++_others;
         }
      }

      return *this;
   }
//! @}
};

//! @brief The base case is a plain copy of the streaming interface of the stream provided.
template <typename T, class Compare, class Stream>
class StreamMerger<T, Compare, Stream> {
   Stream & _my_stream;

public:
   StreamMerger(Compare &, Stream & stream) : _my_stream(stream) {}

//! @name STXXL Streaming Interface
//! @{
   bool empty() const {return _my_stream.empty();}
   const T& operator*() const {return *_my_stream;}
   StreamMerger& operator++() {++_my_stream; return *this;}
//! @}
};