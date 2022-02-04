#include <string>
#include "WGraph.hpp"
#include "SparsePushBFS.hpp"
#include "SparsePullBFS.hpp"
using namespace std;
using namespace graph_tools;
std::string INPUT_NAME;
std::string GRAPH_PATH;
std::string GRAPH_TYPE;
std::string DIRECTION;
std::string OUTPUT_NAME;
std::string ROOT;
int ITERS;

int main(int argc, char *argv[])
{
    int root;

    //if (argc < 7) {
    //    fprintf(stderr, "usage: %s INPUT_NAME ROOT ITERS OUTPUT_NAME\n", argv[0]);
    //    fprintf(stderr, "    ROOT    avg|max|[number]\n");
    //}

    INPUT_NAME = argv[1];
    ROOT = argv[2];
    ITERS = atoi(argv[3]);
    OUTPUT_NAME = argv[4];
    DIRECTION = argv[5];
    GRAPH_TYPE = argv[6];
    GRAPH_PATH = argv[7];

    printf("Reading in %s\n", INPUT_NAME.c_str());
    WGraph wg,wg_csc;

    if(GRAPH_TYPE == "GRAPH500"){
        wg = WGraph::FromGraph500Data(Graph500Data::FromFile(INPUT_NAME));
        wg_csc = WGraph::FromGraph500Data(Graph500Data::FromFile(INPUT_NAME),true);
    }
    else{
        wg = WGraph::FromCSR(INPUT_NAME.c_str(),GRAPH_PATH.c_str());
    }
    if (ROOT == "avg") {
        root = static_cast<int>(wg.node_with_avg_degree());
        printf("root with average degree is: %d\n", root);
    } else if (ROOT == "max") {
        root = static_cast<int>(wg.node_with_max_degree());
        printf("root with max degree is: %d\n", root);
    } else {
        root = atoi(ROOT.c_str());
    }
    printf("Selected %d as the root\n", root);
    
    

    // write the output
    FILE *output = fopen(OUTPUT_NAME.c_str(), "w");
    if (output == NULL) {
        fprintf(stderr, "failed to open output file '%s': %m\n", OUTPUT_NAME.c_str());
        return 1;
    }

    // header
    #define FIELD_STR_FMT "s"
    #define FIELD_INT_FMT "d"

    fprintf(output,
            "%"  FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            ",%" FIELD_STR_FMT
            "\n",
            "input_name",
            "root",
            "num_nodes",
            "num_edges",
            "iteration",
            "frontier_in_size",
            "frontier_out_size",
            "visited_in_size",
            "visited_out_size",
            "traversed_edges",
            "updates"
        );

    if(DIRECTION == "PUSH"){
        std::shared_ptr<WGraph> wgptr = std::shared_ptr<WGraph>(new WGraph(wg));
        std::set<int> frontier = {root};
        std::set<int> visited = {root};
        for(int i=0; i<ITERS; i++){
            SparsePushBFS bfs = SparsePushBFS(wgptr, frontier, visited); 
            bfs.run(1000);
            fprintf(output,
                    "%"  FIELD_STR_FMT
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT                
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT
                    ",%" FIELD_INT_FMT
                    "\n",
                    INPUT_NAME.c_str(),
                    root,
                    static_cast<int>(wg.num_nodes()),
                    static_cast<int>(wg.num_edges()),
                    i,
                    bfs.frontier_in().size(),
                    bfs.frontier_out().size(),
                    bfs.visited_in().size(),
                    bfs.visited_out().size(),
                    bfs.traversed_edges(),
                    bfs.updates()); 
            frontier = bfs.frontier_out();
            visited = bfs.visited_out();
        }
        
    }
    
    

    fclose(output);
    
    return 0;
}
