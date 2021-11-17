
#include <graphit/backend/codegen_hb/codegen_hb.h>

namespace graphit {
    int CodeGenHB::genHBCode() {
        genIncludeStmts();
        //TODO(Emily): we probably want to figure out incl statements for device code
        genEdgeSets();
        genStructTypeDecls();

        //Processing the constants, generting declartions
        //NOTE(Emily): we need to generate these in the device code file
        //and initialize in the main method/host code
        for (auto constant : mir_context_->getLoweredConstants()) {
            if ((std::dynamic_pointer_cast<mir::VectorType>(constant->type)) != nullptr) {
                mir::VectorType::Ptr type = std::dynamic_pointer_cast<mir::VectorType>(constant->type);
                // if the constant decl is a field property of an element (system vector)
                if (type->element_type != nullptr) {
                    oss = &oss_device;
                    genPropertyArrayDecl(constant);
                }
            } else if (std::dynamic_pointer_cast<mir::VertexSetType>(constant->type)) {
                // if the constant is a vertex set  decl
                // currently, no code is generated
            } else {
                // regular constant declaration
                oss = &oss_device;
                genScalarDecl(constant);
            }
        }

        // Generate global declarations for socket-local buffers used by NUMA optimization
        for (auto iter : mir_context_->edgeset_to_label_to_merge_reduce) {
            for (auto inner_iter : iter.second) {
                if (inner_iter.second->numa_aware) {
                    inner_iter.second->scalar_type->accept(this);
                    *oss << " **local_" << inner_iter.second->field_name << ";" << std::endl;
                }
            }
        }

        oss = &oss_device;
        *oss << std::endl;

        auto gen_edge_apply_function_visitor = HBEdgesetApplyFunctionGenerator(mir_context_, oss);
        gen_edge_apply_function_visitor.genEdgeApplyFuncDecls();

        *oss << std::endl;

        //Processing the functions
        std::map<std::string, mir::FuncDecl::Ptr>::iterator it;
        std::vector<mir::FuncDecl::Ptr> functions = mir_context_->getFunctionList();

        for (auto it = functions.begin(); it != functions.end(); it++) {
            it->get()->accept(this);
        }

        oss = &oss_device;

        *oss << std::endl;
        return 0;
    }

    void CodeGenHB::visit(mir::VectorType::Ptr vector_type) {

    }

    void CodeGenHB::visit(mir::ScalarType::Ptr scalar_type) {
        switch (scalar_type->type) {
            case mir::ScalarType::Type::INT:
                *oss << "int ";
                break;
            case mir::ScalarType::Type::FLOAT:
                *oss << "float ";
                break;
            case mir::ScalarType::Type::DOUBLE:
                *oss << "double ";
                break;
            case mir::ScalarType::Type::BOOL:
                *oss << "bool ";
                break;
            case mir::ScalarType::Type::STRING:
                *oss << "string ";
                break;
            default:
                break;
        }
    }

    void CodeGenHB::visit(mir::EdgeSetType::Ptr edgeset_type) {
        *oss << " GraphHB ";
    }

    void CodeGenHB::visit(mir::ElementType::Ptr element_type) {
        //currently, we generate an index id into the vectors
        //*oss << "NodeID ";
        *oss << "int "; //NOTE(Emily): we're not using the NodeID type now
    }

    void CodeGenHB::visit(mir::ForStmt::Ptr for_stmt) {
        printIndent();
        auto for_domain = for_stmt->domain;
        auto loop_var = for_stmt->loopVar;
        *oss << "for ( int " << loop_var << " = ";
        for_domain->lower->accept(this);
        *oss << "; " << loop_var << " < ";
        for_domain->upper->accept(this);
        *oss << "; " << loop_var << "++ )" << std::endl;
        printBeginIndent();
        indent();
        for_stmt->body->accept(this);
        dedent();
        printEndIndent();
        *oss << std::endl;

    }

    void CodeGenHB::visit(mir::WhileStmt::Ptr while_stmt) {
        printIndent();
        *oss << "while ( ";
        while_stmt->cond->accept(this);
        *oss << ")" << std::endl;
        printBeginIndent();
        indent();
        while_stmt->body->accept(this);
        dedent();
        printEndIndent();
        *oss << std::endl;

    }

    void CodeGenHB::visit(mir::IfStmt::Ptr stmt) {
        printIndent();
        *oss << "if (";
        stmt->cond->accept(this);
        *oss << ")" << std::endl;

        printIndent();
        *oss << " { " << std::endl;

        indent();
        stmt->ifBody->accept(this);
        dedent();

        printIndent();
        *oss << " } " << std::endl;

        if (stmt->elseBody) {
            printIndent();
            *oss << "else" << std::endl;

            printIndent();
            *oss << " { " << std::endl;

            indent();
            stmt->elseBody->accept(this);
            dedent();

            *oss << std::endl;

            printIndent();
            *oss << " } " << std::endl;

        }

    }

    void CodeGenHB::visit(mir::ExprStmt::Ptr expr_stmt) {

        if (mir::isa<mir::EdgeSetApplyExpr>(expr_stmt->expr)) {
            printIndent();
            auto edgeset_apply_expr = mir::to<mir::EdgeSetApplyExpr>(expr_stmt->expr);
            genEdgesetApplyFunctionCall(edgeset_apply_expr, "", NULL);
        } else {
            printIndent();
            expr_stmt->expr->accept(this);
            *oss << ";" << std::endl;
        }
    }

    void CodeGenHB::visit(mir::AssignStmt::Ptr assign_stmt) {
      //TODO(Emily): we don't want to allow this case - need to handle differently
        if (mir::isa<mir::VertexSetWhereExpr>(assign_stmt->expr)) {
            // declaring a new vertexset as output from where expression
            printIndent();
            assign_stmt->expr->accept(this);
            *oss << std::endl;

            printIndent();

            assign_stmt->lhs->accept(this);
            *oss << "  = ____graphit_tmp_out; "  << std::endl;
        } else if (mir::isa<mir::EdgeSetApplyExpr>(assign_stmt->expr)) {
            printIndent();
            auto edgeset_apply_expr = mir::to<mir::EdgeSetApplyExpr>(assign_stmt->expr);
            genEdgesetApplyFunctionCall(edgeset_apply_expr, "", assign_stmt->lhs );
            *oss << std::endl;

        //doing assignment on a single val of an array from host code
        } else if(oss == &oss_host && mir::isa<mir::TensorArrayReadExpr>(assign_stmt->lhs) && (mir::isa<mir::FloatLiteral>(assign_stmt->expr) || mir::isa<mir::IntLiteral>(assign_stmt->expr))) {
            auto left = mir::to<mir::TensorArrayReadExpr>(assign_stmt->lhs);
            printIndent();
            *oss << "hammerblade::insert_val(";
            left->index->accept(this);
            *oss << ", ";
            assign_stmt->expr->accept(this);
            *oss << ", ";
            left->target->accept(this);
            *oss << "_dev);" << std::endl;

        } else if (mir::isa<mir::VarExpr>(assign_stmt->lhs) && mir::isa<mir::VarExpr>(assign_stmt->expr)) {
             auto lhs_var = std::dynamic_pointer_cast<mir::VarExpr>(assign_stmt->lhs);
             auto rhs_var = std::dynamic_pointer_cast<mir::VarExpr>(assign_stmt->expr);
             if (mir::isa<mir::VertexSetType>(lhs_var->var.getType()) && mir::isa<mir::VertexSetType>(rhs_var->var.getType())) {
               printIndent();
               *oss << "hammerblade::builtin_swapVectors(";
               assign_stmt->lhs->accept(this);
               *oss << ", ";
               assign_stmt->expr->accept(this);
               *oss << ");" << std::endl;
               printIndent();
             } else {
               printIndent();
               assign_stmt->lhs->accept(this);
               *oss << " = ";
               assign_stmt->expr->accept(this);
               *oss << ";" << std::endl;
             }
        } else if (mir::isa<mir::VarExpr>(assign_stmt->lhs)) {
            auto lhs_var = std::dynamic_pointer_cast<mir::VarExpr>(assign_stmt->lhs);
            if(lhs_var->var.isInitialized()) {
              printIndent();
              assign_stmt->lhs->accept(this);
              *oss << " = ";
              assign_stmt->expr->accept(this);
              *oss << ";" << std::endl;
            } else {
              *oss << "not initialized\n";
            }
        } else {
            //NOTE(Emily): we should only be calling in device code
            printIndent();
            assign_stmt->lhs->accept(this);
            *oss << " = ";
            assign_stmt->expr->accept(this);
            *oss << ";" << std::endl;
        }
    }

