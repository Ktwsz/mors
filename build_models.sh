#!/usr/bin/env bash

set -ex

DATA=""

if [[ $(uname -m) == "arm64" ]]; then
    MINIZINC_PATH="/Applications/MiniZincIDE.app/Contents/Resources/share/minizinc"
    SOLVER_PATH="/Applications/MiniZincIDE.app/Contents/Resources/share/minizinc/solvers/cp-sat"
else
    MINIZINC_PATH="" #TODO on linux
    SOLVER_PATH=""
fi 


if [ $1 == "all" ]; then
    MODELS=$(find models -mindepth 1 -maxdepth 1 -type d -exec basename {} \;)
else
    MODELS=$1
fi

for MODEL in $MODELS;
do 
    if [[ $(find ./models/$MODEL -name '*.dzn') ]]; then
        DATA=models/$MODEL/$MODEL.dzn
    fi 
    
    eval "./build/Debug/mors models/${MODEL}/${MODEL}.mzn $DATA --stdlib-dir $MINIZINC_PATH -I $SOLVER_PATH"
    eval "python3.11 models/${MODEL}/${MODEL}.py"
done


