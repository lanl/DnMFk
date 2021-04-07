import pandas as pd
import numpy as np
import glob, sys
sorted_fs = sorted(glob.glob(sys.argv[1]))
#print(sorted_fs)
dire = sys.argv[1] 
#nperts=30
#nprocs=24
allrelerrs=[]
for ii in range(2,21):
  name = dire+"/silhouettes/relerr_at_k"+str(ii)
  s = pd.read_csv(name, delimiter='\s+', header=None).values
  #allrelerrs.append(np.mean(s.reshape(nperts,nprocs)[:,0]))
  allrelerrs.append(np.mean(s))
np.savetxt(dire+'Relerr_Avg.csv',allrelerrs,delimiter=',')