    void CodeGenHB::visit(mir::CompareAndSwapStmt::Ptr cas_stmt) {
        printIndent();
        *oss << cas_stmt->tracking_var_ << " = compare_and_swap ( ";
        cas_stmt->lhs->accept(this);
        *oss << ", ";
        cas_stmt->compare_val_expr->accept(this);
        *oss << ", ";
        cas_stmt->expr->accept(this);
        *oss << ");" << std::endl;
    }

    void CodeGenHB::visit(mir::FuncDecl::Ptr func_decl) {
        // Generate function signature
        if (func_decl->name == "main") {
            oss = &oss_host;
            func_decl->isFunctor = false;
            *oss << "int " << func_decl->name << "(int argc, char * argv[])";
            oss = &oss_device;
        } else {
            // Use functors for better compiler inlining
            func_decl->isFunctor = true;
            *oss << "struct " << func_decl->name << std::endl;
            printBeginIndent();
            indent();
            *oss << std::string(2 * indentLevel, ' ');

            //NOTE(Emily): if we don't want to use functors, uncomment this
            //func_decl->isFunctor = false;

            if (func_decl->result.isInitialized()) {
                func_decl->result.getType()->accept(this);

                //insert an additional var_decl for returning result
                const auto var_decl = std::make_shared<mir::VarDecl>();
                var_decl->name = func_decl->result.getName();
                var_decl->type = func_decl->result.getType();
                if (func_decl->body->stmts == nullptr) {
                    func_decl->body->stmts = new std::vector<mir::Stmt::Ptr>();
                }
                auto it = func_decl->body->stmts->begin();
                func_decl->body->stmts->insert(it, var_decl);
            } else {
                *oss << "void ";
            }

            //NOTE(Emily): this is for the HB device kernel code
            //*oss << "__attribute__ ((noinline)) ";
            *oss << "operator() (";
            //*oss << func_decl->name << "(";
            bool printDelimiter = false;
            for (auto arg : func_decl->args) {
                if (printDelimiter) {
                    *oss << ", ";
                }

                arg.getType()->accept(this);
                *oss << arg.getName();
                printDelimiter = true;
            }
            *oss << ")";
        }

        *oss << std::endl;
        if(func_decl->name == "main")
        {
          oss = &oss_host;
          printBeginIndent();
          oss = &oss_device;
        }
        else
        {
          printBeginIndent();
        }
        indent();

        //TODO(Emily): need to modify this to fit our needs for main function initialization


        if (func_decl->name == "main") {
            //generate special initialization code for main function
            //TODO: this is probably a hack that could be fixed for later
            oss = &oss_host;
            //First, allocate the edgesets (read them from outside files if needed)

            //First, we want to load device code
            printIndent();
            *oss << "hammerblade::builtin_loadMicroCodeFromFile(ucode_path);" << std::endl;

            //NOTE(Emily): initialize all global scalars here
            for (auto constant : mir_context_->getLoweredConstants()) {
                if ((std::dynamic_pointer_cast<mir::VectorType>(constant->type)) != nullptr) {
                    mir::VectorType::Ptr type = std::dynamic_pointer_cast<mir::VectorType>(constant->type);
                    // if the constant decl is a field property of an element (system vector)
                    if (type->element_type != nullptr) {
                        oss = &oss_host;
                        genPropertyArrayInit(constant);
                    }
                } else if (std::dynamic_pointer_cast<mir::VertexSetType>(constant->type)) {
                    // if the constant is a vertex set  decl
                    // currently, no code is generated
                } else {
                    // regular constant declaration
                    oss = &oss_host;
                    genScalarInit(constant);
                }
            }

            for (auto stmt : mir_context_->edgeset_alloc_stmts) {
                stmt->accept(this);
            }

            //get device instance
            printIndent();
            *oss << "Device::Ptr device = Device::GetInstance();" << std::endl;

            // Initialize graphSegments if necessary
            auto segment_map = mir_context_->edgeset_to_label_to_num_segment;
            for (auto edge_iter = segment_map.begin(); edge_iter != segment_map.end(); edge_iter++) {
                auto edgeset = mir_context_->getConstEdgeSetByName((*edge_iter).first);
                auto edge_set_type = mir::to<mir::EdgeSetType>(edgeset->type);
                bool is_weighted = (edge_set_type->weight_type != nullptr);
                for (auto label_iter = (*edge_iter).second.begin();
                     label_iter != (*edge_iter).second.end(); label_iter++) {
                    auto edge_iter_first = (*edge_iter).first;
                    auto label_iter_first = (*label_iter).first;
                    auto label_iter_second = (*label_iter).second;
                    auto numa_aware_flag = mir_context_->edgeset_to_label_to_merge_reduce[edge_iter_first][label_iter_first]->numa_aware;

                    if (label_iter_second < 0) {
                        //do a specical case for negative number of segments. I
                        // in the case of negative integer, we use the number as argument to runtimve argument argv
                        // this is the only place in the generated code that we set the number of segments
                        *oss << "  " << edgeset->name << ".buildPullSegmentedGraphs(\"" << label_iter_first
                        << "\", " << "atoi(argv[" << -1*label_iter_second << "])"
                        << (numa_aware_flag ? ", true" : "") << ");" << std::endl;
                    } else {
                        // just use the positive integer as argument to number of segments
                        *oss << "  " << edgeset->name << ".buildPullSegmentedGraphs(\"" << label_iter_first
                        << "\", " << label_iter_second
                        << (numa_aware_flag ? ", true" : "") << ");" << std::endl;
                    }
                }
            }

            //generate allocation statemetns for field vectors
            for (auto constant : mir_context_->getLoweredConstants()) {
                if ((std::dynamic_pointer_cast<mir::VectorType>(constant->type)) != nullptr) {
                    mir::VectorType::Ptr type = std::dynamic_pointer_cast<mir::VectorType>(constant->type);
                    // if the constant decl is a field property of an element (system vector)
                    if (type->element_type != nullptr) {
                        //genPropertyArrayImplementationWithInitialization(constant);
                        //genPropertyArrayDecl(constant);
                        //if (constant->needs_allocation)
                            //NOTE(Emily): need to ensure this won't cause problems later
                            //genPropertyArrayAlloc(constant);
                    }
                } else if (std::dynamic_pointer_cast<mir::VertexSetType>(constant->type)) {
                    // if the constant is a vertex set  decl
                    // currently, no code is generated
                } else {
                    // regular constant declaration
                    //constant->accept(this);
                    genScalarAlloc(constant);
                }
            }

            // the stmts that initializes the field vectors
            for (auto stmt : mir_context_->field_vector_init_stmts) {
                stmt->accept(this);
            }

            for (auto iter : mir_context_->edgeset_to_label_to_merge_reduce) {
                for (auto inner_iter : iter.second) {

                    if ((inner_iter.second)->numa_aware) {
                        auto merge_reduce = inner_iter.second;
                        std::string local_field = "local_" + merge_reduce->field_name;
                        *oss << "  " << local_field << " = new ";
                        merge_reduce->scalar_type->accept(this);
                        *oss << "*[omp_get_num_places()];\n";

                        *oss << "  for (int socketId = 0; socketId < omp_get_num_places(); socketId++) {\n";
                        *oss << "    " << local_field << "[socketId] = (";
                        merge_reduce->scalar_type->accept(this);
                        *oss << "*)numa_alloc_onnode(sizeof(";
                        merge_reduce->scalar_type->accept(this);
                        *oss << ") * ";
                        auto count_expr = mir_context_->getElementCount(mir_context_->getElementTypeFromVectorOrSetName(merge_reduce->field_name));
                        count_expr->accept(this);
                        *oss << ", socketId);\n";

                        *oss << "    parallel_for (int n = 0; n < ";
                        count_expr->accept(this);
                        *oss << "; n++) {\n";
                        *oss << "      " << local_field << "[socketId][n] = " << merge_reduce->field_name << "[n];\n";
                        *oss << "    }\n  }\n";

                        *oss << "  omp_set_nested(1);" << std::endl;
                    }
                }
            }
            oss = &oss_device;
        }


        //if the function has a body
        if (func_decl->body->stmts) {

            if(func_decl->name == "main")
            {
              oss = &oss_host;
              func_decl->body->accept(this);
              oss = &oss_device;
            }
            else
            {
              func_decl->body->accept(this);
            }

            //print a return statemetn if there is a result
            if (func_decl->result.isInitialized()) {
                printIndent();
                *oss << "return " << func_decl->result.getName() << ";" << std::endl;
            }


        }

        if (func_decl->isFunctor) {
            dedent();
            printEndIndent();
            *oss << ";";
            *oss << std::endl;
        }
        //NOTE(Emily): This is NUMA aware related. not important for our system
        /*
        if (func_decl->name == "main") {
            for (auto iter : mir_context_->edgeset_to_label_to_merge_reduce) {
                for (auto inner_iter : iter.second) {
                    if (inner_iter.second->numa_aware) {
                        auto merge_reduce = inner_iter.second;
                        *oss << "  for (int socketId = 0; socketId < omp_get_num_places(); socketId++) {\n";
                        oss << "    numa_free(local_" << merge_reduce->field_name << "[socketId], sizeof(";
                        merge_reduce->scalar_type->accept(this);
                        oss << ") * ";
                        mir_context_->getElementCount(mir_context_->getElementTypeFromVectorOrSetName(merge_reduce->field_name))->accept(this);
                        oss << ");\n  }\n";
                    }
                }
            }
        }
        */
        dedent();
        if(func_decl->name == "main")
        {
          oss = &oss_host;
          printEndIndent();
          oss = &oss_device;
        }
        else
        {
          printEndIndent();
        }
        if(func_decl->isFunctor){
          *oss << ";";
        }
        *oss << std::endl;

    };

