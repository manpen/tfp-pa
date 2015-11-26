#include <iostream>
#include <cstdint>

#include <stxxl/cmdline>

#include <stxxl/io>
#include <stxxl/vector>
#include <stxxl/sorter>
#include <stxxl/bits/common/uint_types.h>

#include <GenericComparator.hpp>
#include <DistributionCount.hpp>
#include <FileDataType.hpp>

using FileT = DefaultFileDataType::data_type;
using sorter_type = stxxl::sorter<FileT, GenericComparator<FileT>::Ascending>;

void count_and_display_degree(sorter_type & sorter, std::ostream * outstream) {
   sorter.sort();

// Count degrees
   FileDataTypeReader<sorter_type> casting_stream(sorter);
   DistributionCount<decltype(casting_stream)> degree_count(casting_stream);

   stxxl::sorter<uint64_t, GenericComparator<uint64_t>::Ascending>
      degree_sorter(GenericComparator<uint64_t>::Ascending(), 1llu << 30);

   while(!degree_count.empty()) {
      degree_sorter.push( (*degree_count).count );

      ++degree_count;
   }

   degree_sorter.sort();

// Distribution count
   DistributionCount<decltype(degree_sorter)> distr_count(degree_sorter);

   uint64_t degree_sum = 0;

   while(!distr_count.empty()) {
      auto desc = *distr_count;
      ++distr_count;

      (*outstream) << desc.value << " " << desc.count << std::endl;
      degree_sum += desc.value * desc.count;
   }
}

int main(int argc, char* argv[]) {
   bool directed_graph = false;
   std::vector<std::string> filenames;
   std::string filename_out;
   {
      stxxl::cmdline_parser cp;
      cp.set_author("Manuel Penschuck <manuel at ae.cs.uni-frankfurt.de>");
      cp.set_description("EM distribution counter from edge list");
      cp.add_flag('d', "directed", directed_graph, "Input is a directed edge list; default false");
      cp.add_param_stringlist("input-files", filenames, "Input files; if mutliple files are given they are interpreted as concatenated");
      cp.add_string('o', "output-file", filename_out, "Name of the output file");
      if (!cp.process(argc, argv)) return -1;
   }

   std::cout << "Using " << (8 * sizeof(DefaultFileDataType::data_type)) << "-bit unsinged integers for input" << std::endl;

   stxxl::stats* Stats = stxxl::stats::get_instance();
   stxxl::stats_data stats_begin(*Stats);     
   
// Read and Sort nodes
   sorter_type node_in_sorter(GenericComparator<FileT>::Ascending(), 1llu << 30);
   sorter_type node_out_sorter(GenericComparator<FileT>::Ascending(), 1llu << 30);
   size_t edges = 0;

   // Input handling
   for(auto & filename : filenames) {
      // Open File as a STXXL vector
      stxxl::linuxaio_file input_file(filename, stxxl::file::RDONLY | stxxl::file::DIRECT);
      DefaultFileDataType::vector_type input_vector(&input_file);

      // Copy file into sorter(s)
      bool out_edge = true;
      for(auto node : typename DefaultFileDataType::vector_type::bufreader_type(input_vector)) {
         if (out_edge || !directed_graph) {
            node_out_sorter.push(node);
         } else {
            node_in_sorter.push(node);
         }
         out_edge = !out_edge;
      }

      // Print progress info
      auto this_edges = input_vector.size() / 2;
      edges += this_edges;
      std::cout << "Read " << this_edges << " edges from file " << filename << std::endl;
   }
   std::cout << "# Number of edges: " << edges << std::endl;

   std::ofstream result_file;
   if (!filename_out.empty())
      result_file.open(filename_out);

   std::ostream* result_stream = & std::cout;
   if (result_file.is_open())
      result_stream = &result_file;

   if (!directed_graph) {
      count_and_display_degree(node_out_sorter, result_stream);
   } else {
      (*result_stream) << "# Out-Degrees" << std::endl;
      count_and_display_degree(node_out_sorter, result_stream);

      (*result_stream) << std::endl << std::endl << "# In-Degrees" << std::endl;
      count_and_display_degree(node_in_sorter,  result_stream);
   }

// Output report   
   stxxl::stats_data stats_final(*Stats);        
   std::cout << "Final: " << (stats_final - stats_begin);
   
   return 0;
}