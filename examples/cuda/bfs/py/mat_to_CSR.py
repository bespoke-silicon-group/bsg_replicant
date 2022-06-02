#!usr/bin/python
import random
import sys, os
import argparse
import numpy as np
import scipy
from scipy.sparse import csr_matrix,coo_matrix,csc_matrix,isspmatrix_csc
from scipy.sparse import save_npz 
from scipy.io import loadmat
print('scipy version: {}\n'.format(scipy.__version__))


###########################################################################################
# This script will output sparse matrix in CSR format
###########################################################################################
# Three output files
# _val.dat: non-zero values
# _idx.dat: column indices
# _ptr.dat: row pointer


def main(args):

    # define file name
    #file_path       = "./"
    #file_path       = "/work/shared/common/research/graphblas/data/sparse_matrix_graph/"
    file_path       = str(args.path)
    file_name       = file_path + str(args.name) + ".mtx"
    file_name_base  = args.name
    file_name_idx_A = file_name_base + "_A_idx.dat"
    file_name_ptr_A = file_name_base + "_A_ptr.dat"
    
    #read the row index, column index and value of the sparse matrix from the .mat file 
    row_coo = []
    col_coo = []
    val_coo = []

    # load the matrix file in .mtx format
    with open(file_name,'r') as mat_f:
        line = mat_f.readline()
        #get matrix type in the first line
        para_parse = line.split(" ")
        mat_type = para_parse[3]
        symmetric = para_parse[4]
        print(symmetric)
        while True:
            line = mat_f.readline()
            if not line.startswith('%'):
                break
        #line encoding matrix size and nnz
        para_parse = line.split(" ")
        mat_m = int(para_parse[0])
        mat_n = int(para_parse[1])
        num_nnz = int(para_parse[2])
        if symmetric.strip() == 'symmetric':
            num_nnz = num_nnz *2   
        print("m: {}, n: {}, nnz: {}\n".format(mat_m,mat_n,num_nnz))
        line = mat_f.readline()
        cnt = 0
        while line:
            data_parse = line.split(" ")
            row_coo.append(int(data_parse[0])-1)
            col_coo.append(int(data_parse[1])-1)
            if mat_type.strip() == 'pattern':
                val_coo.append(random.randint(1,30))
            else:
                val_coo.append(float(data_parse[2]))
            cnt = cnt + 1
            line = mat_f.readline()
        print("cnt is {}\n".format(cnt))
    
    #creat a sparse matrix in the coo format support by scipy
    coo = coo_matrix((val_coo,(row_coo,col_coo)),shape=(mat_m,mat_n))
    #if in symmetric format, generate the complement matrix
    if symmetric.strip() == 'symmetric':
        print("matrix attribute get\n")
        coo_cpl = coo_matrix((val_coo,(col_coo,row_coo)),shape=(mat_m,mat_n))
        coo = coo + coo_cpl
        #num_nnz = num_nnz*2
    spm_density = float(num_nnz)/float(mat_m*mat_n)
    
    #construct csr matrix from coo format
    sparse_matrix = coo.tocsr()
    ptr_A     = sparse_matrix.indptr 
    idx_A     = sparse_matrix.indices
    
    
     

    # open files
    f_idx_ptr      = open(file_name_idx_A, "w")
    f_ptr_ptr      = open(file_name_ptr_A, "w")


    ################################### 
    # matrix A
    ################################### 
    
    for i in range(len(ptr_A)):
        f_ptr_ptr.write(str(ptr_A[i])+"\n")

    print("written to ptr file")
    

    for i in range(len(idx_A)):
        f_idx_ptr.write(str(idx_A[i])+"\n")
    
    print("written to idx file")

    
    
    # close files
    f_idx_ptr.close()
    f_ptr_ptr.close()
    
    
    ################################################
    # Dataset info
    ################################################
    if args.info:
        file_name  = "spm_tb_info.dat"
        # open file
        fp         = open(file_name, "w")
        # matrix size
        fp.write(str(mat_m)+"\n")
        fp.write(str(mat_n)+"\n")
        fp.write(args.name)
        # density
        fp.write(str(spm_density)+"\n")
        # total nnz in matrix A
        fp.write(str(num_nnz)+"\n")
        # close file
        fp.close()
        
        print("=====================================================")
        print("Please check out the file named 'spm_tb_info.dat'!")
        print("=====================================================")
        print("1st row: matrix size")
        print("2nd row: density")
        print("3rd row: total # of nnz in input  matrix A")
      

# "real" main function
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate random SpMV test cases.')
    parser.add_argument("--name", type=str, default="dump-c", 
                        help="size of matrices")
    parser.add_argument("--path", type=str, default="/work/shared/users/phd/zy383/SuiteSparse/", 
                        help="path of the source file") 
    parser.add_argument("--info", type=bool, default=True, 
                        help="Create graph info file.")   
    args = parser.parse_args()
    print(args)

    main(args)
