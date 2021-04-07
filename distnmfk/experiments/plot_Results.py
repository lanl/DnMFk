import sys, math, glob, os, re
import pandas as pd
import matplotlib as mpl
#from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import gridspec
import matplotlib.ticker as ticker
from matplotlib.lines import Line2D 
from matplotlib import patches
#from matplotlib.patches import Rectangle

plt.rcParams['font.size'] = 38
plt.rcParams['axes.labelsize'] = 40
plt.rcParams['axes.labelweight'] = 'bold'
plt.rcParams['figure.titleweight'] = 'bold'
plt.rcParams['xtick.labelsize'] = 40
plt.rcParams['ytick.labelsize'] = 40
plt.rcParams['legend.fontsize'] = 40
plt.rcParams['figure.titlesize'] = 40
plt.rcParams['text.usetex'] = True
#plt.rcParams['figure.figsize']=[1.2,1]
#path ='/Users/rvangara/Documents/SLIC/2020_nnMFk_v01/SimulationResults/dracarys/nnmfk/Results_nnMFk_sb-2-10/'
def plot_sils(minsil,relerr,avgsil,name_of_fig,title_of_figure):
    _, ax = plt.subplots(1,1,figsize=(16,10))#
    #minsil = pd.read_csv(path+'bbc_minsil.csv', delimiter=',',header=None).values
    #relerr = pd.read_csv(path+'bbc_relerr_avg_data.csv', delimiter=',',header=None).values
    #feat = np.array([np.asarray(list(range(minsil.shape[1])))+1])
    #feat = np.arange(len(minsil_final))+1
    feat = np.arange(minsil.shape[0])+2
    print(feat.T)
    print(minsil.shape)
    print(relerr.shape)
    print(avgsil.shape)
    ax.grid(linestyle='dotted')
    #lns3 = ax[cnt].axvline(x=c_threshold, c='k', lw=3.5)
    lns1 = ax.plot(feat, avgsil.T, c='g', marker='o', ms=25, ls='-.', lw=3.5, label='Avg Silhouette')
    lns2 = ax.plot(feat.T, minsil.T, c='b', marker='o', ms=25, ls='--', lw=3.5, label='Min Silhouette')
    ax2 = ax.twinx()
    ax2.yaxis.label.set_color('red')
    ax2.tick_params(axis='y', colors='red')
    lns3 = ax2.plot(feat.T, relerr.T, c='r', marker='d', ms=25, ls ='-', lw=3.5, label='Relative Error')
    #ax.xaxis.set_major_locator(ticker.MultipleLocator(2))
    ax2.set_ylabel('Relative Error')
    ax.set_ylabel('Silhouette Width')
    #ax.set_xlim(0,8)
    #ax2.set_xlim(0,8)
    ax.yaxis.label.set_color('blue')
    ax.tick_params(axis='y', colors='blue')
    ax.set_xlabel('Latent Dimensionality $k$',fontweight='bold')
    ax.set_xticklabels(list(feat),fontweight='bold')
    lns = lns1+lns2+lns3
    labels = [l.get_label() for l in lns]
    ax.legend(lns, labels, loc='best')
    #ax.set_ylim(0.9,1.01)
    # ax[ps].set_title('Feature Identification (Data '+str(ps+1)+')')
    #plt.gca().add_patch(Rectangle((4.8,1.1),1.2,0.6,linewidth=1,edgecolor='black',facecolor='none'))
     # Add the patch to the Axes
    #ax.add_patch(rect)
    #rect = patches.Rectangle((4.8,np.min(relerr)),0.4,np.max(relerr)-np.min(relerr),linewidth=2,edgecolor='k',facecolor='none',linestyle='dashed') 
    # Add the patch to the Axes ax.add_patch(rect)
    ax.set_title('Distnmfk Results ('+title_of_figure+')')
    #plt.tight_layout()
    #ax2.add_patch(rect)
    plt.xticks(np.arange(2,len(relerr)+2))
    plt.savefig(name_of_fig+title_of_figure)


name = sys.argv[1]
name_fig =sys.argv[2]
minsil=pd.read_csv(name+"Min_sil.csv",header=None,sep=',').values.flatten()
avgsil=pd.read_csv(name+"Avg_sil.csv",header=None,sep=',').values.flatten()
Relerr=pd.read_csv(name+"Relerr_Avg.csv",header=None,sep=',').values.flatten()
print("Name",name)
plot_sils(np.asarray(minsil),np.asarray(Relerr),np.asarray(avgsil),name,name_fig)
