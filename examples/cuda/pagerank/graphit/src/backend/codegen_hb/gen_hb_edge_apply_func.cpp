#include <graphit/backend/codegen_hb/gen_hb_edge_apply_func.h>

namespace graphit {
    using namespace std;

    void HBEdgesetApplyFunctionGenerator::visit(mir::PushEdgeSetApplyExpr::Ptr push_apply) {
        genGlobalDecls(push_apply);
        genEdgeApplyFunctionDeclaration(push_apply);
    }

    void HBEdgesetApplyFunctionGenerator::visit(mir::PullEdgeSetApplyExpr::Ptr pull_apply) {
        genGlobalDecls(pull_apply);
        genEdgeApplyFunctionDeclaration(pull_apply);
    }

    void HBEdgesetApplyFunctionGenerator::genEdgeApplyFunctionDeclaration(mir::EdgeSetApplyExpr::Ptr apply) {
        auto func_name = genFunctionName(apply);

        // these schedules are still supported by runtime libraries
        if (func_name == "edgeset_apply_push_parallel_sliding_queue_from_vertexset_with_frontier"
            || func_name == "edgeset_apply_push_parallel_sliding_queue_weighted_deduplicated_from_vertexset_with_frontier"){
            return;
        }

        genEdgeApplyFunctionSignature(apply);
        *oss_ << "{ " << endl; //the end of the function declaration
        //TODO(Emily): this func call is what we need to change to fit our execution model
        genEdgeApplyFunctionDeclBody(apply);
        *oss_ << "} //end of edgeset apply function " << endl; //the end of the function declaration
        *oss_ << endl; //NOTE(Emily): want separation of functions for readability

    }

    void HBEdgesetApplyFunctionGenerator::genGlobalDecls(mir::EdgeSetApplyExpr::Ptr apply) {
      //TODO(Emily):
      //we might want to declare the current frontier here too?
      auto apply_func = mir_context_->getFunction(apply->input_function_name);
      // if(apply_func->result.isInitialized()) {
      //   *oss_ << "__attribute__((section(\".dram\"))) int  * __restrict next_frontier;" << endl;
      // }
    }

    void HBEdgesetApplyFunctionGenerator::genEdgeApplyFunctionDeclBody(mir::EdgeSetApplyExpr::Ptr apply) {
        if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
          genEdgePullApplyFunctionDeclBody(apply);
        }

