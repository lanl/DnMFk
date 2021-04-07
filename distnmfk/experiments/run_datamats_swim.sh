#!/bin/bash

#Submit this script with: sbatch filename

#SBATCH --time=08:15:00   # walltime
#SBATCH --nodes=8   # number of nodes
#SBATCH --ntasks-per-node=32 # number of tasks per node
#SBATCH --qos=standard   # qos name
#SBATCH --signal=23@60  # send signal to job at [seconds] before end


# LOAD MODULEFILES, INSERT CODE, AND RUN YOUR PROGRAMS HERE
module load gcc openmpi 

data=$1
resout="${data}/"
execdir="${data}/../../"
silout="silhouettes/"
kl=2 #lower bound for the Rank of my input
ku=20 #upper bound for the Rank of my input
l=30 #number of perturbations
mkdir -p ${data}/silhouettes/
mkdir -p ${data}/factors/
p=$(($3*$4))
for i in 4 
#for i in 0 
do
    for k in $(seq $kl $ku)
    do
    for j in 3000
    do
        out="a${i}_k${k}_t${j}"
#echo $k
mpirun -np $p "${execdir}/distnmfk" -a $i -k $k -i "${data}/${p}cores/A_" -t $j -d "$5 $6" -p "$3 $4" -o "${resout}/factors/nmfout_${out}" -r "0.001 0 0.001 0" -e 1 -u $k -l $l -h "${resout}/${silout}">& "${resout}/out.txt"
	#mpirun -np 24 "${execdir}/distnmfk" -a $i -k $k -i "${resout}/24cores/A_" -t $j -d "576 384" -p "6 4" -o "${resout}/factors/nmfout_${out}" -r "0.00 0 0.00 0" -e 1 -u $u -l $l -h "${resout}/${silout}">& "${resout}/out.txt"
	#mpirun -np 24 "${data}/../distnmfk" -a $i -k $k -i "${resout}/24cores/A_" -t $j -d "576 384" -p "6 4" -o "${resout}/factors/nmfout_${out}" -r "0.00 0 0.00 0" -e 1 -u $u -l $l -h "${resout}/${silout}">& "${resout}/out.txt"
    done
done
done
