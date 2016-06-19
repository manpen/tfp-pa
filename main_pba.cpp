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

#define PTFP_VERBOSE


#include <iostream>

#include <stxxl/cmdline>

#if defined(NDEBUG) and defined(DEBUG_PPQ)
#define NNDEBUG
#undef NDEBUG
#endif

#include <stxxl/bits/containers/parallel_priority_queue.h>

#ifdef NNDEBUG
#define NDEBUG
#endif

#include <stxxl/timer>
#include <stxxl/bits/unused.h>

#include <RandomInteger.hpp>
#include <ReservoirSampling.hpp>

#include <EdgeWriter.hpp>
#include <EdgeWriterPool.hpp>


using Node = uint64_t;
using EdgeId = uint64_t;
constexpr Node invalid_node = std::numeric_limits<Node>::max();

struct Token {
    bool query;
    Node index;
    Node value;
};

class TokenCompressed {
    constexpr static uint64_t mask = (1l << 47) - 1;

    uint64_t _data1;
    uint32_t _data2;

public:
    TokenCompressed() {}

    TokenCompressed(const bool & query, const Node & index, const Node & value)
            : _data1((index << 17) | (static_cast<uint64_t>(query) << 16) | (value >> 32))
            , _data2(static_cast<uint32_t>(value))
    {

        assert((index & mask) == index);
        assert((value & mask) == value);
    }

    TokenCompressed(const Token & t) : TokenCompressed(t.query, t.index, t.value) {}

    TokenCompressed(const uint64_t & data1, const uint32_t & data2) :
        _data1(data1), _data2(data2)
    {}

    bool is_query() const {
        return static_cast<bool>(_data1 & (1l << 16));
    }

    Token decompress() const {
        Token t;
        t.query = is_query();
        t.index = _data1 >> 17;
        t.value = ((_data1 & 0x7FFF) << 32) | _data2;
        return t;
    }

    operator Token() const {return decompress();}

    bool operator<(const TokenCompressed & r) const {
        return std::tie(_data1, _data2) < std::tie(r._data1, r._data2);
    }

    struct ComparatorDesc {
        bool operator()(const TokenCompressed & a, const TokenCompressed & b) const {return b < a;}
        TokenCompressed min_value() const {return TokenCompressed(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint32_t>::max());}
        TokenCompressed max_value() const {return TokenCompressed(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint32_t>::min());}
    };

    struct ComparatorAsc {
        bool operator()(const TokenCompressed & a, const TokenCompressed & b) const {return a < b;}
        TokenCompressed min_value() const {return TokenCompressed(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint32_t>::min());}
        TokenCompressed max_value() const {return TokenCompressed(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint32_t>::max());}
    };
} __attribute__((packed));

std::ostream& operator << (std::ostream& stream, const Token &q) {
    stream << "<Token " << (q.query ? "query" : "link ") << " Id: " << q.index << " Value: " << q.value << ">";
    return stream;
}

class RAGPath {
    const EdgeId _number_of_edges;

public:
    RAGPath(const EdgeId & number_of_edges)
            : _number_of_edges(number_of_edges)
    {}

    // Return node at position in edge list
    Node operator()(const EdgeId & idx) const {
        return idx / 2 + (idx & 1);
    }

    //! Highest vertex id that will be used by this generator
    Node maxVertexId() const {
        return _number_of_edges;
    }

    //! Number of edges that will be produced in total
    EdgeId numberOfEdges() const {
        return _number_of_edges;
    }
};


constexpr size_t pq_size = 1llu << 32;
constexpr size_t pq_max_extract = pq_size / 8;


using ppq_type = stxxl::parallel_priority_queue<
    TokenCompressed, TokenCompressed::ComparatorDesc,
    STXXL_DEFAULT_ALLOC_STRATEGY,
    8*STXXL_DEFAULT_BLOCK_SIZE(Token64), /* BlockSize */
    pq_size                    /* RamSize */
