call conda activate pymol
python setup.py install --no-cxx11 --no-libxml > setup.log
python setup_msvc_parse_log.py
