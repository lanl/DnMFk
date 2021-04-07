module load python
matname=$1
python collectsil.py "${PWD}/Results_${matname}/"
python collectrel.py "${PWD}/Results_${matname}/"
#Plot the Results
python plot_Results.py "${PWD}/Results_${matname}/" "${matname}"
