import pandas as pd
import numpy as np
import glob, sys
sorted_fs = sorted(glob.glob(sys.argv[1]))
#print(sorted_fs)
dire = sys.argv[1] 
sils=[]
minsil=[]
for ii in range(2,21):
  name = dire+"/silhouettes/Si_at_k"+str(ii)
  #sils =[]
    # s = open(f,'r').read().strip()
  s = pd.read_csv(name, delimiter='\s+', header=None).values
  col_mean = np.mean(s, 0)
  #print("Mean in each col for a k={} is {}".format(len(col_mean), col_mean))
  sils.append(np.mean(col_mean))
  minsil.append(np.min(col_mean))
np.savetxt(dire+'Avg_sil.csv',sils,delimiter=',')
np.savetxt(dire+'Min_sil.csv',minsil,delimiter=',')