    void CodeGenHB::visit(mir::ReduceStmt::Ptr reduce_stmt) {

        if (mir::isa<mir::VertexSetWhereExpr>(reduce_stmt->expr) ||
            mir::isa<mir::EdgeSetApplyExpr>(reduce_stmt->expr)) {


        } else {
            switch (reduce_stmt->reduce_op_) {
                case mir::ReduceStmt::ReductionOp::SUM:
                    printIndent();
                    reduce_stmt->lhs->accept(this);
                    *oss << " += ";
                    reduce_stmt->expr->accept(this);
                    *oss << ";" << std::endl;

                    if (reduce_stmt->tracking_var_name_ != "") {
                        // need to set the tracking variable
                        printIndent();
                        *oss << reduce_stmt->tracking_var_name_ << " = true ; " << std::endl;
                    }

                    break;
                case mir::ReduceStmt::ReductionOp::MIN:
                    printIndent();
                    *oss << "if ( ( ";
                    reduce_stmt->lhs->accept(this);
                    *oss << ") > ( ";
                    reduce_stmt->expr->accept(this);
                    *oss << ") ) { " << std::endl;
                    indent();
                    printIndent();
                    reduce_stmt->lhs->accept(this);
                    *oss << "= ";
                    reduce_stmt->expr->accept(this);
                    *oss << "; " << std::endl;


                    if (reduce_stmt->tracking_var_name_ != "") {
                        // need to generate a tracking variable
                        printIndent();
                        *oss << reduce_stmt->tracking_var_name_ << " = true ; " << std::endl;
                    }

                    dedent();
                    printIndent();
                    *oss << "} " << std::endl;
                    break;
                case mir::ReduceStmt::ReductionOp::MAX:
                    //TODO: not supported yet

                    *oss << " max= ";
                    break;
                case mir::ReduceStmt::ReductionOp::ATOMIC_MIN:
                    printIndent();
                    *oss << reduce_stmt->tracking_var_name_ << " = ";
                    *oss << "writeMin( &";
                    reduce_stmt->lhs->accept(this);
                    *oss << ", ";
                    reduce_stmt->expr->accept(this);
                    *oss << " ); " << std::endl;
                    break;
                case mir::ReduceStmt::ReductionOp::ATOMIC_SUM:
                    printIndent();
                    if (reduce_stmt->tracking_var_name_ != "")
                        *oss << reduce_stmt->tracking_var_name_ << " =  true;\n";
                    *oss << "writeAdd( &";
                    reduce_stmt->lhs->accept(this);
                    *oss << ", ";
                    reduce_stmt->expr->accept(this);
                    *oss << " ); " << std::endl;
                    break;
            }

        }
    }

    void CodeGenHB::visit(mir::PrintStmt::Ptr print_stmt) {
        printIndent();
        if(mir::isa<mir::TensorArrayReadExpr>(print_stmt->expr) && oss == &oss_device) {
            auto tare = mir::to<mir::TensorArrayReadExpr>(print_stmt->expr);
            auto target_expr = mir::to<mir::VarExpr>(tare->target);
            auto type = target_expr->var.getType();
            mir::VectorType::Ptr vector_type = mir::to<mir::VectorType>(type);

            if (mir::isa<mir::ScalarType>(vector_type->vector_element_type)){
                auto scalar_type = mir::to<mir::ScalarType>(vector_type->vector_element_type)->type;
                if(scalar_type == mir::ScalarType::Type::INT) {
                    *oss << "bsg_printf(\"%i\\n\", ";
                    print_stmt->expr->accept(this);
                    *oss << ");" << std::endl;
                } else if (scalar_type == mir::ScalarType::Type::FLOAT || scalar_type == mir::ScalarType::Type::DOUBLE) {
                    *oss << "bsg_printf(\"%f\\n\", ";
                    print_stmt->expr->accept(this);
                    *oss << ");" << std::endl;
                }
                else if (scalar_type == mir::ScalarType::Type::STRING){
                    *oss << "bsg_printf(\"";
                    print_stmt->expr->accept(this);
                    *oss << "\\n\");" << std::endl;
                }
                else { assert(false && "Printing for this type not supported"); }
            }
            else {assert(false && "Printing for this type not supported");}

        } else {
            *oss << "std::cout << ";
            print_stmt->expr->accept(this);
            *oss << "<< std::endl;" << std::endl;
        }
    }