        if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
          genEdgePushApplyFunctionDeclBody(apply);
        }
        //TODO(Emily): implement other directions
        /*
        if (mir::isa<mir::HybridDenseEdgeSetApplyExpr>(apply)) {
            genEdgeHybridDenseApplyFunctionDeclBody(apply);
        }

        if (mir::isa<mir::HybridDenseForwardEdgeSetApplyExpr>(apply)) {
            genEdgeHybridDenseForwardApplyFunctionDeclBody(apply);
        }
         */
    }

    void HBEdgesetApplyFunctionGenerator::setupFlags(mir::EdgeSetApplyExpr::Ptr apply,
                                                       bool & apply_expr_gen_frontier,
                                                       bool &from_vertexset_specified,
                                                       std::string &dst_type) {

        // set up the flag for checking if a from_vertexset has been specified
        if (apply->from_func != "")
            if (!mir_context_->isFunction(apply->from_func))
                from_vertexset_specified = true;

        // Check if the apply function has a return value
        auto apply_func = mir_context_->getFunction(apply->input_function_name);
        dst_type = apply->is_weighted ? "d.v" : "d";

        if (apply_func->result.isInitialized()) {
            // build an empty vertex subset if apply function returns
            apply_expr_gen_frontier = true;
        }
    }

    // Set up the global variables numVertices, numEdges, outdegrees
    void HBEdgesetApplyFunctionGenerator::setupGlobalVariables(mir::EdgeSetApplyExpr::Ptr apply,
                                                                 bool apply_expr_gen_frontier,
                                                                 bool from_vertexset_specified) {
        *oss_ << "    int64_t numVertices = g.num_nodes(), numEdges = g.num_edges();\n";


        if (!mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {

            if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)){
                //we still need to convert the from_vertexset to sparse, and compute m for SparsePush
                // even when it does not return a frontier
                if (from_vertexset_specified) {
                    //TODO(Emily): we will need to implement our own toSparse() (or remove this)
                    *oss_ << "    from_vertexset->toSparse();" << std::endl;
                    *oss_ << "    long m = from_vertexset->size();\n";

                } else {
                    *oss_ << "    long m = numVertices; \n";
                }
            }
        }
    }


    //TODO(Emily): this is the main part of code to modify for our purposes
    // Print the code for traversing the edges in the push direction and return the new frontier
    // the apply_func_name is used for hybrid schedule, when a special push_apply_func is used
    // usually, the apply_func_name is fixed to "apply_func" (see the default argument)
    void HBEdgesetApplyFunctionGenerator::printPushEdgeTraversalReturnFrontier(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name) {


        //TODO(Emily): do we want to support this? if so it needs to be modified
        //set up logic fo enabling deduplication with CAS on flags (only if it returns a frontier)
        // if (apply->enable_deduplication && apply_expr_gen_frontier) {
        //     *oss_ << "    if (g.flags_ == nullptr){\n"
        //             "      g.flags_ = new int[numVertices]();\n"
        //             "      parallel_for(int i = 0; i < numVertices; i++) g.flags_[i]=0;\n"
        //             "    }\n";
        // }

        // If apply function has a return value, then we need to return a temporary vertexsubset
        //NOTE(Emily): we don't want to deal with this. so we will never generate this code
        if(false) {
        //if (apply_expr_gen_frontier) {
            // build an empty vertex subset if apply function returns
            //set up code for outputing frontier for push based edgeset apply operations
            *oss_ <<
                 "    VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);\n"
                         "    if (numVertices != from_vertexset->getVerticesRange()) {\n"
                         "        cout << \"edgeMap: Sizes Don't match\" << endl;\n"
                         "        abort();\n"
                         "    }\n"
                         "    if (outDegrees == 0) return next_frontier;\n"
                         "    uintT *offsets = degrees;\n"
                         "    long outEdgeCount = sequence::plusScan(offsets, degrees, m);\n"
                         "    uintE *outEdges = newA(uintE, outEdgeCount);\n";
        }


        indent();

        printIndent();

        *oss_ << "int start, end;" << std::endl;
        printIndent();
        if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::VERTEX_BASED) {
            *oss_ << "local_range(V, &start, &end);" << std::endl;
        }
        else {
            *oss_ << "edge_aware_local_range(V, E, &start, &end, out_indices);" << std::endl;
        }
        printIndent();

        std::string for_type = "for";
        //if (apply->is_parallel)
        //    for_type = "parallel_for";

        std::string node_id_type = "int";
        if (apply->is_weighted) node_id_type = "WNode";

        //TODO(Emily): will we always assume from vertexset is specified?
        //             - also do we want to use vertex_t type? or just assume we will always index w/ long type

        std::string outer_end = "V";
        std::string iter = "s";
        *oss_ << for_type << " ( int " << iter << " = start; " << iter << " < end; " << iter << "++) {" << std::endl;
        indent();

        if(from_vertexset_specified) {
          printIndent();
          if(apply->is_weighted) { *oss_ << "if(from_vertexset[s].vertex) {" << std::endl;}
          else { *oss_ << "if(from_vertexset[s]) {" << std::endl; }
          indent();
        }

        //NOTE(Emily): more vars that we most likely won't need
        //if (from_vertexset_specified){
        if(false){
            *oss_ << "    NodeID s = from_vertexset->dense_vertex_set_[i];\n"
                    "    int j = 0;\n";
            if (apply_expr_gen_frontier){
                *oss_ <<  "    uintT offset = offsets[i];\n";
            }
        }


        if (apply->from_func != "" && !from_vertexset_specified) {
            printIndent();
            *oss_ << "if (from_func(s)){ " << std::endl;
            indent();
        }

        printIndent();
        *oss_ << "int degree = out_indices[s + 1] - out_indices[s];" << std::endl;
        printIndent();
        *oss_ << node_id_type << " * neighbors = &out_neighbors[out_indices[s]];" << std::endl;
        printIndent();
        *oss_ << "for(int d = 0; d < degree; d++) { "<< std::endl;

        // print the checks on filtering on sources s
        if (apply->to_func != "") {
            indent();
            printIndent();

            *oss_ << "if";
            //TODO: move this logic in to MIR at some point
            if (mir_context_->isFunction(apply->to_func)) {
                //if the input expression is a function call
                if(apply->is_weighted) {
                  *oss_ << " (to_func( neighbors[d].vertex)";
                } else {
                  *oss_ << "(to_func(neighbors[d])";
                }

            } else {
                //the input expression is a vertex subset
                //NOTE(Emily): we currently don't support this
                *oss_ << " (to_vertexset->bool_map_[neighbors[d]] ";
            }
            *oss_ << ") { " << std::endl;
        }

        indent();
        printIndent();
        if (apply_expr_gen_frontier) {
            *oss_ << "if( ";
        }

        // generating the C++ code for the apply function call
        if (apply->is_weighted) {
            *oss_ << apply_func_name << " ( s , neighbors[d].vertex, neighbors[d].weight )";
        } else {
            *oss_ << apply_func_name << " ( s, neighbors[d] )";

        }

        if (!apply_expr_gen_frontier) {
            *oss_ << ";" << std::endl;

        } else {


            //need to return a frontier
            // if (apply->enable_deduplication && apply_expr_gen_frontier) {
            //     *oss_ << " && CAS(&(g.flags_[" << dst_type << "]), 0, 1) ";
            // }

            indent();
            //TODO(Emily): they're using this outEdges as a temp var to build the frontier, we don't want this
            //generate the code for adding destination to "next" frontier
            *oss_ << " ) { " << std::endl;
            printIndent();
            if(apply->is_weighted) {
              *oss_ << "next_frontier[neighbors[d].vertex] = 1;" << std::endl;
            } else {
              *oss_ << "next_frontier[neighbors[d]] = 1;" << std::endl;
            }
            dedent();
            printIndent();
            *oss_ << "}" << std::endl;


//            dedent();
//            printIndent();
//            *oss_ << "}" << std::endl;
        }



        // end of from filtering
        if (apply->to_func != "") {
            dedent();
            printIndent();
            *oss_ << "} //end of to func" << std::endl;


        }

        //increment the index for each source vertex

        // dedent();
        // printIndent();
        // *oss_ << "} //end of if statement to check if in current block" << std::endl;

        //end of for loop on the neighbors

        dedent();
        printIndent();
        *oss_ << "} //end of for loop on neighbors" << std::endl;

        dedent();
        printIndent();
        *oss_ << "}" << std::endl;



        if (apply->from_func != "" && !from_vertexset_specified) {
            dedent();
            printIndent();
            *oss_ << "} //end of from func " << std::endl;
        }

        if(from_vertexset_specified) {
          dedent();
          printIndent();
          *oss_ << "}" << std::endl;
        }


        // if(apply_expr_gen_frontier)
        // {
        //    dedent();
        //    printIndent();
        //    *oss_ << "}" << std::endl; //end of next frontier blocking
        // }
        // dedent();
        // printIndent();
        // *oss_ << "}" << std::endl; //end of current frontier blocking

        //TODO(Emily): this is generating new frontier to be returned
        //             we will want to modify this to be what we want before returning

        //return a new vertexset if no subset vertexset is returned
        //if (apply_expr_gen_frontier) {
        if(false) {
            *oss_ << "  uintE *nextIndices = newA(uintE, outEdgeCount);\n"
                    "  long nextM = sequence::filter(outEdges, nextIndices, outEdgeCount, nonMaxF());\n"
                    "  free(outEdges);\n"
                    "  free(degrees);\n"
                    "  next_frontier->num_vertices_ = nextM;\n"
                    "  next_frontier->dense_vertex_set_ = nextIndices;\n";

            //set up logic fo enabling deduplication with CAS on flags (only if it returns a frontier)
            if (apply->enable_deduplication && from_vertexset_specified) {
                //clear up the indices that are set
                    *oss_ << "  parallel_for(int i = 0; i < nextM; i++){\n"
                            "     g.flags_[nextIndices[i]] = 0;\n"
                            "  }\n";
            }
            *oss_ << "  return next_frontier;\n";
        }
        printIndent();
        *oss_ << "barrier.sync();" << std::endl;
        printIndent();
        *oss_ << "return 0;" << std::endl;
    }

    void HBEdgesetApplyFunctionGenerator::printPushBlockedEdgeTraversalReturnFrontier(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name) {
      std::string node_id_type = "int";
      if (apply->is_weighted) node_id_type = "WNode";
      indent();
      printIndent();
      *oss_ << "int BLOCK_SIZE = 32; //cache line size" << std::endl;
      printIndent();
      *oss_ << "vertexdata lcl_nodes[ BLOCK_SIZE ];" << std::endl;
      if(from_vertexset_specified) {
        printIndent();
        *oss_ << "int lcl_frontier [ BLOCK_SIZE ];" << std::endl;
      }
      printIndent();
      *oss_ << "int blk_src_n = V/BLOCK_SIZE + (V%BLOCK_SIZE == 0 ? 0 : 1);" << std::endl;
      printIndent();
      *oss_ << "for (int blk_src_i = bsg_id; blk_src_i < blk_src_n; blk_src_i += bsg_tiles_X*bsg_tiles_Y) {" << std::endl;
      indent();
      printIndent();
      *oss_ << "int block_off = blk_src_i * BLOCK_SIZE;" << std::endl;
      if(from_vertexset_specified) {
        printIndent();
        *oss_ << "memcpy(&lcl_frontier[0], &frontier[block_off], BLOCK_SIZE * sizeof(lcl_frontier[0]));" << std::endl;
      }
      printIndent();
      *oss_ << "memcpy(&lcl_nodes[0], &out_vertices[block_off], BLOCK_SIZE * sizeof(lcl_nodes[0]));" << std::endl;
      printIndent();
      *oss_ << "for(int s = 0; s < BLOCK_SIZE; s++) {" << std::endl;
      indent();
      if(from_vertexset_specified) {
        printIndent();
        *oss_ << "if(lcl_frontier[s] == 0) continue;" << std::endl;
      }
      printIndent();
      *oss_ << "const " << node_id_type << " * neighbors = &out_neighbors[lcl_nodes[s].offset];" << std::endl;
      printIndent();
      *oss_ << "int dst_n = lcl_nodes[s].degree;" << std::endl;
      printIndent();
      *oss_ << "for( int d = 0; d < dst_n; d++) {" << std::endl;
      indent();
      printIndent();
      *oss_ << node_id_type << " dst = neighbors[d];" << std::endl;

      if (apply->to_func != "") {
          printIndent();
          *oss_ << "if";
          //TODO: move this logic in to MIR at some point
          if (mir_context_->isFunction(apply->to_func)) {
              //if the input expression is a function call
              if(apply->is_weighted) {
                *oss_ << " (to_func( dst.vertex)";
              } else {
                *oss_ << "(to_func(dst))";
              }

          } else {
              //the input expression is a vertex subset
              //NOTE(Emily): we currently don't support this
              *oss_ << " (to_vertexset[dst] ";
          }
          *oss_ << ") { " << std::endl;
          indent();
      }
      printIndent();
      if (apply_expr_gen_frontier) {
          *oss_ << "if( ";
      }

      // generating the C++ code for the apply function call
      if (apply->is_weighted) {
          *oss_ << apply_func_name << " ( (block_off + s) , dst.vertex, dst.weight )";
      } else {
          *oss_ << apply_func_name << " ( (block_off + s), dst )";
      }

      if (!apply_expr_gen_frontier) {
          *oss_ << ";" << std::endl;

      } else {
        indent();
        *oss_ << ") {" << std::endl;
        printIndent();
        if(apply->is_weighted) {
          *oss_ << "next_frontier[dst.vertex] = 1;" << std::endl;
        } else {
          *oss_ << "next_frontier[dst] = 1;" << std::endl;
        }
        dedent();
        printIndent();
        *oss_ << "}" << std::endl;
      }
      // end of from filtering
      if (apply->to_func != "") {
          dedent();
          printIndent();
          *oss_ << "} //end of to func" << std::endl;


      }
      dedent();
      printIndent();
      *oss_ << "} //end of for loop on neighbors" << std::endl;

      dedent();
      printIndent();
      *oss_ << "} //end of for loop on source nodes" << std::endl;

      dedent();
      printIndent();
      *oss_ << "} //end of loop on blocks" << std::endl;

      printIndent();
      *oss_ << "barrier.sync();" << std::endl;
      printIndent();
      *oss_ << "return 0;" << std::endl;

    }

    void HBEdgesetApplyFunctionGenerator::printPushAlignedEdgeTraversalReturnFrontier(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name) {
      std::string node_id_type = "int";
      if (apply->is_weighted) node_id_type = "WNode";
      indent();
      printIndent();
      *oss_ << "int BLOCK_SIZE = 32; //cache line size" << std::endl;
      printIndent();
      *oss_ << "int n_blocks = V/BLOCK_SIZE + (V%BLOCK_SIZE == 0 ? 0 : 1);" << std::endl;
      printIndent();
      *oss_ << "for (int i = bsg_id; i < n_blocks; i += bsg_tiles_X*bsg_tiles_Y) {" << std::endl;
      indent();
      printIndent();
      *oss_ << "int start = i * BLOCK_SIZE;" << std::endl;
      printIndent();
      *oss_ << "int end = start + BLOCK_SIZE;" << std::endl;

      printIndent();
      *oss_ << "for(int s = start; s < end; s++) {" << std::endl;
      indent();
      printIndent();
      *oss_ << "if(s < V) {" << std::endl;
      indent();
      if(from_vertexset_specified) {
        printIndent();
        *oss_ << "if(frontier[s] == 0) continue;" << std::endl;
      }
      printIndent();
      *oss_ << "const " << node_id_type << " * neighbors = &out_neighbors[out_indices[s]];" << std::endl;
      printIndent();
      *oss_ << "int degree = out_indices[s+1] - out_indices[s];" << std::endl;
      printIndent();
      *oss_ << "for( int d = 0; d < degree; d++) {" << std::endl;
      indent();
      printIndent();
      *oss_ << node_id_type << " dst = neighbors[d];" << std::endl;

      if (apply->to_func != "") {
          printIndent();
          *oss_ << "if";
          //TODO: move this logic in to MIR at some point
          if (mir_context_->isFunction(apply->to_func)) {
              //if the input expression is a function call
              if(apply->is_weighted) {
                *oss_ << " (to_func( dst.vertex)";
              } else {
                *oss_ << "(to_func(dst))";
              }

          } else {
              //the input expression is a vertex subset
              //NOTE(Emily): we currently don't support this
              *oss_ << " (to_vertexset[dst] ";
          }
          *oss_ << ") { " << std::endl;
          indent();
      }
      printIndent();
      if (apply_expr_gen_frontier) {
          *oss_ << "if( ";
      }

      // generating the C++ code for the apply function call
      if (apply->is_weighted) {
          *oss_ << apply_func_name << " ( s , dst.vertex, dst.weight )";
      } else {
          *oss_ << apply_func_name << " ( s, dst )";
      }

      if (!apply_expr_gen_frontier) {
          *oss_ << ";" << std::endl;

      } else {
        indent();
        *oss_ << ") {" << std::endl;
        printIndent();
        if(apply->is_weighted) {
          *oss_ << "next_frontier[dst.vertex] = 1;" << std::endl;
        } else {
          *oss_ << "next_frontier[dst] = 1;" << std::endl;
        }
        dedent();
        printIndent();
        *oss_ << "}" << std::endl;
      }
      // end of from filtering
      if (apply->to_func != "") {
          dedent();
          printIndent();
          *oss_ << "} //end of to func" << std::endl;


      }
      dedent();
      printIndent();
      *oss_ << "} //end of for loop on neighbors" << std::endl;

      dedent();
      printIndent();
      *oss_ << "} //end of for loop on source nodes" << std::endl;

      dedent();
      printIndent();
      *oss_ << "} //end of loop on blocks" << std::endl;

      printIndent();
      *oss_ << "barrier.sync();" << std::endl;
      printIndent();
      *oss_ << "return 0;" << std::endl;

    }

    void HBEdgesetApplyFunctionGenerator::printPullEdgeTraversalInnerNeighborLoop(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name,
            bool cache_aware,
            bool numa_aware) {

        if (apply->to_func != "") {
            printIndent();
            *oss_ << "if (to_func(d)){" << std::endl;
            indent();
        }

        std::string node_id_type = "int";
        if (apply->is_weighted) node_id_type = "WNode";

        printIndent();
        *oss_ << "int degree = in_indices[d + 1] - in_indices[d];" << std::endl;
        printIndent();
        *oss_ << node_id_type << " * neighbors = &in_neighbors[in_indices[d]];" << std::endl;
        printIndent();
        *oss_ << "for(int s = 0; s < degree; s++) { "<< std::endl;

        if(apply->from_func != "") {
          indent();
          printIndent();

          *oss_ << "if";

          std::string src_type = "neighbors[s]";

          if(from_vertexset_specified) {
            if(mir_context_->isFunction(apply->from_func)) {
              if(apply->is_weighted) {*oss_ << " (from_func(" << src_type << ") && from_vertexset[" << src_type << "].vertex";}
              else {*oss_ << " (from_func(" << src_type << ") && from_vertexset[" << src_type << "]";}
            }
            else {
              if(apply->is_weighted) { *oss_ << "(from_vertexset[" << src_type << "].vertex";}
              else {*oss_ << "(from_vertexset[" << src_type << "]";}
            }
          }
          else {
            if(mir_context_->isFunction(apply->from_func)) {
              *oss_ << " (from_func(" << src_type << ")";
            }
            else {
              *oss_ << "(true";
            }
          }
          *oss_ << ") {" << std::endl;
        }

        indent();
        printIndent();
        if(apply_expr_gen_frontier) {
          *oss_ << "if( ";
        }

        if (apply->is_weighted) {
            *oss_ << apply_func_name << " ( neighbors[s].vertex, d, neighbors[s].weight )";
        } else {
            *oss_ << apply_func_name << " (neighbors[s] , d)";

        }

        if(!apply_expr_gen_frontier) {
          *oss_ << ";" << std::endl;
        }
        else {
          indent();
          *oss_ << ") { " << std::endl;
          printIndent();
          *oss_ << "next_frontier[d] = 1; " << std::endl;

          // if(apply->to_func != "") {
          //   printIndent();
          //   *oss_ << "if(!to_func(d)) break; " << std::endl;
          // }
          dedent();
          printIndent();
          *oss_ << "}" << std::endl;
        }

        if(apply->from_func != "") {
          dedent();
          printIndent();
          *oss_ << "}" <<std::endl;
        }

        dedent();
        printIndent();
        *oss_ << "} //end of loop on in neighbors" << std::endl;

        if(apply->to_func != "") {
          dedent();
          printIndent();
          *oss_ << "} //end of to filtering" <<std::endl;
        }

    }

    void HBEdgesetApplyFunctionGenerator::printPullBlockedEdgeTraversalInnerNeighborLoop(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name,
            bool cache_aware,
            bool numa_aware) {
        printIndent();
        *oss_ << "if ( lcl_visited[d] == 1)" << std::endl;
        indent();
        printIndent();
        *oss_ << "continue; //skip visited nodes" << std::endl;
        dedent();
        if (apply->to_func != "") {
            printIndent();
            *oss_ << "if (to_func(d)){ " << std::endl;
            indent();
        }
        std::string node_id_type = "int";
        if (apply->is_weighted) node_id_type = "WNode";

        printIndent();
        *oss_ << "int degree = lcl_nodes[d].degree;" << std::endl;
        printIndent();
        *oss_ << node_id_type << " * neighbors = &in_neighbors[lcl_nodes[d].offset];" << std::endl;
        printIndent();
        *oss_ << "for(int s = 0; s < degree; s++) { "<< std::endl;
        if(apply->from_func != "") {
          indent();
          printIndent();

          *oss_ << "if";
          std::string src_type = "neighbors[s]";
          if(from_vertexset_specified) {
            if(mir_context_->isFunction(apply->from_func)) {
              if(apply->is_weighted) {*oss_ << " (from_func(" << src_type << ") && from_vertexset[" << src_type << "].vertex";}
              else {*oss_ << " (from_func(" << src_type << ") && from_vertexset[" << src_type << "]";}
            }
            else {
              if(apply->is_weighted) { *oss_ << "(from_vertexset[" << src_type << "].vertex";}
              else {*oss_ << "(from_vertexset[" << src_type << "]";}
            }
          }
          else {
            if(mir_context_->isFunction(apply->from_func)) {
              *oss_ << " (from_func(" << src_type << ")";
            }
            else {
              *oss_ << "(true";
            }
          }


          *oss_ << ") {" << std::endl;
        }

        indent();
        printIndent();
        if(apply_expr_gen_frontier) {
          *oss_ << "if( ";
        }

        if (apply->is_weighted) {
            *oss_ << apply_func_name << " ( d , neighbors[s].vertex, neighbors[s].weight )";
        } else {
            *oss_ << apply_func_name << " ( d, neighbors[s] )";

        }

        if(!apply_expr_gen_frontier) {
          *oss_ << ";" << std::endl;
        }
        else {
          indent();
          *oss_ << ") { " << std::endl;
          printIndent();
          *oss_ << "lcl_next_frontier[d] = 1; " << std::endl;
          printIndent();
          *oss_ << "lcl_visited[d] = 1; " << std::endl;
          dedent();
          printIndent();
          *oss_ << "}" << std::endl;
        }

        if(apply->from_func != "") {
          dedent();
          printIndent();
          *oss_ << "}" <<std::endl;
        }

        dedent();
        printIndent();
        *oss_ << "} //end of loop on in neighbors" << std::endl;

        if(apply->to_func != "") {
          dedent();
          printIndent();
          *oss_ << "} //end of to filtering" << std::endl;
        }


    }


    // Iterate through per-socket local buffers and merge the result into the global buffer
    //NOTE(Emily): we will probably want to leverage code like this for our own memory system
    //For now we shouldn't ever be using this
    void HBEdgesetApplyFunctionGenerator::printNumaMerge(mir::EdgeSetApplyExpr::Ptr apply) {
        *oss_ << "}// end of per-socket parallel region\n\n";
        auto edgeset_name = mir::to<mir::VarExpr>(apply->target)->var.getName();
        auto merge_reduce = mir_context_->edgeset_to_label_to_merge_reduce[edgeset_name][apply->scope_label_name];
        *oss_ << "  parallel_for (int n = 0; n < numVertices; n++) {\n";
        *oss_ << "    for (int socketId = 0; socketId < omp_get_num_places(); socketId++) {\n";
        *oss_ << "      " << apply->merge_reduce->field_name << "[n] ";
        switch (apply->merge_reduce->reduce_op) {
            case mir::ReduceStmt::ReductionOp::SUM:
                *oss_ << "+= local_" << apply->merge_reduce->field_name  << "[socketId][n];\n";
                break;
            case mir::ReduceStmt::ReductionOp::MIN:
                *oss_ << "= min(" << apply->merge_reduce->field_name << "[n], local_"
                << apply->merge_reduce->field_name  << "[socketId][n]);\n";
                break;
            default:
                // TODO: fill in the missing operators when they are actually used
                abort();
        }
        *oss_ << "    }\n  }" << std::endl;
    }

    //NOTE(Emily): also probably useful for distributing data on our machine
    void HBEdgesetApplyFunctionGenerator::printNumaScatter(mir::EdgeSetApplyExpr::Ptr apply) {
        *oss_ << "parallel_for (int n = 0; n < numVertices; n++) {\n";
        *oss_ << "    for (int socketId = 0; socketId < omp_get_num_places(); socketId++) {\n";
        *oss_ << "      local_" << apply->merge_reduce->field_name  << "[socketId][n] = "
        << apply->merge_reduce->field_name << "[n];\n";
        *oss_ << "    }\n  }\n";
    }

    void HBEdgesetApplyFunctionGenerator::printPullEdgeTraversalReturnFrontier(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name) {
        // If apply function has a return value, then we need to return a temporary vertexsubset
        if (apply_expr_gen_frontier) {
            // build an empty vertex subset if apply function returns
            apply_expr_gen_frontier = true;

            //        "  long numVertices = g.num_nodes(), numEdges = g.num_edges();\n"
            //        "  long m = from_vertexset->size();\n"

            // *oss_ << "  VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);\n"
            //         "  bool * next = newA(bool, g.num_nodes());\n"
            //         "  parallel_for (int i = 0; i < numVertices; i++)next[i] = 0;\n";
        }
        indent();
        printIndent();
        *oss_ << "int start, end;" << std::endl;
        printIndent();
        if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::VERTEX_BASED) {
            *oss_ << "local_range(V, &start, &end);" << std::endl;
        }
        else {
            *oss_ << "edge_aware_local_range(V, E, &start, &end, in_indices);" << std::endl;
        }




        //TODO(Emily): implement a vertexset device runtime library
        //              to allow dense/sparse transformations

        // if (apply->from_func != "") {
        //     if (!mir_context_->isFunction(apply->from_func)) {
        //         printIndent();
        //         *oss_ << "from_vertexset->toDense();" << std::endl;
        //     }
        // }

        //generate a bitvector from the dense vertexset (bool map)
        // if (from_vertexset_specified && apply->use_pull_frontier_bitvector){
        //     *oss_ << "  Bitmap bitmap(numVertices);\n"
        //             "  bitmap.reset();\n"
        //             "  parallel_for(int i = 0; i < numVertices; i+=64){\n"
        //             "     int start = i;\n"
        //             "     int end = (((i + 64) < numVertices)? (i+64):numVertices);\n"
        //             "     for(int j = start; j < end; j++){\n"
        //             "        if (from_vertexset->bool_map_[j])\n"
        //             "          bitmap.set_bit(j);\n"
        //             "     }\n"
        //             "  }" << std::endl;
        // }

        printIndent();

        // Setup flag for cache_awareness: use cache optimization if the data modified by this apply is segemented
         bool cache_aware = false;
        // auto segment_map = mir_context_->edgeset_to_label_to_num_segment;
        // for (auto edge_iter = segment_map.begin(); edge_iter != segment_map.end(); edge_iter++) {
        //     for (auto label_iter = (*edge_iter).second.begin();
        //     label_iter != (*edge_iter).second.end();
        //     label_iter++) {
        //         if ((*label_iter).first == apply->scope_label_name)
        //             cache_aware = true;
        //     }
        // }

        // Setup flag for numa_awareness: use numa optimization if the numa flag is set in the merge_reduce data structure
         bool numa_aware = false;
        // for (auto iter : mir_context_->edgeset_to_label_to_merge_reduce) {
        //     for (auto inner_iter : iter.second) {
        //         if (mir::to<mir::VarExpr>(apply->target)->var.getName() == iter.first
        //             && inner_iter.second->numa_aware)
        //             numa_aware = true;
        //     }
        // }
        //
        // if (numa_aware) {
        //     printNumaScatter(apply);
        // }

        std::string outer_end = "V";
        std::string iter = "d";

    //     if (numa_aware || cache_aware) {
    //         if (numa_aware) {
    //             std::string num_segment_str = "g.getNumSegments(\"" + apply->scope_label_name + "\");";
    //             *oss_ << "  int numPlaces = omp_get_num_places();\n";
    //             *oss_ << "    int numSegments = g.getNumSegments(\"" + apply->scope_label_name + "\");\n";
		// *oss_ << "    int segmentsPerSocket = (numSegments + numPlaces - 1) / numPlaces;\n";
    //             *oss_ << "#pragma omp parallel num_threads(numPlaces) proc_bind(spread)\n{\n";
    //             *oss_ << "    int socketId = omp_get_place_num();\n";
    //             *oss_ << "    for (int i = 0; i < segmentsPerSocket; i++) {\n";
    //             *oss_ << "      int segmentId = socketId + i * numPlaces;\n";
    //             *oss_ << "      if (segmentId >= numSegments) break;\n";
    //         } else {
    //             *oss_ << "  for (int segmentId = 0; segmentId < g.getNumSegments(\"" << apply->scope_label_name
    //                  << "\"); segmentId++) {\n";
    //         }
    //         *oss_ << "      auto sg = g.getSegmentedGraph(std::string(\"" << apply->scope_label_name << "\"), segmentId);\n";
    //         outer_end = "sg->numVertices";
    //         iter = "localId";
    //     }

        //genearte the outer for loop
        if (! apply->use_pull_edge_based_load_balance) {
            std::string for_type = "for";
            *oss_ << for_type << " ( int " << iter << " = start; " << iter << " < end; " << iter << "++) {" << std::endl;
            indent();
            // if (cache_aware) {
            //     printIndent();
            //     *oss_ << "int d = sg->graphId[localId];" << std::endl;
            // }
        } else {
            // use edge based load balance
            // recursive load balance scheme
            //TODO(Emily): implement a more advanced load balancing

            //set up the edge index (in in edge array) for estimating number of edges
            // *oss_ << "  if (g.offsets_ == nullptr) g.SetUpOffsets(true);\n"
            //         "  SGOffset * edge_in_index = g.offsets_;\n";
            //
            // *oss_ << "    std::function<void(int,int,int)> recursive_lambda = \n"
            //         "    [" << (apply->to_func != "" ?  "&to_func, " : "")
            //      << "&apply_func, &g,  &recursive_lambda, edge_in_index" << (cache_aware ? ", sg" : "");
            // // capture bitmap and next frontier if needed
            // if (from_vertexset_specified) {
            //     if(apply->use_pull_frontier_bitvector) *oss_ << ", &bitmap ";
            //     else *oss_ << ", &from_vertexset";
            // }
            // if (apply_expr_gen_frontier) *oss_ << ", &next ";
            // *oss_ <<"  ]\n"
            //         "    (NodeID start, NodeID end, int grain_size){\n";
            // if (cache_aware)
            //     *oss_ << "         if ((start == end-1) || ((sg->vertexArray[end] - sg->vertexArray[start]) < grain_size)){\n"
            //             "  for (NodeID localId = start; localId < end; localId++){\n"
            //             "    NodeID d = sg->graphId[localId];\n";
            // else
            //     *oss_ << "         if ((start == end-1) || ((edge_in_index[end] - edge_in_index[start]) < grain_size)){\n"
            //             "  for (NodeID d = start; d < end; d++){\n";
            // indent();

        }

        //print the code for inner loop on in neighbors
        printPullEdgeTraversalInnerNeighborLoop(apply, from_vertexset_specified, apply_expr_gen_frontier,
            dst_type, apply_func_name, cache_aware, numa_aware);


        if (! apply->use_pull_edge_based_load_balance) {
            //end of outer for loop
            dedent();
            printIndent();
            *oss_ << "} //end of outer for loop" << std::endl;
        } else {
            // dedent();
            // printIndent();
            // *oss_ << " } //end of outer for loop" << std::endl;
            // *oss_ << "        } else { // end of if statement on grain size, recursive case next\n"
            //         "                 cilk_spawn recursive_lambda(start, start + ((end-start) >> 1), grain_size);\n"
            //         "                  recursive_lambda(start + ((end-start)>>1), end, grain_size);\n"
            //         "        } \n"
            //         "    }; //end of lambda function\n";
            // *oss_ << "    recursive_lambda(0, " << (cache_aware ? "sg->" : "") << "numVertices, "  <<  apply->pull_edge_based_load_balance_grain_size << ");\n"
            //         "    cilk_sync; \n";
        }

        printIndent();
        *oss_ << "barrier.sync();" << std::endl;
        printIndent();
        *oss_ << "return 0;" << std::endl;

        // if (numa_aware) {
        //   *oss_ << "} // end of per-socket parallel_for\n";
        // }
        // if (cache_aware) {
        //     *oss_ << "    } // end of segment for loop\n";
        // }

        // if (numa_aware) {
        //     printNumaMerge(apply);
        // }

        //return a new vertexset if no subset vertexset is returned
        // if (apply_expr_gen_frontier) {
        //     *oss_ << "  next_frontier->num_vertices_ = sequence::sum(next, numVertices);\n"
        //             "  next_frontier->bool_map_ = next;\n"
        //             "  next_frontier->is_dense = true;\n"
        //             "  return next_frontier;\n";
        // }

    }

    void HBEdgesetApplyFunctionGenerator::printPullBlockedEdgeTraversalReturnFrontier(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name) {

      if (apply_expr_gen_frontier) {
          apply_expr_gen_frontier = true;
      }
      bool cache_aware = false;
      bool numa_aware = false;
      std::string node_id_type = "int";
      if (apply->is_weighted) node_id_type = "WNode";
      indent();
      printIndent();
      *oss_ << "int BLOCK_SIZE = 32; //cache line size" << std::endl;
      printIndent();
      *oss_ << "vertexdata lcl_nodes [ BLOCK_SIZE ];" << std::endl;
      printIndent();
      *oss_ << "int lcl_next_frontier [ BLOCK_SIZE ];" << std::endl;
      printIndent();
      *oss_ << "int lcl_visited [ BLOCK_SIZE ];" << std::endl;
      printIndent();
      *oss_ << "int blk_dst_n = V/BLOCK_SIZE + (V%BLOCK_SIZE == 0 ? 0 : 1);" << std::endl;
      printIndent();
      *oss_ << "for (int blk_dst_i = bsg_id; blk_dst_i < blk_dst_n; blk_dst_i += bsg_tiles_X * bsg_tiles_Y) {" << std::endl;
      indent();
      printIndent();
      *oss_ << "int blk_dst_base = blk_dst_i * BLOCK_SIZE;" << std::endl;
      printIndent();
      *oss_ << "memcpy(&lcl_visited[0], &visited[blk_dst_base],sizeof(lcl_visited));" << std::endl;
      printIndent();
      *oss_ << "memcpy(&lcl_nodes[0], &in_vertices[blk_dst_base], sizeof(lcl_nodes));" << std::endl;
      printIndent();
      *oss_ << "memset(&lcl_next_frontier[0], 0, sizeof(lcl_dense_o));" << std::endl;

      std::string outer_end = "BLOCK_SIZE";
      std::string iter = "d";
      printIndent();
      *oss_ << "for ( int " << iter << " = 0; " << iter << " < " << outer_end << "; " << iter << "++) {" << std::endl;
      indent();

      printPullBlockedEdgeTraversalInnerNeighborLoop(apply, from_vertexset_specified, apply_expr_gen_frontier,
          dst_type, apply_func_name, cache_aware, numa_aware);

      dedent();
      printIndent();
      *oss_ << "} //end of dst node for loop" << std::endl;



      printIndent();
      *oss_ << "memcpy(&next_frontier[blk_dst_base], &lcl_next_frontier[0],sizeof(lcl_dense_o));" << std::endl;
      printIndent();
      *oss_ << "memcpy(&visited[blk_dst_base], &lcl_visited[0], sizeof(lcl_visited_io));" << std::endl;
      //printIndent();
      //*oss_ << "barrier.sync();" << std::endl;
      dedent();//end of outer block loop
      printIndent();
      *oss_ << "} //end of outer blocked loop" << std::endl;

      printIndent();
      *oss_ << "barrier.sync();" << std::endl;

      printIndent();
      *oss_ << "return 0;" << std::endl;


    }

    void HBEdgesetApplyFunctionGenerator::printPullAlignedEdgeTraversalReturnFrontier(
            mir::EdgeSetApplyExpr::Ptr apply,
            bool from_vertexset_specified,
            bool apply_expr_gen_frontier,
            std::string dst_type,
            std::string apply_func_name) {

      if (apply_expr_gen_frontier) {
          apply_expr_gen_frontier = true;
      }
      bool cache_aware = false;
      bool numa_aware = false;
      std::string node_id_type = "int";
      if (apply->is_weighted) node_id_type = "WNode";
      indent();
      printIndent();
      *oss_ << "int BLOCK_SIZE = 32; //cache line size" << std::endl;
      printIndent();
      *oss_ << "vertexdata lcl_nodes [ BLOCK_SIZE ];" << std::endl;
      printIndent();
      *oss_ << "int blocks = V/BLOCK_SIZE + (V%BLOCK_SIZE == 0 ? 0 : 1);" << std::endl;
      printIndent();
      *oss_ << "for (int i = bsg_id; i < blocks; i += bsg_tiles_X * bsg_tiles_Y) {" << std::endl;
      indent();
      printIndent();
      *oss_ << "int start = i * BLOCK_SIZE;" << std::endl;
      printIndent();
      *oss_ << "int end = start + BLOCK_SIZE;" << std::endl;

      std::string outer_end = "end";
      std::string iter = "d";
      printIndent();
      *oss_ << "for ( int " << iter << " = start; " << iter << " < " << outer_end << "; " << iter << "++) {" << std::endl;
      indent();

      printPullEdgeTraversalInnerNeighborLoop(apply, from_vertexset_specified, apply_expr_gen_frontier,
          dst_type, apply_func_name, cache_aware, numa_aware);

      dedent();
      printIndent();
      *oss_ << "} //end of dst node for loop" << std::endl;

      dedent();//end of outer block loop
      printIndent();
      *oss_ << "} //end of outer blocked loop" << std::endl;

      printIndent();
      *oss_ << "barrier.sync();" << std::endl;

      printIndent();
      *oss_ << "return 0;" << std::endl;


    }

    // Generate the code for pushed based program
    void HBEdgesetApplyFunctionGenerator::genEdgePushApplyFunctionDeclBody(mir::EdgeSetApplyExpr::Ptr apply) {
        bool apply_expr_gen_frontier = false;
        bool from_vertexset_specified = false;
        string dst_type;
        setupFlags(apply, apply_expr_gen_frontier, from_vertexset_specified, dst_type);
        //TODO(Emily): need to enable these options again with the new schedule
        if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::BLOCKED) {
          printPushBlockedEdgeTraversalReturnFrontier(apply, from_vertexset_specified, apply_expr_gen_frontier, dst_type);
        }
        else if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::ALIGNED) {
          printPushAlignedEdgeTraversalReturnFrontier(apply, from_vertexset_specified, apply_expr_gen_frontier, dst_type);
        }
        else {
          printPushEdgeTraversalReturnFrontier(apply, from_vertexset_specified, apply_expr_gen_frontier, dst_type);
        }
    }

    void HBEdgesetApplyFunctionGenerator::genEdgePullApplyFunctionDeclBody(mir::EdgeSetApplyExpr::Ptr apply) {
        bool apply_expr_gen_frontier = false;
        bool from_vertexset_specified = false;
        string dst_type;
        setupFlags(apply, apply_expr_gen_frontier, from_vertexset_specified, dst_type);
        if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::BLOCKED) {
          printPullBlockedEdgeTraversalReturnFrontier(apply, from_vertexset_specified, apply_expr_gen_frontier, dst_type);
        }
        else if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::ALIGNED) {
          printPullAlignedEdgeTraversalReturnFrontier(apply, from_vertexset_specified, apply_expr_gen_frontier, dst_type);
        }
        else {
          printPullEdgeTraversalReturnFrontier(apply, from_vertexset_specified, apply_expr_gen_frontier, dst_type);
        }
    }

    //TODO(Emily): we want to get rid of the templating here - need to track the params passed to call in main
    void HBEdgesetApplyFunctionGenerator::genEdgeApplyFunctionSignature(mir::EdgeSetApplyExpr::Ptr apply) {
        auto func_name = genFunctionName(apply);
        auto apply_func = mir_context_->getFunction(apply->input_function_name);
        auto mir_var = std::dynamic_pointer_cast<mir::VarExpr>(apply->target);
        vector<string> templates = vector<string>();
        vector<string> arguments = vector<string>();

        //TODO(Emily): are we going to use their Graph class or will we need our own
        if (apply->is_weighted) {
          if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::BLOCKED) {
            if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
              arguments.push_back("vertexdata *out_indices");
              arguments.push_back("WNode *out_neighbors");
            } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
              arguments.push_back("vertexdata *in_indices");
              arguments.push_back("WNode *in_neighbors");
            }
          } else {
            if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
              arguments.push_back("int *out_indices");
              arguments.push_back("WNode *out_neighbors");
            } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
              arguments.push_back("int *in_indices");
              arguments.push_back("WNode *in_neighbors");
            }
          }
        } else {
            //arguments.push_back("Graph & g");
            if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::BLOCKED) {
              if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
                arguments.push_back("vertexdata *out_vertices");
                arguments.push_back("int *out_neighbors");
              } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
                arguments.push_back("vertexdata *in_vertices");
                arguments.push_back("int *in_neighbors");
              }
              else {
                //TODO(Emily): implement other directions
              }
            }
            else {
              if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
                arguments.push_back("int *out_indices");
                arguments.push_back("int *out_neighbors");
              } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
                arguments.push_back("int *in_indices");
                arguments.push_back("int *in_neighbors");
              }
              else {
                //TODO(Emily): implement other directions
              }
            }

        }

        if (apply->from_func != "") {
            if (mir_context_->isFunction(apply->from_func)) {
                // the schedule is an input from function
                templates.push_back("typename FROM_FUNC");
                arguments.push_back("FROM_FUNC from_func");
            } else {
                // the input is an input from vertexset
                arguments.push_back("int* from_vertexset");
            }
        }

        if (apply_func->result.isInitialized()) {
            // build an empty vertex subset if apply function returns
            arguments.push_back("int * next_frontier");
        }

        if (apply->to_func != "") {
            if (mir_context_->isFunction(apply->to_func)) {
                // the schedule is an input to function
                templates.push_back("typename TO_FUNC");
                arguments.push_back("TO_FUNC to_func");
            } else {
                // the input is an input to vertexset
                arguments.push_back("int* to_vertexset");
            }
        }


        templates.push_back("typename APPLY_FUNC");
        arguments.push_back("APPLY_FUNC apply_func");

        arguments.push_back("int V");
        arguments.push_back("int E");
        arguments.push_back("int block_size_x");

        *oss_ << "template <";

        bool first = true;
        for (auto temp : templates) {
            if (first) {
                *oss_ << temp << " ";
                first = false;
            } else
                *oss_ << ", " << temp;
        }
        *oss_ << "> ";
        //NOTE(Emily): we are always initializing funcs as returning int for now
        //*oss_ << (mir_context_->getFunction(apply->input_function_name)->result.isInitialized() ?
        //         "VertexSubset<NodeID>* " : "void ")  << func_name << "(";
        *oss_ << "int " << func_name << "(";

        first = true;
        for (auto arg : arguments) {
            if (first) {
                *oss_ << arg << " ";
                first = false;
            } else
                *oss_ << ", " << arg;
        }

        *oss_ << ") " << endl;


    }

    //NOTE(Emily): we may want to somehow signify this is a parallel kernel in the name, but otherwise we don't need to change this
    //generates different function name for different schedules
    // important for cases where we split the kernel iterations and assign different schedules to different iters
    std::string HBEdgesetApplyFunctionGenerator::genFunctionName(mir::EdgeSetApplyExpr::Ptr apply) {
        // A total of 48 schedules for the edgeset apply operator for now
        // Direction first: "push", "pull" or "hybrid_dense"
        // Parallel: "parallel" or "serial"
        // Weighted: "" or "weighted"
        // Deduplicate: "deduplicated" or ""
        // From: "" (no from func specified) or "from_vertexset" or "from_filter_func"
        // To: "" or "to_vertexset" or "to_filter_func"
        // Frontier: "" (no frontier tracking) or "with_frontier"
        // Weighted: "" (unweighted) or "weighted"

        string output_name = "edgeset_apply";
        auto apply_func = mir_context_->getFunction(apply->input_function_name);

        //check direction
        if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
            output_name += "_push";
        } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
            output_name += "_pull";
        } else if (mir::isa<mir::HybridDenseForwardEdgeSetApplyExpr>(apply)) {
            output_name += "_hybrid_denseforward";
        } else if (mir::isa<mir::HybridDenseEdgeSetApplyExpr>(apply)) {
            output_name += "_hybrid_dense";
        }

        output_name += "_parallel";

        if (apply->use_sliding_queue) {
            output_name += "_sliding_queue";
        }

        //check if it is weighted
        if (apply->is_weighted) {
            output_name += "_weighted";
        }

        // check for deduplication
        if (apply->enable_deduplication && apply_func->result.isInitialized()) {
            output_name += "_deduplicated";
        }

        if (apply->from_func != "") {
            if (mir_context_->isFunction(apply->from_func)) {
                // the schedule is an input from function
                output_name += "_from_filter_func";
            } else {
                // the input is an input from vertexset
                output_name += "_from_vertexset";
            }
        }

        if (apply->to_func != "") {
            if (mir_context_->isFunction(apply->to_func)) {
                // the schedule is an input to function
                output_name += "_to_filter_func";
            } else {
                // the input is an input to vertexset
                output_name += "_to_vertexset";
            }
        }

        if (mir::isa<mir::HybridDenseEdgeSetApplyExpr>(apply)) {
            auto apply_expr = mir::to<mir::HybridDenseEdgeSetApplyExpr>(apply);
            if (apply_expr->push_to_function_ != "") {
                if (mir_context_->isFunction(apply->to_func)) {
                    // the schedule is an input to function
                    output_name += "_push_to_filter_func";
                } else {
                    // the input is an input to vertexset
                    output_name += "_push_to_vertexset";
                }
            }
        }


        if (apply_func->result.isInitialized()) {
            //if frontier tracking is enabled (when apply function returns a boolean value)
            output_name += "_with_frontier";
        }

        if (apply->use_pull_frontier_bitvector){
            output_name += "_pull_frontier_bitvector";
        }

        if (apply->use_pull_edge_based_load_balance){
            output_name += "_pull_edge_based_load_balance";
        }

        return output_name;
    }

}
