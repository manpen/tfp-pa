/**
 * @file
 * @brief Main of the BBCR graph generator
 *
 * Generator following "Directed Scale-Free Graphs" by
 * B Bollobas, C. Borgs, J. Chayes, O. Riordan
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
#include <iostream>

#include <stxxl/cmdline>
#include <stxxl/sorter>
#include <stxxl/bits/containers/priority_queue.h>

#include <InitialCircle.hpp>
#include <StreamMerger.hpp>
#include <ProcessTokenSequence.hpp>

#include <EdgeWriter.hpp>
#include <EdgeSorter.hpp>
#include <EdgeFilter.hpp>

#include "models/ModelBBCR.hpp"

int main(int argc, char* argv[]) {
   // parse command-line arguments
   uint64_t number_of_seed_vertices = 2;
   uint64_t number_of_edges = 1;

   bool filter_self_loops = false;
   bool filter_multi_edges = false;

   double alpha = 0.1;
   double beta  = 0.8;
   double gamma = 0.1;

   double degree_offset_out = 0.0;
   double degree_offset_in = 0.0;

   std::string output_file;

   {
      stxxl::cmdline_parser cp;
      cp.set_author("Manuel Penschuck <manuel at ae.cs.uni-frankfurt.de>");
      cp.set_description(
            "Directed Preferential Attachment EM Graph Generator\n"
            "Model based on >Directed Scale-Free Graphs< by \n"
            "B Bollobas, C. Borgs, J. Chayes, O. Riordan"
      );

      cp.add_param_string("filename", output_file, "Path to output file");

      stxxl::uint64 edges, seed_verts;
      cp.add_param_bytes("no-edges", edges, "Number of random edges; positive");
      cp.add_bytes('n', "seed-vertices", seed_verts, "Number of seed vertices");

      cp.add_double('a', "alpha", alpha, "Relative prob. to add new vertex with outgoing edge");
      cp.add_double('b', "beta",  beta,  "Relative prob. to link two existing vertices");
      cp.add_double('g', "gamma", gamma, "Relative prob. to add new vertex with incoming edge");

      cp.add_double('y', "d-in", degree_offset_in, "Non-negative offset in  in-degree distribution");
      cp.add_double('z', "d-out", degree_offset_out, "Non-negative offset in  in-degree distribution");

      cp.add_flag('s', "filter-self-loops", filter_self_loops, "Remove all self-loops (w/o replacement)");
      cp.add_flag('m', "filter-multi-edges", filter_multi_edges, "Collapse parallel edges into a single one");

      if (!cp.process(argc, argv)) return -1;

      if (alpha < 0 || beta < 0 || gamma < 0 || (alpha + beta + gamma) < 1e-9) {
         std::cout << "alpha, beta, gamma >= 0" << std::endl;
         cp.print_usage();
         return -1;
      } else {
         double norm = alpha + beta + gamma;
         alpha /= norm;
         beta  /= norm;
         //gamma /= norm;
      }

      if (degree_offset_in < 0 || degree_offset_out < 0) {
         std::cout << "d-in, d-out >= 0" << std::endl;
         cp.print_usage();
         return -1;
      }

      if (!edges || seed_verts < 2) {
         std::cout << "no-edges > 0; seed_verts > 1" << std::endl;
         cp.print_usage();
         return -1;
      }

      // apply config
      cp.print_result();
      number_of_edges = edges;
      number_of_seed_vertices = seed_verts;
   }

   // compile-time config
   const unsigned int sorter_size = 1 << 30;
   constexpr size_t pq_size = 1 << 30;

   // This stream yields all token to define a small initial circle
   InitialCircle seedTokens(number_of_seed_vertices);

   // Now generate random indices and substantially sort them,
   // to ensure that they are available at the moment in time,
   // when the queried value is produced
   ModelBBCR model(
         number_of_edges,
         seedTokens.maxVertexId() + 1,
         seedTokens.numberOfEdges(),
         alpha, beta,
         degree_offset_in, degree_offset_out,
         sorter_size
   );

   // Merge all these streams
   using merger_type = StreamMerger<Token64, Token64::ComparatorAsc, typename decltype(model)::sorter_type, decltype(seedTokens)>;
   Token64::ComparatorAsc compare;
   merger_type merger(compare, model.sorter(), seedTokens);

   // Setup priority queue
   // we need an desc comparator, since its a max-pq and we want the smallest element on top
   using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<Token64, Token64::ComparatorDesc, pq_size, size_t(1) << 20>::result;
   //using pq_type = stxxl::priority_queue<stxxl::priority_queue_config<Token<unsigned long>, Token<unsigned long>::ComparatorDesc, 32u, 8192u, 64u, 2u, 4194304u, 64u, 2u, stxxl::RC>>;
   pq_type prio_queue(pq_size / 2, pq_size / 2);

   // Process streams
   ProcessTokenSequence<decltype(merger), decltype(prio_queue)> process(merger, prio_queue);

   // Write graph into file
   EdgeWriter<> edge_writer(output_file, seedTokens.numberOfEdges() + number_of_edges);

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
