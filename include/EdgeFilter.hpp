/**
 * @file
 * @brief Filter self-loops and multi-edges in stream
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

/**
 * @brief Filter self-loops and multi-edges in stream
 * @tparam Input Stream, to detect multi-edge, the stream has to be lexicographically sorted
 */
template <class InputStream>
class EdgeFilter {
public:
   using value_type = typename InputStream::value_type;

protected:
   InputStream & _stream;
   bool _empty;

   bool _self_loops;
   bool _multi_edges;

   value_type _current_edge;
   value_type _last_edge;

   void _fetch(bool initial_fetch) {
      _last_edge = _current_edge;

      bool issue = true;
      while(issue && !_stream.empty()) {
         auto & candidate = *_stream;

         issue  = (_self_loops && candidate.first == candidate.second)
               || (_multi_edges && !initial_fetch && _last_edge == candidate);

         if (LIKELY(!issue))
            _current_edge = candidate;

         ++_stream;
      }

      _empty = issue;
   }

public:
   /**
    * @param stream      Input stream
    * @param self_loops  If true, self-loops are filtered
    * @param multi_edges If true, multi-edges are reduced to a single edge
    * @warning If @p multi_edges is asserted, the input stream has to be sorted lexicographically
    */
   EdgeFilter(InputStream & stream, bool self_loops = false, bool multi_edges = false)
         : _stream(stream)
         , _empty(false)
         , _self_loops(self_loops)
         , _multi_edges(multi_edges)
   {
      _fetch(true);
   }

//! @name STXXL Streaming Interface
//! @{
   bool empty() const {return _empty;}
   const value_type & operator*() const {return _current_edge;}
   EdgeFilter & operator++() {_fetch(false); return *this;}
//! @}
};