    void CodeGenHB::visit(mir::BreakStmt::Ptr print_stmt) {
        printIndent();
        *oss << "break;" << std::endl;
    }

    void CodeGenHB::visit(mir::Call::Ptr call_expr) {
        if(call_expr->name == "builtin_transpose") {
          *oss << "hammerblade::builtin_transposeHB(";
          bool printDelimiter = false;

          for (auto arg : call_expr->args) {
              if (printDelimiter) {
                  *oss << ", ";
              }
              arg->accept(this);
              printDelimiter = true;
          }
          *oss << ") ";
        }

        else if(call_expr->name == "deleteObject") {
          *oss << "hammerblade::deleteObject(";
          bool printDelimiter = false;

          for (auto arg : call_expr->args) {
              if (printDelimiter) {
                  *oss << ", ";
              }
              arg->accept(this);
              printDelimiter = true;
          }
          *oss << ") ";
        }

        else if(call_expr->name == "builtin_getVertexSetSize")
        {
          *oss << "hammerblade::builtin_getVertexSetSizeHB(";

          bool printDelimiter = false;

          for (auto arg : call_expr->args) {
              if (printDelimiter) {
                  *oss << ", ";
              }
              arg->accept(this);
              printDelimiter = true;
          }
          //TODO(Emily): need to get rid of this explicit ref to edges
          *oss << ", edges.num_nodes()) ";


        }
        else if(call_expr->name == "builtin_addVertex")
        {
          *oss << "hammerblade::builtin_addVertexHB(";
          bool printDelimiter = false;

          for (auto arg : call_expr->args) {
              if (printDelimiter) {
                  *oss << ", ";
              }
              arg->accept(this);
              printDelimiter = true;
          }
          *oss << ")";
        }
        else if(call_expr->name == "builtin_getVertices")
        {
          *oss << "hammerblade::builtin_getVerticesHB(";
          bool printDelimiter = false;
          for(auto arg : call_expr->args) {
            if (printDelimiter) {
                *oss << ", ";
            }
            arg->accept(this);
            printDelimiter = true;
          }
          *oss << ")";
        }
        else
        {
          *oss << call_expr->name;


          if (call_expr->generic_type != nullptr) {
              *oss << " < ";
              call_expr->generic_type->accept(this);
              *oss << " > ";
          }

          if (mir_context_->isFunction(call_expr->name)) {
              auto mir_func_decl = mir_context_->getFunction(call_expr->name);
              if (mir_func_decl->isFunctor)
                  *oss << "()";
          }

          *oss << "(";

          bool printDelimiter = false;

          for (auto arg : call_expr->args) {
              if (printDelimiter) {
                  *oss << ", ";
              }
              arg->accept(this);
              printDelimiter = true;
          }

          *oss << ") ";
      }
    };

    void CodeGenHB::visit(mir::TensorArrayReadExpr::Ptr expr) {
        //for dense array tensor read
        expr->target->accept(this);
        *oss << "[";
        expr->index->accept(this);
        *oss << "]";
    };

    void CodeGenHB::visit(mir::TensorStructReadExpr::Ptr expr) {
        //for dense array tensor read
        *oss << expr->array_of_struct_target << "[";
        expr->index->accept(this);
        *oss << "].";
        expr->field_target->accept(this);
        *oss << " ";
    };

    void CodeGenHB::visit(mir::VertexSetAllocExpr::Ptr alloc_expr) {
        // *oss << "new VertexSubset<int> ( ";
        // //This is the current number of elements, but we need the range
        // //alloc_expr->size_expr->accept(this);
        // const auto size_expr = mir_context_->getElementCount(alloc_expr->element_type);
        // size_expr->accept(this);
        // *oss << " , ";
        // alloc_expr->size_expr->accept(this);
        // *oss << ")";
        *oss << "new Vector<int32_t>(";
        const auto size_expr = mir_context_->getElementCount(alloc_expr->element_type);
        size_expr->accept(this);
        //TODO(Emily): may want to modify vector class to more closely follow vertexsubset host class
        //*oss << " , ";
        //alloc_expr->size_expr->accept(this);
        *oss << ", 0)"; //NOTE(Emily): always initializing all values to 0

    }

    void CodeGenHB::visit(mir::ListAllocExpr::Ptr alloc_expr) {
        *oss << "new std::vector< ";
        alloc_expr->element_type->accept(this);
        *oss << " > ( ";
        // currently we don't support initializing a vector with size
        //This is the current number of elements, but we need the range
        //alloc_expr->size_expr->accept(this);
        //const auto size_expr = mir_context_->getElementCount(alloc_expr->element_type);
        //size_expr->accept(this);
        //oss << " , ";
        //alloc_expr->size_expr->accept(this);
        *oss << ")";
    }

