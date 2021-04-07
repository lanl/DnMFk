module load python
matname="swim"
pr=16
pc=16
m=1024
n=256
python split_data_2d.py "data/${matname}.mat" $pr $pc "Results_${matname}/"
sbatch run_datamats_swim.sh ${PWD}/Results_${matname}/ 1 $pr $pc $m $n
