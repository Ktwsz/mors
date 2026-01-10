set -x

PYTHON_LIB_DIR=$(python -c 'import site; print(site.getsitepackages()[0])')

rm $PYTHON_LIB_DIR/mors_ir.cpython-313-x86_64-linux-gnu.so 
rm $PYTHON_LIB_DIR/mors_lib.py 
rm $PYTHON_LIB_DIR/mors_emitter.py 

rm /usr/lib/mors -rf
rm /usr/bin/mors

