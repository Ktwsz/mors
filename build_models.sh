#!/usr/bin/env bash

set -e


function print_usage() {
  echo "Script for automatic build of models. The script automatically adds data file if it is present in the dir."
  echo "Usage: $(basename "$0") [MODEL_TO_BUILD | \"all\"]"
  echo ""
  echo "Arguments:"
  echo "    MODEL_TO_BUILD      Model to build from the models directory. When \"all\" is passed, will build all of the models"
  echo ""
  echo "Example:"
  echo "  ./build_models.sh all"
  echo "  ./build_models.sh aust"
}

if [[ "$1" == "--help" || "$1" == "-h" || $# -eq 0 ]]; then
  print_usage
  exit 0
fi

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
    echo ""
    echo "${MODEL}"
    echo ""
    eval "./build/mors build models/${MODEL}/${MODEL}.mzn $DATA "|| true
    eval "python3.13 models/${MODEL}/${MODEL}.py" || true
    echo ""
done


