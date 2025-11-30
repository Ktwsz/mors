#!/usr/bin/env bash

set -x

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )


cd build
cmake ..
cmake --build .

cd ..

ln -s $(pwd)/mors_lib $(python3.13 -m site --user-site)/mors_lib || true
for f in ~/.profile ~/.bashrc ~/.zshrc; do
    [ -f "$f" ] && echo "export PATH=\"$SCRIPT_DIR/build:\$PATH\"" >> "$f"
    source $f
done