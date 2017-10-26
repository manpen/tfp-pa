 External Memory Graph Generators using Preferential Attachment 
==================

This repository contains preferential attachment-based generators for graphs
that do not fit into main memory. The underlying idea is introduced in
["Generating Massive Scale-Free Networks under Resource Constraints", U.Meyer
and M.Penschuck, ALENEX16](https://doi.org/10.1137/1.9781611974317.4). This implementation, however, uses vertex-based tokens
rather edge based, which better fits the POD-constraints imposed by the STXXL.

Resolving Dependencies
----------------------
The generators depend on [STXXL](http://stxxl.sourceforge.net) and 
[Google Test](https://github.com/google/googletest). The libraries are added as
git submodules and hence need to be initialized before the first build.

    git submodule init
    git submodule update
    
Please note that the ./compile.sh script includes this steps.
Also, it is crucial that you checkout this repository rathern than downloading it as a Zip-Snapshot.

Edge-List-Format
----------------
Each generator produces an edge vector which fully represents the graph:
An output file contains a sequence of std::pair<NodeT, NodeT> where NodeT the
unsigned integer used to represent a node id: By default it is a 64-bit unsigned integer,
however smaller data types (32, 40, 48) are supported. Just edit the "-DFILE_DATA_WIDTH=" parameter
in CMakeLists.txt and recompile; the 40-bit / 48-bit types correspond to the STXXL::uintX
types.

An output file does not have a header but only contains the binary vector ids. Two
consecutive entries for an edge (from, to). In an undirected graph only one edge
with (from < to) is stored.

Barabasi-Albert-Generator
-------------------------
Usage: ./tfp_ba [options] <filename> <no-vertices> <edges-per-vert>

The Barabasi-Albert model yields undirected scale-free graphs. Given a small seed graph
(in our case a ring with 2*<edges-per-vert> vertices) an edge is introduce and connected
to <edges-per-vert> existing vertices. This procedure is repeated until <no-vertices>
vertices are added. We allow multi-edges during this process (use -m to later remove them;
this will however affect the average degree in the graph).

During the sampling of the <edges-per-vert> neighbors for vertex there are two modes of
operation: By default, all neighbors are sampled "in parallel" with the same probability
distribution. Use the -d option, to sample the neighbors "sequentially", i.e. to take
the shifted probability distribution of earlier edges into account. In this mode, self-loops
are possible, which can be later removed using the -s option.

"Directed scale-free graphs" by B Bollobas, C. Borgs, J. Chayes, and O. Riordan, SODA03
----------------------------------------------------------
Usage: ./tfp_bbcr [options] <filename> <no-edges>

This model again starts with a small seed ring of <seed-vertices, default 100> vertices.
Then parameters alpha + beta + gamma = 1 are used to select one of the following methods
to introduce a new edge:
- With prob. alpha add a new vertex and connect with an out-going edge; 
  the neighbor is sampled according to its in-degree
- With prob. beta two existing vertices u, v are connected with edge (u, v) where
  u is sampled based on its out-degree and v based on its in-degree
- With prob. gamma add a new vertex and connect with an in-coming edge;
  the neighbor is sampled according to its out-degree

The probability to select a vertex u based on its out-degree is proportional to
   Pout[u] \propto deg_out(u) + <d-out>
where <d-out, default 0.0> is a constant. The higher <d-out> the more uniform the sampling
takes place (i.e. the preferential attachment is damped). The mechanism for in-degree sampling
is analogous.

Computing the Degree Distribution
---------------------------------
This suite also include a distribution counter:

    # undirected graphs
    ./tfp_ba /local/graph.bin 1m 10
    ./distribution_count -o /local/distr /local/graph.bin
    gnuplot -e "datafile='/local/distr'" ../gnuplots/undirected_degree.gp

    # directed graphs
    ./tfp_bbcr /local/graph.bin 10m
    ./distribution_count --directed -o /local/distr /local/graph.bin
    gnuplot -e "datafile='/local/distr'" ../gnuplots/directed_degree.gp




