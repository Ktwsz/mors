#!/usr/bin/env bash

set -ex

cd build
cmake ..
cmake --build .