>;

using buffer_type = std::vector<TokenCompressed>;
using buffer_iter = typename buffer_type::iterator;


int main(int argc, char* argv[]) {
    std::cout << "Debug mode: "
#ifdef NDEBUG
        << "no"
#else
        << "yes"
#endif
    << std::endl;


    // parse command-line arguments
    uint64_t number_of_vertices = 1;
    uint64_t edges_per_vertex = 2;

    const uint64_t min_batch_size = 1l << 14;

    bool edge_dependencies = false;

    bool filter_self_loops = false;
    bool filter_multi_edges = false;

    stxxl::set_seed(1);

    unsigned int number_of_threads = omp_get_max_threads();
    {
        uint32_t seed = 0;

        stxxl::cmdline_parser cp;
        cp.set_author("Manuel Penschuck <manuel at ae.cs.uni-frankfurt.de>");
        cp.set_description("Barabasi-Albert Preferential Attachment EM Graph Generator");

        stxxl::uint64 verts, epv;
        cp.add_param_bytes("no-vertices", verts, "Number of random vertices; positive");
        cp.add_param_bytes("edges-per-vert", epv, "Edge per random vertex; positive");

        cp.add_flag('d', "edge-dependencies", edge_dependencies, "Dependencies between edges of same vertex");
        cp.add_flag('s', "filter-self-loops", filter_self_loops, "Remove all self-loops (w/o replacement)");
        cp.add_flag('m', "filter-multi-edges", filter_multi_edges, "Collapse parallel edges into a single one");
        cp.add_uint('p', "threads", number_of_threads, "Max. number of threads");

        cp.add_uint('x', "seed", seed, "Random seed; default [=0]: use STXXL default seed");

        if (!cp.process(argc, argv)) return -1;

        if (!verts || !epv) {
            cp.print_usage();
            return -1;
        }

        // apply config
        cp.print_result();
        number_of_vertices = verts;
        edges_per_vertex = epv;

        if (seed) stxxl::set_seed(seed);
    }

    stxxl::stats & stats = *stxxl::stats::get_instance();
    stxxl::stats_data stats_begin(stats);

    // compile-time config

   ppq_type mppq {
          TokenCompressed::ComparatorDesc(),
          pq_size, // total_ram
          1.5, // num_read_blocks_per_ea
          64, // num_write_buffer_blocks
          number_of_threads, // num_insertion_heaps
          1L << 24, // single_heap_ram
          pq_max_extract // extract_buffer_ram
    };

    RAGPath seed_graph(1000 * edges_per_vertex);

    // fill requests
    {
        stxxl::scoped_print_timer timer("Filling PPQ");
        mppq.bulk_push_begin(0);

        auto seed = stxxl::get_next_seed();

        omp_set_num_threads(number_of_threads);
        #pragma omp parallel
        {
            #pragma omp single
            {
                std::cout << "Generate random tokens with " << omp_get_num_threads() << " threads" << std::endl;
            }

            const auto pid = omp_get_thread_num();
            stxxl::random_number64 rand(seed + pid);

            const EdgeId seed_weight = 2*seed_graph.numberOfEdges();

            #pragma omp for
            for (uint64_t vertex = 0; vertex < number_of_vertices; vertex++) {
                uint64_t weight = seed_weight + 2 * edges_per_vertex * vertex;
                uint64_t idx = edges_per_vertex * vertex;

                for (uint64_t edge = 0; edge < edges_per_vertex; edge++, idx++) {
                    auto r = rand(weight);
                    if (UNLIKELY(r < seed_weight)) {
                        mppq.bulk_push(TokenCompressed{false, idx, seed_graph(r)});
                    } else if (r&1) {
                        mppq.bulk_push(TokenCompressed{false, idx, (r-seed_weight)/2/edges_per_vertex + seed_graph.maxVertexId() + 1});
                    } else {
                        mppq.bulk_push(TokenCompressed{true, (r-seed_weight) / 2, idx});
                    }

                    weight += 2 * edge_dependencies;
                }
            }
        }
        mppq.bulk_push_end();
    }
    std::cout << (stxxl::stats_data(stats) - stats_begin);

    // instantiate edge writer
    EdgeWriterPool<EdgeWriter> ewpool(number_of_threads);
    for(unsigned int i=0; i < number_of_threads; i++) {
        ewpool[i].setDisableOutput(true);
    }

    // push seed into result graph (do it sequentially)
    {
        RAGPath seed_graph(1000 * edges_per_vertex);
        for (EdgeId i = 0; i < 2*seed_graph.numberOfEdges(); i+=2) {
            ewpool[0](seed_graph(i), seed_graph(i+1));
        }
    }

    auto process_stats = stxxl::stats_data(stats);
    // process requests
    {
        stxxl::scoped_print_timer timer("Process Requests");

        // estimate batch size and reserve memory accordingly
        auto get_batch_size = [&] (const EdgeId processed) {
            return std::min<size_t>(pq_max_extract/sizeof(TokenCompressed) - 1, std::max<size_t>(std::pow(processed, 0.75), min_batch_size));
        };

        const size_t max_batch_size = get_batch_size(2 * number_of_vertices * edges_per_vertex);
        std::cout << "Max batch size: " << max_batch_size << std::endl;

        std::vector<TokenCompressed> pq_buffer, pq_buffer2;
        //pq_buffer.reserve( get_batch_size(max_batch_size) );
        //pq_buffer2.reserve( get_batch_size(max_batch_size) );

        size_t edges_processed = seed_graph.numberOfEdges();

        auto process = [&mppq, &edges_per_vertex, &seed_graph]
              (std::vector<TokenCompressed>& pq_buffer, buffer_iter it, buffer_iter end, uint32_t& unanswered, uint32_t& completed, EdgeWriter& writer) {
            // process tokens
            while(it != end) {
                // in the sequence does not start with an create token,
                // the query was not yet answered and we skip the sequence
                // and reprocess it in the next batch.
                if (UNLIKELY(it->is_query())) {
                    for(; it!=end && it->is_query(); ++it) {
                        mppq.bulk_push(*it);
                        unanswered++;
                    }
                    continue;
                }

                const Token t = it->decompress();

                // answer queries
                for(++it; it != end; ++it) {
                    const Token qt = it->decompress();

                    // found a new sequence
                    if (t.index != qt.index)
                        break;

                    // and finally answer the query
                    assert(qt.value > t.index);
                    mppq.bulk_push(TokenCompressed(false, qt.value, t.value));
                }

                // in case we are processing the very last token in the buffer,
                // we reinsert the create token without yielding an edge, since
                // there might be additional queries in the next frame
                if (UNLIKELY(it == end && pq_buffer.size() > 1)) {
                    mppq.bulk_push(TokenCompressed(false, t.index, t.value));
                } else {
                    const auto second = t.index/edges_per_vertex + seed_graph.maxVertexId() + 1;
                    writer(t.value, second);
                    completed++;
                }
            }
        };


        while(!mppq.empty()) {
            auto bs = std::max(min_batch_size, get_batch_size(ewpool.totalEdgesWritten()));

            double start_time = stxxl::timestamp();
            mppq.bulk_pop(pq_buffer, bs);

            if (pq_buffer.size() < 2*min_batch_size) {
                // single!
                mppq.bulk_push_begin(0);

                unsigned int unanswered = 0;
                unsigned int completed = 0;

                double pstart_time = stxxl::timestamp();
                process(pq_buffer, pq_buffer.begin(), pq_buffer.end(), unanswered, completed, ewpool[0]);
                double stop_time = stxxl::timestamp();
                mppq.bulk_push_end();
                double stop_push = stxxl::timestamp();
#ifdef PTFP_VERBOSE
                std::cout << "Requested: " << bs << "\tGot " << pq_buffer.size() << "\t"
                      "Unanswered: " << unanswered << "\tCompleted " << completed << ". "
                      "\tStill in PQ: " << mppq.size() << "\tEdges emitted: " << edges_processed <<
                      "\tPop: " << static_cast<unsigned int>(1e6 * (pstart_time - start_time)) << "us"
                      "\tProc: " << static_cast<unsigned int>(1e6 * (stop_time - pstart_time)) << "us"
                      "\tPush: " << static_cast<unsigned int>(1e6 * (stop_push - stop_time)) << "us"
                      "\tThreads: " << 1
                << std::endl;
#endif
                edges_processed += completed;

            } else {
/*                double start_time = stxxl::timestamp();

                mppq.bulk_pop(pq_buffer, bs);
                while (pq_buffer.size() < 3 * bs / 4 && !mppq.empty()) {
                    mppq.bulk_pop(pq_buffer2, bs - pq_buffer.size());
                    const auto prev_size = pq_buffer.size();
                    pq_buffer.resize(prev_size + pq_buffer2.size());
                    std::copy(pq_buffer2.begin(), pq_buffer2.end(), pq_buffer.begin() + prev_size);
                }
*/
                mppq.bulk_push_begin(0);

                unsigned int unanswered = 0;
                unsigned int completed = 0;

                omp_set_num_threads(std::max(1u,
                                             std::min<unsigned int>(pq_buffer.size() / min_batch_size,
                                                                    number_of_threads)
                ));

                double pstart_time = stxxl::timestamp();

                unsigned int nthreads;

                #pragma omp parallel reduction(+:unanswered, completed)
                {
                    const auto chunk_size = pq_buffer.size() / omp_get_num_threads();
                    assert(chunk_size >= min_batch_size || omp_get_num_threads() == 1);

                    #pragma omp master
                    nthreads = omp_get_num_threads();

                    const auto tid = omp_get_thread_num();
                    const auto is_first_thread = !tid;
                    const auto is_last_thread = (tid + 1 == omp_get_num_threads());

                    // find start position, i.e. the first token in the threads strip
                    // which starts a new edge list position
                    auto it = pq_buffer.begin() + chunk_size * omp_get_thread_num();
                    if (!is_first_thread) {
                        for (; it != pq_buffer.end() && it->is_query(); ++it);
                    }

                    // find end of our segment by extending it into the next thread's
                    // segment until a new edge list index starts.
                    buffer_iter end;
                    if (is_last_thread) {
                        end = pq_buffer.end();
                    } else {
                        end = pq_buffer.begin() + chunk_size * (tid + 1);
                        for (; end != pq_buffer.end() && end->is_query(); ++end);
                    }

                    process(pq_buffer, it, end, unanswered, completed, ewpool[tid]);
                }

                double stop_time = stxxl::timestamp();
                mppq.bulk_push_end();
                double stop_push = stxxl::timestamp();

                edges_processed += completed;

#ifdef PTFP_VERBOSE
                std::cout << "Requested: " << bs << "\tGot " << pq_buffer.size() << "\t"
                      "Unanswered: " << unanswered << "\tCompleted " << completed << ". "
                      "\tStill in PQ: " << mppq.size() << "\tEdges emitted: " << edges_processed <<
                      "\tPop: " << static_cast<unsigned int>(1e6 * (pstart_time - start_time)) << "us"
                      "\tProc: " << static_cast<unsigned int>(1e6 * (stop_time - pstart_time)) << "us"
                      "\tPush: " << static_cast<unsigned int>(1e6 * (stop_push - stop_time)) << "us"
                      "\tThreads: " << nthreads
                << std::endl;
#endif
            }

        }
    }

    std::cout << "Produced " << ewpool.totalEdgesWritten() << " edges" << std::endl;
    std::cout << (stxxl::stats_data(stats) - process_stats) << "\n\n"
              << (stxxl::stats_data(stats) - stats_begin);


    return 0;
}
