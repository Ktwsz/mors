set -x

cd build
cmake ..
cmake --build .

cd ..

PYTHON_LIB_DIR=$(python3.13 -c 'import site; print(site.getsitepackages()[0])')

cp mors_ir.cpython-313-x86_64-linux-gnu.so $PYTHON_LIB_DIR
cp mors_lib.py $PYTHON_LIB_DIR
cp mors_emitter.py $PYTHON_LIB_DIR

mkdir /usr/lib/mors
cp share /usr/lib/mors -r

mkdir /usr/lib/mors/bin
cp build/mors /usr/lib/mors/bin
ln -s ../lib/mors/bin/mors /usr/bin/mors
