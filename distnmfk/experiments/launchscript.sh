module load python
matname="swim"
pr=16
pc=16
m=1024
n=256
python split_data_2d.py "data/${matname}.mat" $pr $pc "Results_${matname}_10k/"
sbatch run_Datamats.sh ${PWD}/Results_${matname}_10k/ 1 $pr $pc $m $n
