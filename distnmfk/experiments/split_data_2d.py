import sys, os
from scipy.io import loadmat
import numpy as np
import pandas as pd

data_file = sys.argv[1]
if "mat" in data_file[-3:]:
    mat = loadmat(data_file)
    A = mat['X']
    print("Shape of Input mat is ", A.shape)
else:
    print("Wrong input format, can only deal with '*.mat' files for now.")
    sys.exit(0)

pr = int(sys.argv[2])
pc = int(sys.argv[3])
p = pr * pc
print("splitting data Pr =", type(pr),"Pc =", type(pc))
row_wise = True if mat['X'].shape[0] > mat['X'].shape[1] else False

path = sys.argv[4] #os.getcwd()+"/"
os.makedirs(path+str(p)+"cores/",exist_ok=True)
#if not os.path.exists(path+"cores/"):
#    os.mkdir(path+str(p)+"cores/")
os.makedirs(path,exist_ok=True)
if row_wise: # A nob for the split between row-wise and column-wise.
    """
    # Following works only to split the input matrix row-wise into p parts
    """
    nrows = A.shape[0]//pr
    ncols = A.shape[1]//pc
    cnt =0
    for i in range(0, pr):
        for j in range(0,pc):
            np.savetxt(path+str(p)+"cores/A_"+str(cnt), A[i*nrows:(i+1)*nrows,j*ncols:(j+1)*ncols], delimiter="\t")
            cnt = cnt +1
else:
    """
    # Following works only to split the input matrix column-wise into p parts
    Will TEST the following later.
    """
    raise Exception("Sorry, Rows > Columns , for now ") 
    ncols = A.shape[1]//p
    for i in range(0, p):
        np.savetxt(path+str(p)+"cores/A_"+str(i), A[:, i*ncols:(i+1)*ncols], delimiter="\t")


# Command to run this file to split the input data
# python split_data.py data_file num_mpi_ranks_rows num_mpi_ranks_columns
