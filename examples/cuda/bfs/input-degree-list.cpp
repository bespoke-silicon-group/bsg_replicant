#include "WGraph.hpp"

using namespace std;
using namespace graph_tools;

std::string INPUT_NAME;
std::string OUTPUT_NAME;
WGraph::NodeID MIN_DEGREE;
WGraph::NodeID MAX_DEGREE;

int main(int argc, char *argv[])
{
    int root;

    if (argc < 5) {
        fprintf(stderr, "usage: %s INPUT_NAME ROOT ITERS OUTPUT_NAME\n", argv[0]);
        fprintf(stderr, "    ROOT    avg|max|[number]\n");
    }

    INPUT_NAME = argv[1];
    MIN_DEGREE = atoi(argv[2]);
    MAX_DEGREE = atoi(argv[3]);
    OUTPUT_NAME = argv[4];
    
    WGraph wg = WGraph::FromGraph500Data(Graph500Data::FromFile(INPUT_NAME));

    FILE *output = fopen(OUTPUT_NAME.c_str(), "w");
    fprintf(output, "vertex,degree\n");
    for (WGraph::NodeID v = 0; v < wg.num_nodes(); v++) {
        if (MIN_DEGREE <= wg.degree(v) && wg.degree(v) <= MAX_DEGREE) {
            fprintf(output, "%u,%u\n", v, wg.degree(v));
        }
    }
    fclose(output);
}
