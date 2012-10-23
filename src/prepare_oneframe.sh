#!/bin/bash

code_home=/home/michal/addsig/src	# place where the codes are 
data=/storage/VSR1-allsky-data/xdat	# place where the data is 
band=196                                # band number 
howmany=10				# how many simulations
gsize=2         			# +- gsize around the grid
h0=0.55e-3        			# GW amplitude 
list_of_frames=${data}/good-frames	# list of frames to analyze

#----------------------------------------------------------------------

cp ${code_home}/script_oneframe.sh script.sh
cp ${code_home}/clean.sh .
cp ${code_home}/get_summary.sh .
#cp ${code_home}/job.sub .

#for the case when signal enters another band
next_band=$(($band+1))

# some replacements in script.sh 
sed -i 's|BAND|'$band'|g;s|GSIZE|'$gsize'|g;s|H0|'$h0'|g' script.sh
sed -i 's|DATA|'$data'|g;s|LOF|'$list_of_frames'/'$band'.d|g' script.sh
sed -i 's|NEXTB|'$next_band'|g;s|LONF|'$list_of_frames'/'$next_band'.d|g' script.sh 

# loop below produces a file called run.sh that launches the simulations 
# in catalogues 01, 02,..., $howmany
# First, let's delete the old run.sh 
rm -f run.sh

cata=${PWD##*/}
for dirs in $(seq -w 1 $howmany); do

	if [ ! -d "$dirs" ]; then 
		mkdir $dirs
	fi

	ln -sf ${code_home}/search $dirs/search
	ln -sf ${code_home}/sigen $dirs/sigen
	ln -sf ${code_home}/b2t $dirs/b2t
 	ln -sf ../script.sh $dirs/script.sh
	ln -sf ${code_home}/job.sub $dirs/job.sub

        string=${h0%e-3}_${band}_${cata}__${dirs}
	echo "cd "${PWD}"/"${dirs}"; qsub -N "${string}" job.sub" >> run.sh

done

chmod 755 run.sh 

exit 0
