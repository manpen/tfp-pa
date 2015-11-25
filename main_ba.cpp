/**
 * @file
 * @brief Main of the Barabasi-Albert graph generator
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
#include <iostream>

#include <stxxl/cmdline>
#include <stxxl/sorter>
#include <stxxl/bits/containers/priority_queue.h>

#include <InitialCircle.hpp>
#include <RegularVertexTokenStream.hpp>
#include <RandomInteger.hpp>
#include <StreamMerger.hpp>
#include <ProcessTokenSequence.hpp>

#include <EdgeWriter.hpp>
#include <EdgeSorter.hpp>
#include <EdgeFilter.hpp>


int main(int argc, char* argv[]) {
   // parse command-line arguments
   uint64_t number_of_vertices = 1;
   uint64_t edges_per_vertex = 2;
   bool edge_dependencies = true;

   bool filter_self_loops = false;
   bool filter_multi_edges = false;

   std::string output_file;

   {
      stxxl::cmdline_parser cp;
      cp.set_author("Manuel Penschuck <manuel at ae.cs.uni-frankfurt.de>");
      cp.set_description("Barabasi-Albert Preferential Attachment EM Graph Generator");

      cp.add_param_string("filename", output_file, "Path to output file");

      stxxl::uint64 verts, epv;
      cp.add_param_bytes("no-vertices", verts, "Number of random vertices; positive");
      cp.add_param_bytes("edges-per-vert", epv, "Edge per random vertex; positive");

      cp.add_flag('d', "edge-dependencies", edge_dependencies, "Dependencies between edges of same vertex");
      cp.add_flag('s', "filter-self-loops", filter_self_loops, "Remove all self-loops (w/o replacement)");
      cp.add_flag('m', "filter-multi-edges", filter_multi_edges, "Collapse parallel edges into a single one");

      if (!cp.process(argc, argv)) return -1;

      if (!verts || !epv) {
         cp.print_usage();
         return -1;
      }

      // apply config
      cp.print_result();
      number_of_vertices = verts;
      edges_per_vertex = epv;
   }

   // compile-time config
   const unsigned int sorter_size = 1 << 30;
   constexpr size_t pq_size = 1 << 30;

   // This stream yields all token to define a small initial circle
   InitialCircle seedTokens(2 * edges_per_vertex);

   // This stream gives all "fixed" vertices of the edge list
   RegularVertexTokenStream regularTokens(
      seedTokens.maxVertexId() + 1,
      2*seedTokens.numberOfEdges(),
      number_of_vertices,
      edges_per_vertex
   );

   // Now generate random indices and substantially sort them,
   // to ensure that they are available at the moment in time,
   // when the queried value is produced
   Token64::ComparatorAsc comparator;
   stxxl::sorter<Token64, Token64::ComparatorAsc> randomTokens(comparator, sorter_size);

   uint64_t weight = seedTokens.numberOfEdges() * 2;
   uint64_t idx = weight + 1;
   for(uint64_t vertex = 0; vertex < number_of_vertices; vertex++) {
      uint64_t this_weight = weight;
      for(uint64_t edge = 0; edge < edges_per_vertex; edge++) {
         Token64 token(true, RandomInteger<8>::randint(this_weight), idx);
         randomTokens.push(token);
         this_weight += 2 * edge_dependencies;
         idx += 2;
      }

      weight += 2 * edges_per_vertex;
   }
   randomTokens.sort();

   // Merge all these streams
   using merger_type = StreamMerger<
         Token64, Token64::ComparatorAsc,
         decltype(regularTokens), decltype(randomTokens), decltype(seedTokens)
   >;
   merger_type merger(comparator, regularTokens, randomTokens, seedTokens);

   // Setup priority queue
   // we need an desc comparator, since its a max-pq and we want the smallest element on top
   using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<Token64, Token64::ComparatorDesc, pq_size, size_t(1) << 30>::result;
   //using pq_type = stxxl::priority_queue<stxxl::priority_queue_config<Token<unsigned long>, Token<unsigned long>::ComparatorDesc, 32u, 8192u, 64u, 2u, 4194304u, 64u, 2u, stxxl::RC>>;
   pq_type prio_queue(pq_size / 2, pq_size / 2);

   // Process streams
   ProcessTokenSequence<decltype(merger), decltype(prio_queue)> process(merger, prio_queue);

   // Write graph into file
   EdgeWriter<> edge_writer(output_file, seedTokens.numberOfEdges() + number_of_vertices*edges_per_vertex);


   if (filter_self_loops || filter_multi_edges) {
      EdgeSorter<decltype(process)> sortedEdges(process, sorter_size);
      EdgeFilter<decltype(sortedEdges)> filteredEdges(sortedEdges, filter_self_loops, filter_multi_edges);
      edge_writer.writeEdges(filteredEdges);
   } else {
      edge_writer.writeVertices(process);
   }

   std::cout << "Wrote " << edge_writer.edgesWritten() << " edges" << std::endl;

   return 0;
}