    void CodeGenHB::visit(mir::VertexSetApplyExpr::Ptr apply_expr) {
        //vertexset apply
        auto mir_var = std::dynamic_pointer_cast<mir::VarExpr>(apply_expr->target);
        std::string arg_list = "";
        std::string arg_def_list = "";

        //check if this is a print statement function
        // and generate on host


        if (mir_context_->isConstVertexSet(mir_var->var.getName())){
            //if the verstexset is a const / global vertexset, then we can get size easily
            auto associated_element_type = mir_context_->getElementTypeFromVectorOrSetName(mir_var->var.getName());
            assert(associated_element_type);
            auto associated_element_type_size = mir_context_->getElementCount(associated_element_type);
            assert(associated_element_type_size);
            *oss << "device->enqueueJob(\"" << apply_expr->input_function_name << "_kernel\",{";
            //apply_expr->target->accept(this);
            //*oss << ".getAddr(), ";
            associated_element_type_size->accept(this);
            *oss <<  "});" << std::endl;
            printIndent();
            *oss << "device->runJobs()";
            arg_def_list = "int V";
            if(apply_expr->input_function_name.find("print") != std::string::npos) {
                genVertexsetPrintKernel(apply_expr, arg_def_list);
            }
            else {
                genVertexsetApplyKernel(apply_expr, arg_def_list);
            }
        } else {
            //TODO(Emily): need to find other solution to get length of array.
            *oss << "device->enqueueJob(\"" << apply_expr->input_function_name << "_kernel\",{";
            *oss << mir_var->var.getName() << ".getAddr()";
            *oss <<  "});" << std::endl;
            printIndent();
            *oss << "device->runJobs()";
            arg_def_list = "int V";
            if(apply_expr->input_function_name.find("print") != std::string::npos) {
                genVertexsetPrintKernel(apply_expr, arg_def_list);
            }
            else {
                genVertexsetApplyKernel(apply_expr, arg_def_list);
            }

        }


    }
    void CodeGenHB::visit(mir::VertexSetWhereExpr::Ptr vertexset_where_expr) {


        //dense vertex set apply
        if (vertexset_where_expr->is_constant_set) {
            auto associated_element_type =
            mir_context_->getElementTypeFromVectorOrSetName(vertexset_where_expr->target);
            assert(associated_element_type);
            auto associated_element_type_size = mir_context_->getElementCount(associated_element_type);
            assert(associated_element_type_size);
            //*oss << "auto ____graphit_tmp_out = new VertexSubset <NodeID> ( ";
            *oss << "Vector<int32_t> ____graphit_tmp_out = new Vector<int32_t> ( ";

            //get the total number of vertices in the vertex set

            associated_element_type_size->accept(this);
            *oss << " , ";
            //vertices_range_expr->accept(this);
            // the output vertexset is initially set to 0
            *oss << "0";
            *oss << " );" << std::endl;
            printIndent();
            std::string next_bool_map_name = "next" + mir_context_->getUniqueNameCounterString();
            *oss << "GlobalScalar<hb_mc_eva_t> " << next_bool_map_name << "_dev = GlobalScalar<hb_mc_eva_t>(\" " << next_bool_map_name << "\");" << std::endl;
            oss = &oss_device;
            *oss << "__attribute__ ((section(\".dram\"))) bool * __restrict " << next_bool_map_name << ";" << std::endl;
            oss = &oss_host;
            printIndent();
            *oss << next_bool_map_name <<"_dev.set(device->malloc(" << next_bool_map_name << "_dev.scalar_size() * ";
            associated_element_type_size->accept(this);
            *oss << "));\n";
            oss = &oss_device;
            *oss << "extern \"C\" int __attribute__ ((noinline)) " << vertexset_where_expr->input_func << "_where_call(int V, int block_size_x) { " << std::endl;
            *oss << "\t" << "int start_x = block_size_x * (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);" << std::endl;
            *oss << "\t" << "for (int iter_x = __bsg_id; iter_x < block_size_x; iter_x += bsg_tiles_X * bsg_tiles_Y) {" << std::endl;
            *oss << "\t\t" << "if (iter_x < V) {" << std::endl;
            *oss << "\t\t\t" << next_bool_map_name << "[iter_x] = 0;" << std::endl;
            *oss << "\t\t\t" << "if ( " << vertexset_where_expr->input_func << "()( iter_x ) )" << std::endl;
            *oss << "\t\t\t\t" << next_bool_map_name << "[iter_x] = 1;" << std::endl;
            *oss << "\t\t" << "}" << std::endl;
            *oss << "\t\t" << "else { break; }" << std::endl;
            *oss << "\t" << "} //end of loop\n";
            *oss << "\t" << "return 0;" << std::endl;
            *oss << "}" << std::endl;
            oss = &oss_host;
            printIndent();
            *oss << "device->enqueueJob(\"" << vertexset_where_expr->input_func << "_where_call\", {";
            associated_element_type_size->accept(this);
            //TODO(Emily): get rid of this explicit call to edges.
            *oss << ", edges.num_nodes()});" << std::endl;
            printIndent();
            *oss << "device->runJobs();" << std::endl;
            printIndent();
            *oss << "int32_t temp_buf[";
            associated_element_type_size->accept(this);
            *oss << "];" << std::endl;
            printIndent();
            *oss << "hammerblade::read_global_buffer(temp_buf, " << next_bool_map_name << "_dev, ";
            associated_element_type_size->accept(this);
            *oss << ");" <<std::endl;
            printIndent();
            *oss << "____graphit_tmp_out.copyToDevice(temp_buf, ";
            associated_element_type_size->accept(this);
            *oss << ");" << std::endl;
        }

    }

    void CodeGenHB::visit(mir::VarExpr::Ptr expr) {
        *oss << expr->var.getName();
    };

    //NOTE(Emily): we either want to rewrite these built in load functions
    //or do something else here
    void CodeGenHB::visit(mir::EdgeSetLoadExpr::Ptr edgeset_load_expr) {
        if (edgeset_load_expr->is_weighted_){
            *oss << "builtin_loadWeightedEdgesFromFile ( ";
            edgeset_load_expr->file_name->accept(this);
            *oss << ") ";
        } else {
            *oss << "hammerblade::builtin_loadEdgesFromFileToHB ( ";
            edgeset_load_expr->file_name->accept(this);
            *oss << ") ";
        }
    }

    void CodeGenHB::visit(mir::NegExpr::Ptr neg_expr) {
        if (neg_expr->negate) *oss << " -";
        neg_expr->operand->accept(this);
    }

    void CodeGenHB::visit(mir::EqExpr::Ptr expr) {
        *oss << "(";
        expr->operands[0]->accept(this);
        *oss << ")";

        for (unsigned i = 0; i < expr->ops.size(); ++i) {
            switch (expr->ops[i]) {
                case mir::EqExpr::Op::LT:
                    *oss << " < ";
                    break;
                case mir::EqExpr::Op::LE:
                    *oss << " <= ";
                    break;
                case mir::EqExpr::Op::GT:
                    *oss << " > ";
                    break;
                case mir::EqExpr::Op::GE:
                    *oss << " >= ";
                    break;
                case mir::EqExpr::Op::EQ:
                    *oss << " == ";
                    break;
                case mir::EqExpr::Op::NE:
                    *oss << " != ";
                    break;
                default:
                    break;
            }

            *oss << "(";
            expr->operands[i + 1]->accept(this);
            *oss << ")";
        }
    }

    void CodeGenHB::visit(mir::AndExpr::Ptr expr) {
        *oss << '(';
        expr->lhs->accept(this);
        *oss << " && ";
        expr->rhs->accept(this);
        *oss << ')';
    };

    void CodeGenHB::visit(mir::OrExpr::Ptr expr) {
        *oss << '(';
        expr->lhs->accept(this);
        *oss << " || ";
        expr->rhs->accept(this);
        *oss << ')';
    };

    void CodeGenHB::visit(mir::XorExpr::Ptr expr) {
        *oss << '(';
        expr->lhs->accept(this);
        *oss << " ^ ";
        expr->rhs->accept(this);
        *oss << ')';
    };

    void CodeGenHB::visit(mir::NotExpr::Ptr not_expr) {
        *oss << " !";
        not_expr->operand->accept(this);
    }

    void CodeGenHB::visit(mir::MulExpr::Ptr expr) {
        *oss << '(';
        expr->lhs->accept(this);
        *oss << " * ";
        expr->rhs->accept(this);
        *oss << ')';
    }

    void CodeGenHB::visit(mir::DivExpr::Ptr expr) {
        *oss << '(';
        expr->lhs->accept(this);
        *oss << " / ";
        expr->rhs->accept(this);
        *oss << ')';
    }

    void CodeGenHB::visit(mir::AddExpr::Ptr expr) {
        *oss << '(';
        expr->lhs->accept(this);
        *oss << " + ";
        expr->rhs->accept(this);
        *oss << ')';
    };

    void CodeGenHB::visit(mir::SubExpr::Ptr expr) {
        *oss << '(';
        expr->lhs->accept(this);
        *oss << " - ";
        expr->rhs->accept(this);
        *oss << ')';
    };

    void CodeGenHB::visit(mir::BoolLiteral::Ptr expr) {
        *oss << "(bool) ";
        *oss << (bool) expr->val;
    };

