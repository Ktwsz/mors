#!/usr/bin/env bash

set -ex



if [ $1 == "all" ]; then
    MODELS=$(find models -mindepth 1 -maxdepth 1 -type d -exec basename {} \;)
else
    MODELS=$1
fi

for MODEL in $MODELS;
do 
    DATA=""
    if [[ $(find ./models/$MODEL -name '*.dzn') ]]; then
        DATA=models/$MODEL/$MODEL.dzn
    fi 
    
    eval "./build/Debug/mors models/${MODEL}/${MODEL}.mzn $DATA "|| true
    # eval "python3.11 models/${MODEL}/${MODEL}.py"
done


