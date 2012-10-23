#!/bin/bash

sig_pars=''
until [ -n "$sig_pars" ]; do  
	sig_pars=$(./sigen -d DATA -b BAND -s GSIZE -a H0)
done 

echo "$sig_pars" > sig_BAND

num_of_frames=0 

for frame in $(cat LOF|sort -r); do 

	./search -d DATA -i ${frame} -b BAND --whitenoise -a <(echo $sig_pars)
	let "num_of_frames += 1"
	if [[ $? -eq 137 ]]; then
		last_frame=${frame}
		break
	fi	

done 

if [[ $last_frame ]]; then 

	for frame in $(awk '{if($1<='${last_frame}') print $1}' LONF|sort -r); do

	        ./search -d /home/mbejger/xdat -i ${frame} -b NEXTB --whitenoise -a <(echo $sig_pars)	
		let "num_of_frames += 1"

	done 
fi 

# Signal-to-noise estimated from the input data
snr=$(grep "SNR:" *.err | awk '{print $2}')

./b2t candidates/triggers_* | sort -gk5 > candidates/t
highsnr=$(tail -1  candidates/t)
sim_num=${PWD##*/} 

#rm -fr state* candidates/* *.e* *.o*
# write down summary 
echo $sim_num $sig_pars $snr $highsnr > H0_BAND_${sim_num}.sum

exit 0
