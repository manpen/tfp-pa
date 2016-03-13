#include <iostream>
#include <cstdint>

#include <stxxl/cmdline>

#include <stxxl/io>
#include <stxxl/vector>
#include <stxxl/bits/common/uint_types.h>

#include <GenericComparator.hpp>
#include <DistributionCount.hpp>
#include <FileDataType.hpp>

#include <list>
#include <vector>
#include <queue>

using NodeT = uint64_t;
using FileT = DefaultFileDataType::data_type;
using AdjListT = std::vector<std::list<NodeT>>;

void bfs(const AdjListT & adj) {
    std::vector<bool> visited(adj.size());
    uint64_t number_components = 0;
    uint64_t vertices_visited = 0;

    for(NodeT start = 0; start < adj.size(); start++) {
        if (visited[start]) continue;
        number_components++;

        std::queue<NodeT> queue;
        queue.push(start);

        while(!queue.empty()) {
            auto & current = queue.front();
            if (!visited[current]) {
                vertices_visited++;
                visited[current] = true;
                for(const auto & n : adj[current])
                    if (!visited[n])
                        queue.push(n);
            }
            queue.pop();
        }

        if (vertices_visited == adj.size())
            break;
    }

    std::cout << "Number of components found: " << number_components << std::endl;
    std::cout << "Vertices visited: " << vertices_visited << std::endl;

    if (vertices_visited != adj.size()) {
        std::cerr << "------- Unvisited Vertices ---------" << std::endl;
    }
}


int main(int argc, char* argv[]) {
    bool directed_graph = false;
    std::vector<std::string> filenames;
    std::string filename_out;
    stxxl::uint64 adj_list_size;
    {
        stxxl::cmdline_parser cp;
        cp.set_author("Manuel Penschuck <manuel at ae.cs.uni-frankfurt.de>");
        cp.set_description("IM BFS implementation to study connectedness of small graphs");
        cp.add_flag('d', "directed", directed_graph, "Input is a directed edge list; default false");
        cp.add_param_stringlist("input-files", filenames, "Input files; if multiple files are given they are interpreted as concatenated");
        cp.add_bytes('n', "no-vertices", adj_list_size, "Number of vertices; give an upper bound; may speed up build of adj list");
        if (!cp.process(argc, argv)) return -1;
    }

    std::cout << "Using " << (8 * sizeof(DefaultFileDataType::data_type)) << "-bit unsinged integers for input" << std::endl;
    std::cout << "Underlying graph is " << (directed_graph?"DIRECTED":"UNdirected") << std::endl;


// Build adjacency list
    AdjListT adj_list(adj_list_size);
    size_t edges = 0;

    // Input handling
    NodeT max_vertex = 0;
    for(auto & filename : filenames) {
        // Open File as a STXXL vector
        stxxl::linuxaio_file input_file(filename, stxxl::file::RDONLY | stxxl::file::DIRECT);
        DefaultFileDataType::vector_type input_vector(&input_file);

        // Copy file into adjacency list
        bool out_edge = true;
        NodeT from;
        NodeT to;

        for(auto node : typename DefaultFileDataType::vector_type::bufreader_type(input_vector)) {
            if (out_edge) {
                from = DefaultFileDataType::toInternal(node);
            } else {
                to = DefaultFileDataType::toInternal(node);

                // compute maximal vertex
                auto max = std::max(from, to);
                max_vertex = std::max(max_vertex, max);

                // ... and increase array size if it does not fit
                if (max >= adj_list.size())
                    adj_list.resize(std::max(adj_list.size() * 2, max+1));

                // insert nodes into adj_list
                adj_list[from].push_back(to);

                if (!directed_graph) {
                    adj_list[to].push_back(from);
                }

            }
            out_edge = !out_edge;
        }

        // Print progress info
        auto this_edges = input_vector.size() / 2;
        edges += this_edges;
        std::cout << "Read " << this_edges << " edges from file " << filename << std::endl;
    }
    std::cout << "# Number of vertices: " << (max_vertex+1) << std::endl;
    std::cout << "# Number of edges: " << edges << std::endl;

    adj_list.resize(max_vertex+1);
    adj_list.shrink_to_fit();

    // Remove duplicates in adj lists
    {
        uint64_t elements_removed = 0;
        for(auto & a : adj_list) {
            elements_removed += a.size();
            a.sort();
            a.unique();
            elements_removed -= a.size();
        }
        std::cout << "# Number of duplicated edges removed: " << elements_removed << std::endl;
    }

    bfs(adj_list);

    return 0;
}