//function to compute pr-nibble on host up to current iter
#pragma once
#include <iostream>
#include <fstream>

inline void host_pr_calc(std::vector<double> & p, std::vector<double> & old_rank, std::vector<double> & new_rank, std::vector<int> & frontier, int iter) {
    double alpha = (double) 0.15;
    double epsilon = (double) 1e-06;
    auto g = edges.getHostGraph();
    int * in_neigh = g.in_neighbors_shared_.get();
    int ** in_index = g.in_index_shared_.get();
		std::string fname = "iter-" + std::to_string(iter) + ".txt";
    ofstream ofile;
    ofile.open (fname);
    for(int i = 0; i < iter; i++) {
        //std::memcpy(new_rank, old_rank, sizeof(double)*edges.num_nodes());
	    //new_rank = old_rank;
        new_rank.assign(old_rank.begin(), old_rank.end());
        //print out iteration and size:
        int num_items = std::count(frontier.begin(), frontier.end(), 1);
        std::cerr << "on iteration: " << i << " with frontier size: " << num_items << std::endl;
        //update_self
        for(int v = 0; v < g.num_nodes(); v++) {
            p[v] += (2.0 * alpha) / (1.0  + alpha) * old_rank[v];
            new_rank[v] = (double) 0.0 ;
        }
        //update edges
        for(int d = 0; d < g.num_nodes(); d++) {
            for(int s : g.in_neigh(d)) {
                if(frontier[s]){
                    double update = ((1.0 - alpha) / (1.0  + alpha)) * old_rank[s];
										update = update / ((double) g.out_degree(s));
										new_rank[d] += update;
										if(i == (iter - 1)) {ofile << s << " " << d << " " << new_rank[d] << std::endl;}
                }
            }
        }
        //old_rank.swap(new_rank);
        //std::memcpy(old_rank, new_rank, sizeof(double)*edges.num_nodes());
        //old_rank = new_rank;
        old_rank.assign(new_rank.begin(), new_rank.end());
        //update frontier
        for(int v = 0; v < g.num_nodes(); v++) {
            frontier[v] = 0;
            if(g.out_degree(v) > 0 && old_rank[v] >= (((double) g.out_degree(v)) * epsilon)) {
                frontier[v] = 1;
            }
        }
    }
		ofile.close();
    int num_items = std::count(frontier.begin(), frontier.end(), 1);
    std::cerr << "returning with frontier size: " << num_items << std::endl;
}
