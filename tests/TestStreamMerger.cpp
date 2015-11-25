/**
 * @file
 * @brief Tests for StreamMerger
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
#include <gtest/gtest.h>

#include <random>
#include <vector>
#include <list>

#include <RandomInteger.hpp>
#include <GenericComparator.hpp>
#include <StreamMerger.hpp>
#include <stxxl/bits/stream/stream.h>

class TestStreamMerger : public ::testing::Test {
protected:
   using Comparator = GenericComparator<unsigned int>::Ascending;

   struct EmptyStream {
      unsigned int value;
      bool empty() {return true;}
      EmptyStream& operator++() {return *this;}
      unsigned int & operator*() {return value;}
      const unsigned int & operator*() const {return value;}
   };

   std::vector<std::list<unsigned int>> _generateStreamData(unsigned int no_streams, unsigned int no_items) {
      std::vector<unsigned int> no_items_per_stream(no_streams);
      {
         unsigned int no_items_assigned = 0;
         for(unsigned int i=1; i < no_streams; i++) {
            unsigned int assigned = RandomInteger<4>::randint(no_items - no_items_assigned);
            no_items_per_stream[i] = assigned;
            no_items_assigned += assigned;
         }
         no_items_per_stream[0] = no_items - no_items_assigned;
      }

      std::vector<std::list<unsigned int>> stream_data(no_streams);
      for(unsigned int i = 0; i < no_items; i++) {
         unsigned int key = RandomInteger<4>::randint(no_items - i);
         unsigned int stream_idx = 0;
         {
            unsigned int tmp = 0;
            for(; tmp <= key; stream_idx++) {
               tmp += no_items_per_stream[stream_idx];
            }
            stream_idx--;
            no_items_per_stream[stream_idx]--;
         }

         stream_data[stream_idx].push_back(i);
      }

      return stream_data;
   }

   std::vector<stxxl::stream::iterator2stream<typename std::list<unsigned int>::iterator>>
   _generateStreams(std::vector<std::list<unsigned int>> & data) {
      std::vector<stxxl::stream::iterator2stream<typename std::list<unsigned int>::iterator>> result;
      result.reserve(data.size());
      for(auto & list : data) {
         result.push_back(stxxl::stream::streamify(list.begin(), list.end()));
      }
      return result;
   }

   template <class Stream>
   void _assert_coverage(Stream & stream, unsigned int no_items) {
      for(unsigned int i = 0; i < no_items; i++) {
         ASSERT_FALSE(stream.empty()) << "i: " << i << " no_items: " << no_items;
         ASSERT_EQ(i, *stream) << "no_items: " << no_items;
         ++stream;
      }
      ASSERT_TRUE(stream.empty());
   }
};

/**
 * Construct StreamMerger with one and two empty streams and check
 * that it is empty on first item
 */
TEST_F(TestStreamMerger, initialEmpty) {
   Comparator comp;

   {
      EmptyStream e1;
      StreamMerger<unsigned int, Comparator, EmptyStream> stream_merger(comp, e1);
      ASSERT_TRUE( stream_merger.empty() );
   }

   {
      EmptyStream e1, e2;
      StreamMerger<unsigned int, Comparator, EmptyStream, EmptyStream> stream_merger(comp, e1, e2);
      ASSERT_TRUE( stream_merger.empty() );
   }

   {
      EmptyStream e1, e2, e3;
      StreamMerger<unsigned int, Comparator, EmptyStream, EmptyStream, EmptyStream> stream_merger(comp, e1, e2, e3);
      ASSERT_TRUE( stream_merger.empty() );
   }
}

/**
 * Construct merger over three input streams with randomized values
 * and size; all streams have disjoint values and perfectly cover
 * an integer interval [0:x]. It then is tested, that all values
 * are correctly merged and returned in the correct order.
 */
TEST_F(TestStreamMerger, coverage3) {
   const unsigned int no_items = 1024 + RandomInteger<4>::randint(1000);
   auto data = _generateStreamData(3, no_items);
   auto streams = _generateStreams(data);
   using stream = typename decltype(streams)::value_type;

   Comparator comp;
   StreamMerger<unsigned int, Comparator, stream, stream, stream>
         stream_merger(comp, streams[0], streams[1], streams[2]);

   _assert_coverage(stream_merger, no_items);
}

TEST_F(TestStreamMerger, coverage2) {
   const unsigned int no_items = 1024 + RandomInteger<4>::randint(1000);
   auto data = _generateStreamData(2, no_items);
   auto streams = _generateStreams(data);
   using stream = typename decltype(streams)::value_type;

   Comparator comp;
   StreamMerger<unsigned int, Comparator, stream, stream>
         stream_merger(comp, streams[0], streams[1]);

   _assert_coverage(stream_merger, no_items);
}

TEST_F(TestStreamMerger, coverage1) {
   const unsigned int no_items = 1024 + RandomInteger<4>::randint(1000);
   auto data = _generateStreamData(1, no_items);
   auto streams = _generateStreams(data);
   using stream = typename decltype(streams)::value_type;

   Comparator comp;
   StreamMerger<unsigned int, Comparator, stream>
         stream_merger(comp, streams[0]);

   _assert_coverage(stream_merger, no_items);
}