    void CodeGenHB::visit(mir::StringLiteral::Ptr expr) {
        *oss << "\"";
        *oss << expr->val;
        *oss << "\"";
    };

    void CodeGenHB::visit(mir::FloatLiteral::Ptr expr) {
        *oss << "(";
        *oss << "(float) ";
        *oss << expr->val;
        *oss << ") ";
    };

    void CodeGenHB::visit(mir::IntLiteral::Ptr expr) {
        *oss << "(";
        //*oss << "(int) ";
        *oss << expr->val;
        *oss << ") ";
    }

    void CodeGenHB::visit(mir::VarDecl::Ptr var_decl) {

        if (mir::isa<mir::VertexSetWhereExpr>(var_decl->initVal)) {
            // declaring a new vertexset as output from where expression
            printIndent();
            var_decl->initVal->accept(this);
            *oss << std::endl;

            printIndent();
            var_decl->type->accept(this);
            *oss << var_decl->name << "  = ____graphit_tmp_out; " << std::endl;

        //TODO(Emily): can't return from a kernel call, so need to handle this case
        } else if (mir::isa<mir::EdgeSetApplyExpr>(var_decl->initVal)) {
            printIndent();
            auto edgeset_apply_expr = mir::to<mir::EdgeSetApplyExpr>(var_decl->initVal);

            //TODO(Emily): need to check the type and might need to do more instantiate for device here
            var_decl->type->accept(this);
            *oss << var_decl->name;
            *oss << "= new Vector<int32_t>(";
            edgeset_apply_expr->target->accept(this);
            *oss << ".num_nodes(), 0);" << std::endl;
            printIndent();

            genEdgesetApplyFunctionCall(edgeset_apply_expr, var_decl->name, NULL);
        } else {
            printIndent();

            //we probably don't need the modifiers now
            //*oss << var_decl->modifier << ' ';

            var_decl->type->accept(this);
            *oss << var_decl->name << " ";
            if (var_decl->initVal != nullptr) {
                *oss << "= ";
                var_decl->initVal->accept(this);
            }
            *oss << ";" << std::endl;
        }
    }

    void CodeGenHB::visit(mir::VertexSetType::Ptr vertexset_type) {
        //*oss << "VertexSubset<int> *  ";
        *oss << "Vector<int32_t> ";
    }

    //TODO(Emily): need to change this if we don't keep vectors
    void CodeGenHB::visit(mir::ListType::Ptr list_type) {
        *oss << "std::vector< ";
        list_type->element_type->accept(this);
        *oss << " > *  ";
    }

    void CodeGenHB::visit(mir::StructTypeDecl::Ptr struct_type) {
        *oss << struct_type->name << " ";
    }


    //******END OF VISIT FUNCS****

    void CodeGenHB::genIncludeStmts() {
        oss = &oss_device;

        *oss << "#include \"bsg_manycore.h\"" << std::endl;
        *oss << "#include \"bsg_set_tile_x_y.h\"" << std::endl;
        *oss << "#define BSG_TILE_GROUP_X_DIM bsg_tiles_X" << std::endl;
        *oss << "#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y" << std::endl;
        *oss << "#include \"bsg_tile_group_barrier.hpp\"" << std::endl;
        *oss << "bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;" << std::endl;
        //NOTE(Emily): include device runtime libraries here:
        *oss << "#include <local_range.h>" << std::endl;
        *oss << "#include <vertex_struct.h>" << std::endl;
        *oss << "#include <atomics.h>" << std::endl;
        oss = &oss_host;

        *oss << "#include \"hb_intrinsics.h\"" << std::endl;
        *oss << "#include <string.h> " << std::endl;

        //TODO(Emily): need a better way of naming the ucode path based on the input program
        *oss << "const std::string ucode_path = " << std::endl;
        *oss << "\t\t \"/home/centos/bsg_bladerunner/bsg_manycore\"" << std::endl;
        *oss << "\t\t \"/software/spmd/bsg_cuda_lite_runtime/graphit_kernel_code/main.riscv\"" << std::endl;

        *oss << "using hammerblade::Device;" << std::endl;
        *oss << "using hammerblade::Vector;" << std::endl;
        *oss << "using hammerblade::GraphHB;" << std::endl;
        *oss << "using hammerblade::WGraphHB;" << std::endl;
        *oss << "using hammerblade::GlobalScalar;" <<std::endl;


        // *oss << "#include \"hammerblade-grt/printing.h\"" << std::endl;
        // *oss << "#include \"hammerblade-grt/algraph.h\"" << std::endl;
        // *oss << "#include \"hammerblade-grt/csrgraph.h\"" << std::endl;
        // *oss << "#include \"hammerblade-grt/array_size.h\"" << std::endl;
        // *oss << "#include \"hammerblade-grt/swap.h\"" << std::endl;
        // *oss << "#include \"hammerblade-grt/syscheck.h\"" << std::endl;
        // *oss << "#include \"hammerblade-grt/core_id.h\"" << std::endl;
        // *oss << "#include \"reference/serial_breadth_first_search.h\"" << std::endl;

    }

    void CodeGenHB::genEdgeSets() {
        for (auto edgeset : mir_context_->getEdgeSets()) {

            auto edge_set_type = mir::to<mir::EdgeSetType>(edgeset->type);
            if (edge_set_type->weight_type != nullptr) {
                //weighted edgeset
                //unweighted edgeset
                *oss << "WGraph " << edgeset->name << ";" << std::endl;
            } else {
                //unweighted edgeset
                *oss << "GraphHB " << edgeset->name << "; " << std::endl;
            }
        }
    }

    void CodeGenHB::genPropertyArrayDecl(mir::VarDecl::Ptr var_decl) {
        // read the name of the array
        const auto name = var_decl->name;

        // read the type of the array
        mir::VectorType::Ptr vector_type = std::dynamic_pointer_cast<mir::VectorType>(var_decl->type);
        assert(vector_type != nullptr);
        auto vector_element_type = vector_type->vector_element_type;
        assert(vector_element_type != nullptr);

        *oss << "__attribute__((section(\".dram\"))) ";
        // oss = &oss_host;
        if (!mir::isa<mir::VectorType>(vector_element_type)){
            // *oss << "Vector<";
             vector_element_type->accept(this);
            // *oss << "> " << name << ";" << std::endl;
            *oss << " * __restrict " << name << ";" << std::endl;
            oss = &oss_host;
            *oss << "GlobalScalar<hb_mc_eva_t> " << name << "_dev;" << std::endl;
        } else if (mir::isa<mir::VectorType>(vector_element_type)) {
            //if each element is a vector
            auto vector_vector_element_type = mir::to<mir::VectorType>(vector_element_type);
            assert(vector_vector_element_type->range_indexset != 0);
            int range = vector_vector_element_type->range_indexset;

            //first generates a typedef for the vector type
            *oss << "typedef ";
            vector_vector_element_type->vector_element_type->accept(this);
            std::string typedef_name = "defined_type_" + mir_context_->getUniqueNameCounterString();
            *oss << typedef_name <<  " ";
            *oss << "[ " << range << "]; " << std::endl;
            vector_vector_element_type->typedef_name_ = typedef_name;

            //use the typedef defined type to declare a new pointer
            *oss << typedef_name << " * __restrict  " << name << ";" << std::endl;

            //*oss << "Vector<" << typedef_name << "> " << name << ";" << std::endl;
            oss = &oss_host;
            *oss << "GlobalScalar<hb_mc_eva_t> " << name << "_dev;" << std::endl;

        } else {
            std::cout << "unsupported type for property: " << var_decl->name << std::endl;
            exit(0);
        }
    }

