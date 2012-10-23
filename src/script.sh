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

	        ./search -d DATA -i ${frame} -b NEXTB --whitenoise -a <(echo $sig_pars)	
		let "num_of_frames += 1"

	done 
fi 

# signal-to-noise estimate
snr=$(grep "SNR:" *.err | awk '{sum +=$2*$2} END {print sqrt(sum/'$num_of_frames')}')
sim_num=${PWD##*/} 

# vetoing candidates
cd candidates

for f in $(ls tri*.bin) 
	do 
		if [[ $f =~ triggers_([0-9]{2})_* ]] 
		then 
			COINCH/trigveto/trigveto $f COINCH/sum_0.05/sum_${BASH_REMATCH[1]}.hum DATA/${BASH_REMATCH[1]}/DetSSB.bin 
		fi 
	done

# find coincydence
ls pvc_trig*.bin > list_BAND.info
COINCH/cp_clean/cp_simple -scale CELL list_BAND.info > 0000_BAND.txt
COINCH/cp_clean/cp_simple -ascn -scale CELL list_BAND.info > 0001_BAND.txt
COINCH/cp_clean/cp_simple -decl -scale CELL list_BAND.info > 0010_BAND.txt
COINCH/cp_clean/cp_simple -decl -ascn -scale CELL list_BAND.info > 0011_BAND.txt
COINCH/cp_clean/cp_simple -sdwn -scale CELL list_BAND.info > 0100_BAND.txt
COINCH/cp_clean/cp_simple -sdwn -ascn -scale CELL list_BAND.info > 0101_BAND.txt
COINCH/cp_clean/cp_simple -sdwn -decl -scale CELL list_BAND.info > 0110_BAND.txt
COINCH/cp_clean/cp_simple -sdwn -decl -ascn -scale CELL list_BAND.info > 0111_BAND.txt
COINCH/cp_clean/cp_simple -freq -scale CELL list_BAND.info > 1000_BAND.txt
COINCH/cp_clean/cp_simple -freq -ascn -scale CELL list_BAND.info > 1001_BAND.txt
COINCH/cp_clean/cp_simple -freq -decl -scale CELL list_BAND.info > 1010_BAND.txt
COINCH/cp_clean/cp_simple -freq -decl -ascn -scale CELL list_BAND.info > 1011_BAND.txt
COINCH/cp_clean/cp_simple -freq -sdwn -scale CELL list_BAND.info > 1100_BAND.txt
COINCH/cp_clean/cp_simple -freq -sdwn -ascn -scale CELL list_BAND.info > 1101_BAND.txt
COINCH/cp_clean/cp_simple -freq -sdwn -decl -scale CELL list_BAND.info > 1110_BAND.txt
COINCH/cp_clean/cp_simple -freq -sdwn -decl -ascn -scale CELL list_BAND.info > 1111_BAND.txt

# find max coincydence
max=0
for f in $(ls *.txt)
  do
    if [[ $f =~ ([0-1]{4})_([0-9]{3}).+ ]]
    then 
      br1=${BASH_REMATCH[1]}
      st=$(grep 'Scalin' $f)
      if [[ $st =~ .+\ ([0-9]+)$ ]]
      then 
        if [ ${BASH_REMATCH[1]} -gt $max ]
        then
          max=${BASH_REMATCH[1]}
          dr=$br1
        fi
      fi
    fi
  done

# Make "*.stat" file
COINCH/cp_clean/resf2stat $dr'_BAND.resf'

# Estimate candidate parameters
est=$(COINCH/cp_clean/stat2date_simple -cval $max $dr'_BAND.stat') 

# cleanup 
cd ../; rm -fr state* candidates/* *.e* *.o*

# write down the summary 
echo $sim_num $sig_pars $snr $max $dr CELL $est >> H0_BAND_${sim_num}.sum

exit 0
