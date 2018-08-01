call conda activate pymol
python setup.py install --no-cxx11 --no-libxml > setup.log 2>&1
python setup_msvc_parse_log.py