    void CodeGenHB::genPropertyArrayInit(mir::VarDecl::Ptr var_decl) {
        printIndent();

        // read the name of the array
        const auto name = var_decl->name;

        // read the type of the array
        mir::VectorType::Ptr vector_type = std::dynamic_pointer_cast<mir::VectorType>(var_decl->type);
        assert(vector_type != nullptr);
        auto vector_element_type = vector_type->vector_element_type;
        assert(vector_element_type != nullptr);


        if (!mir::isa<mir::VectorType>(vector_element_type)){
            *oss << name << "_dev = GlobalScalar<hb_mc_eva_t>(\"" << name << "\");" << std::endl;
            printIndent();
            const auto size_expr = mir_context_->getElementCount(vector_type->element_type);
            *oss << "hammerblade::init_global_array(";
            size_expr->accept(this);
            *oss << ", " << name << "_dev);" << std::endl;
            printIndent();
            *oss << "hammerblade::assign_val(0, ";
            size_expr->accept(this);
            *oss << ", ";
            var_decl->initVal->accept(this);
            *oss << ", " << name << "_dev);" << std::endl;
            // *oss << name << "= new Vector<";
            // vector_element_type->accept(this);
            // *oss << ">();" << std::endl;
        } else if (mir::isa<mir::VectorType>(vector_element_type)) {
            auto vector_type_vector_element_type = mir::to<mir::VectorType>(vector_element_type);
            *oss << name << "_dev = GlobalScalar<hb_mc_eva_t>(\"" << name << "\");" << std::endl;
            printIndent();
            const auto size_expr = mir_context_->getElementCount(vector_type->element_type);
            *oss << "hammerblade::init_global_array(";
            size_expr->accept(this);
            *oss << ", " << name << "_dev);" << std::endl;
            printIndent();
            *oss << "hammerblade::assign_val(0, ";
            size_expr->accept(this);
            *oss << ", ";
            var_decl->initVal->accept(this);
            *oss << ", " << name << "_decl);" << std::endl;
            //*oss << name << "= new Vector<" << vector_type_vector_element_type->typedef_name_ << ">();" << std::endl;
        } else {
            std::cout << "unsupported type for property: " << var_decl->name << std::endl;
            exit(0);
        }
    }

    void CodeGenHB::genScalarDecl(mir::VarDecl::Ptr var_decl){
        //the declaration and the value are separate. The value is generated as a separate assign statement in the main function
        //decl in device file
        *oss << "__attribute__((section(\".dram\"))) ";
        var_decl->type->accept(this);
        *oss << var_decl->name << "; " << std::endl;
        //decl in host file
        oss = &oss_host;
        *oss << "GlobalScalar<";
        var_decl->type->accept(this);
        *oss << "> " << var_decl->name << "_dev;" <<std::endl;

    }

    void CodeGenHB::genScalarInit(mir::VarDecl::Ptr var_decl){
        printIndent();
        *oss << var_decl->name << "_dev = GlobalScalar<";
        var_decl->type->accept(this);
        *oss << ">(\"" << var_decl->name << "\");" <<std::endl;

    }

    void CodeGenHB::genScalarAlloc(mir::VarDecl::Ptr var_decl) {

        printIndent();

        *oss << var_decl->name << " ";
        if (var_decl->initVal != nullptr) {
            *oss << "= ";
            var_decl->initVal->accept(this);
        }
        *oss << ";" << std::endl;

    }

    //TODO(Emily): look more closely at this func and see if we need to change this

    void CodeGenHB::genPropertyArrayAlloc(mir::VarDecl::Ptr var_decl) {
        const auto name = var_decl->name;
        printIndent();
        *oss << name;
        // read the size of the array
        mir::VectorType::Ptr vector_type = std::dynamic_pointer_cast<mir::VectorType>(var_decl->type);
        const auto size_expr = mir_context_->getElementCount(vector_type->element_type);
        auto vector_element_type = vector_type->vector_element_type;

        assert(size_expr != nullptr);

        /** Deprecated, now we uses a "new" allocation scheme for arrays
         oss << " = std::vector< ";
         vector_element_type->accept(this);
         oss << " >  ";
         oss << " ( ";
         size_expr->accept(this);
         oss << " ); " << std::endl;
         **/

        *oss << " = new ";

        if (mir::isa<mir::VectorType>(vector_element_type)){
            //for vector type, we use the name from typedef
            auto vector_type_vector_element_type = mir::to<mir::VectorType>(vector_element_type);
            assert(vector_type_vector_element_type->typedef_name_ != "");
            *oss << vector_type_vector_element_type->typedef_name_ << " ";
        } else {
            vector_element_type->accept(this);
        }

        *oss << "[ ";
        size_expr -> accept(this);
        *oss << "];" << std::endl;
    }

    //TODO(Emily): this needs to be changed for device kernel calls
    // we want to keep the context here to get rid of templating elsewhere
    // NOTE(Emily): we can possibly use this to aid in arg generation for vertexsets
    void CodeGenHB::genEdgesetApplyFunctionCall(mir::EdgeSetApplyExpr::Ptr apply, std::string return_arg, mir::Expr::Ptr lhs) {
        // the arguments order here has to be consistent with genEdgeApplyFunctionSignature in gen_edge_apply_func_decl.cpp

        auto apply_func = mir_context_->getFunction(apply->input_function_name);
        auto edgeset_apply_func_name = edgeset_apply_func_gen_->genFunctionName(apply);

        auto mir_var = std::dynamic_pointer_cast<mir::VarExpr>(apply->target);
        std::vector<std::string> arguments = std::vector<std::string>();
        std::vector<std::string> arguments_def = std::vector<std::string>();
        //TODO: want to move this out of while loop.
        if(return_arg == "" && apply_func->result.isInitialized()) {
          *oss << "Vector<int32_t> next_frontier = new Vector<int32_t>(";
          apply->target->accept(this);
          *oss << ".num_nodes(), 0);" << std::endl;
          printIndent();
        }
        oss = &oss_device;
        *oss << "extern \"C\" int __attribute__ ((noinline)) ";
        *oss << edgeset_apply_func_name << "_call(";
        //TODO(Emily): reenable blocking with new schedule
        if (apply->is_weighted) {
          if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::BLOCKED) {
            if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
              *oss << "vertexdata *out_indices, WNode *out_neighbors";
            } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
              *oss << "vertexdata *in_indices, WNode *in_neighbors";
            }
          } else {
            if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
              *oss << "int *out_indices, WNode *out_neighbors";
            } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
              *oss << "int *in_indices, WNode *in_neighbors";
            }
          }
        } else {
            //arguments.push_back("Graph & g");
            if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::BLOCKED) {
              if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
                *oss << "vertexdata *out_indices, int *out_neighbors";
              } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
                *oss << "vertexdata *in_indices, int *in_neighbors";
              }
            }
            else {
              if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
                *oss << "int *out_indices, int *out_neighbors";
              } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
                *oss << "int *in_indices, int *in_neighbors";
              }
            }

        }

        if (apply->from_func != "") {
            if (mir_context_->isFunction(apply->from_func)) {
                // the schedule is an input from function
                // Create functor instance
                arguments.push_back(apply->from_func + "()");
            } else {
                // the input is an input from vertexset
                arguments.push_back(apply->from_func);
                arguments_def.push_back("int *frontier");

            }
        }

        if(return_arg != "") {
          arguments.push_back(return_arg);
          arguments_def.push_back("int *" + return_arg);
        }
        else if(apply_func->result.isInitialized()){
          arguments.push_back("next_frontier");
          arguments_def.push_back("int *next_frontier");
        }

        if (apply->to_func != "") {
            if (mir_context_->isFunction(apply->to_func)) {
                // the schedule is an input to function
                // Create functor instance
                arguments.push_back(apply->to_func + "()");
            } else {
                // the input is an input to vertexset
                arguments.push_back(apply->to_func);
                arguments_def.push_back("int *to_vertexset");
            }
        }

        // the original apply function (pull direction in hybrid case)
        arguments.push_back(apply->input_function_name + "()");

        // a filter function for the push direction in hybrid code
        //TODO(Emily) implement other directions
        /*
        if (mir::isa<mir::HybridDenseEdgeSetApplyExpr>(apply)){
            auto apply_expr = mir::to<mir::HybridDenseEdgeSetApplyExpr>(apply);
            if (apply_expr->push_to_function_ != ""){
                arguments.push_back(apply_expr->push_to_function_ + "()");
            }
        }

        // the push direction apply function for hybrid schedule
        if (mir::isa<mir::HybridDenseEdgeSetApplyExpr>(apply)){
            auto apply_expr = mir::to<mir::HybridDenseEdgeSetApplyExpr>(apply);
            arguments.push_back(apply_expr->push_function_ + "()");
        }
         */

        // the edgeset that is being applied over (target)
        //apply->target->accept(this);

        for (auto &arg : arguments_def) {
          *oss << ", " << arg;
        }
        *oss << ", int V, int E, int block_size_x";

        *oss << ") {" << std::endl;
        *oss << "\t" << edgeset_apply_func_name << "(";
        if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
          *oss << "out_indices, out_neighbors";
        } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
          *oss << "in_indices, in_neighbors";
        }


        //apply->target->accept(this);
        for (auto &arg : arguments) {
          *oss << ", " << arg;
        }

        *oss << ", V, E, block_size_x";


        *oss << ");" << std::endl;
        *oss << "\t" << "return 0;" << std::endl;
        *oss << "}" << std::endl;

        oss = &oss_host;
        //printIndent();
        *oss << "device->enqueueJob(\"";
        *oss << edgeset_apply_func_name << "_call\", {";

        apply->target->accept(this);
        if(apply->manycore_schedule.hb_load_balance_type == fir::hb_schedule::SimpleHBSchedule::HBLoadBalanceType::BLOCKED) {
          if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
            *oss << ".getOutVertexListAddr()";
          } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
            *oss << ".getInVertexListAddr()";
          }

        }
        else {
          if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
            *oss << ".getOutIndicesAddr()";
          } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
            *oss << ".getInIndicesAddr()";
          }
        }
        *oss << " , ";
        apply->target->accept(this);
        if (mir::isa<mir::PushEdgeSetApplyExpr>(apply)) {
          *oss << ".getOutNeighborsAddr()";
        } else if (mir::isa<mir::PullEdgeSetApplyExpr>(apply)) {
          *oss << ".getInNeighborsAddr()";
        }

        if(lhs != NULL){
          *oss << ", ";
          lhs->accept(this);
          *oss <<".getAddr()";;
        }
        if(return_arg != "") { *oss << ", " << return_arg << ".getAddr()"; }
        else if(apply_func->result.isInitialized()){ *oss << ", next_frontier.getAddr()"; }

        *oss << ", ";
        apply->target->accept(this);
        *oss <<".num_nodes(), ";
        apply->target->accept(this);
        *oss << ".num_edges(), ";
        apply->target->accept(this);
        *oss << ".num_nodes()";
        //*oss << ", edges.num_nodes(), edges.num_edges(),edges.num_nodes()";
        *oss << "}); " << std::endl;
        printIndent();
        *oss << "device->runJobs();" << std::endl;
        //need to swap the next frontier and frontier here
        //& delete the next frontier object to be safe
        if(return_arg == "" && apply_func->result.isInitialized() && apply->from_func != "") {
          if(!mir_context_->isFunction(apply->from_func)) {
            printIndent();
            *oss << "hammerblade::builtin_swapVectors(";
            *oss << apply->from_func;
            *oss << ", next_frontier);" << std::endl;
            printIndent();
            *oss << "deleteObject(next_frontier);" << std::endl;
          }
        }
    }

    /**
     * Generate the struct types before the arrays are generated
     */
    void CodeGenHB::genStructTypeDecls() {
        for (auto const &struct_type_decl_entry : mir_context_->struct_type_decls) {
            auto struct_type_decl = struct_type_decl_entry.second;
            *oss << "typedef struct ";
            *oss << struct_type_decl->name << " { " << std::endl;

            for (auto var_decl : struct_type_decl->fields) {
                indent();
                printIndent();
                var_decl->type->accept(this);
                //we don't initialize in the struct declarations anymore
                // the initializations are done in the main function
                *oss << var_decl->name;
                // << " = ";
                //var_decl->initVal->accept(this);
                *oss << ";" << std::endl;
                dedent();
            }
            *oss << "} " << struct_type_decl->name << ";" << std::endl;
        }
    }


    //NOTE(Emily): method to generate the kernel to call vertexset apply funcs on device
    // want to figure out better way to indent (mid overall generation)
    void CodeGenHB::genVertexsetApplyKernel(mir::VertexSetApplyExpr::Ptr apply, std::string arg_list) {
        oss = &oss_device;
        *oss << "extern \"C\" int  __attribute__ ((noinline)) " << apply->input_function_name << "_kernel(" << arg_list << ") {" << std::endl;
        *oss << "\t" << "int start, end;" << std::endl;
        *oss << "\t" << "local_range(V, &start, &end);" << std::endl;
        *oss << "\t" << "for (int iter_x = start; iter_x < end; iter_x++) {" << std::endl;
        *oss << "\t\t" << apply->input_function_name << "()(iter_x);" << std::endl;
        *oss << "\t" << "}" << std::endl;
        *oss << "\t" << "barrier.sync();" << std::endl;
        *oss << "\t" << "return 0;" << std::endl;
        *oss << "}" << std::endl;

        oss = &oss_host;

    }

    void CodeGenHB::genVertexsetPrintKernel(mir::VertexSetApplyExpr::Ptr apply, std::string arg_list) {
        oss = &oss_device;
        *oss << "extern \"C\" int  __attribute__ ((noinline)) " << apply->input_function_name << "_kernel(" << arg_list << ") {" << std::endl;
        *oss << "\t" << "if(bsg_id == 0) {" << std::endl;
        *oss << "\t\t" << "for (int iter_x = 0; iter_x < V; iter_x++) {" << std::endl;
        *oss << "\t\t\t" << apply->input_function_name << "()(iter_x);" << std::endl;
        *oss << "\t\t" << "}" << std::endl;
        *oss << "\t" << "}" << std::endl;
        *oss << "\t" << "barrier.sync();" << std::endl;
        *oss << "\t" << "return 0;" << std::endl;
        *oss << "}" << std::endl;

        oss = &oss_host;
    }

}
