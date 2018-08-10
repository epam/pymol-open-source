# Installing PyMOL for development from sources on Windows

The easiest way to start is to use the python environment manager so that all your pymol packages reside in a separate directory and do not clash with your generic python installation. You can live with a global python installation if you'd like. On your own risk.

## Install Anaconda Python Environment Manager

[Download and install Miniconda][Miniconda] for Python 2.7, 64 bit (!). Accept selecting as a default Python installation and adding to PATH despite the warning. Note that "Pymol 1.8.0.0 is not available for 32 bits version..."

[Miniconda]: https://conda.io/miniconda.html

Run Anaconda Prompt, create a new environment for PyMol with Python 2.7 defaults:

    conda create -n pymol python=2.7
    conda activate pymol

Install and then uninstall PyMOL so that all dependencies appear in your environment:

    conda install -c schrodinger pymol=2.1.0
    conda uninstall pymol

Something like "The following NEW packages will be INSTALLED" message appears. The package list may change but here is an example:

    apbs:                1.5-1                   schrodinger
    asn1crypto:          0.24.0-py27_0
    blas:                1.0-mkl
    ca-certificates:     2018.03.07-0
    certifi:             2018.4.16-py27_0
    cffi:                1.11.5-py27hdb016f4_0
    chardet:             3.0.4-py27h56c3b73_1
    cryptography:        2.2.2-py27h0c8e037_0
    decorator:           4.3.0-py27_0
    enum34:              1.1.6-py27h2aa175b_1
    freemol:             1.158-py27_0            schrodinger
    freetype:            2.8-hea645e0_1
    glew:                2.0.0-vc9_0             schrodinger [vc9]
    h5py:                2.8.0-py27h5f1a774_0
    hdf5:                1.10.2-h4b43f98_1
    icc_rt:              2017.0.4-h97af966_0
    icu:                 58.2-h2aa20d9_1
    idna:                2.7-py27_0
    intel-openmp:        2018.0.3-0
    ipaddress:           1.0.22-py27_0
    jpeg:                9b-ha175dff_2
    libiconv:            1.15-hda2e4ec_7
    libpng:              1.6.34-h325896a_0
    libtiff:             4.0.9-h1c3b264_1
    libxml2:             2.9.4-vc9_4             schrodinger [vc9]
    linecache2:          1.0.0-py27h6c509b2_0
    mengine:             1-0                     schrodinger
    mkl:                 2018.0.3-1
    mkl_fft:             1.0.1-py27hc997a72_0
    mpeg_encode:         1-0                     schrodinger
    msgpack-python:      0.5.6-py27hdc96acc_0
    mtz2ccp4_px:         1.0-1                   schrodinger
    networkx:            2.1-py27_0
    numpy:               1.14.5-py27h911edcf_3
    numpy-base:          1.14.5-py27h0bb1d87_3
    olefile:             0.45.1-py27_0
    openssl:             1.0.2o-h2c51139_0
    pdb2pqr:             2.1.1-py27_0            schrodinger
    pillow:              5.1.0-py27h901f87c_0
    pip:                 10.0.1-py27_0
    pmw:                 2.0.1-py27_0            schrodinger
    pycparser:           2.18-py27hb43d16c_1
    pymol:               2.1.0-py27_12           schrodinger
    pyopenssl:           18.0.0-py27_0
    pyqt:                5.6.0-py27h224ed30_5
    pysocks:             1.6.8-py27_0
    python:              2.7.15-he216670_0
    qt:                  5.6.2-vc9hc26998b_12
    requests:            2.19.1-py27_0
    rigimol:             1.3-2                   schrodinger
    setuptools:          39.2.0-py27_0
    simplemmtf-python:   0.1-py27_1              schrodinger
    sip:                 4.18.1-py27h5ec1c1a_2
    six:                 1.11.0-py27ha5e1701_1
    sqlite:              3.24.0-h7a46e7a_0
    tk:                  8.6.7-h144d9c4_3
    traceback2:          1.4.0-py27h4ec7efc_0
    unittest2:           1.1.0-py27h6240796_0
    urllib3:             1.23-py27_0
    vc:                  9-h7299396_1
    vs2008_runtime:      9.00.30729.1-hfaea7d5_1
    vs2013_runtime:      12.0.21005-1
    wheel:               0.31.1-py27_0
    win_inet_pton:       1.0.1-py27hf41312a_1
    win_unicode_console: 0.5-py27hc037021_0
    wincertstore:        0.2-py27hf04cefb_0
    zlib:                1.2.11-hbc2faf4_2

## Compile PyMOL Sources from Scratch

[Download PyMOL sources][PyMOL] (e.g. `pymol-v2.1.0.tar.bz2`). Unpack it to a directory, say, `C:\Projects\pymol`.

Initialize a new git repository there for keeping track of changes.

    git init
    git add *
    git commit -m "Unpack pymol-v2.1.0.tar.bz2"

Add at least the following to `.gitignore`:

    build/
    pymol.egg-info/
    *.pyc

[PyMOL]: https://sourceforge.net/projects/pymol/files

### Microsoft Compiler

The official way to compile C++ packages for Python is to use the same compiler which is used for the Python itself. Python 2.7 uses Microsoft Visual Studio 2008, so you need one for compiling PyMOL.
Unless you already have 64-bit Microsoft Visual Studio 2008 (Express) installed you must [download and install Microsoft Visual C++ Compiler for Python 2.7][MSVC]. Note that in this case you must patch `setup.py` to use `setuptools` instead of `distutils` since the latter do not know how to find this custom compiler.

```python
# from distutils.core import setup
from setuptools import setup
```

You'll need to add more changes to `setup.py` as win32 "branch not tested in years and may not work..." --- replace gcc warning options with MSVC onces, add win32 libraries, and probably copy required dlls along with the PyMOL installation.

[MSVC]: https://www.microsoft.com/en-us/download/details.aspx?id=44266

### Name Conflicts

Quite strange that memory allocations in PyMOL go through `Alloc()` defines which are non-namespaced, non-prefixed and completely dangerous to use as global defines. Of course, there's a name confilct with some win32 headers and the compiler goes mad about this redefinition. That's why you need to patch all the sources to prefix those methods with the help of the following script:

```python
import os
import re

dirs = ['layer%i' % i for i in xrange(6)]
names = ['Alloc', 'Calloc', 'Realloc', 'FreeP', 'DeleteP', 'DeleteAP']

for path in dirs:
  for filename in os.listdir(path):
    fullname = os.path.join(path, filename)
    originalText = text = None
    with open(fullname, 'r') as fi:
      originalText = text = fi.read()
      for name in names:
        text = re.sub(r'\b(%s)\b' % name, r'PyMol\1', text)
    if text and text != originalText:
      with open(fullname, 'w') as fo:
        fo.write(text)
```

### C++11

The next problem is quite tiresome. Lately the lazy keyword `auto` has been used in the PyMOL code base instead of specifying the exact variable type. However, the official compiler for Python 2.7 on Windows is Visual Studio 2008, which (evidently) cannot support C++ in its 2011-th year standard. One solution is to use Visual Studio 2017 (which is unsupported for Python 2.7 package development and takes several gigabytes on your hard drive), another one is to fix up the sources (_PYMOL_NO_CXX11 define suggests that C++11 code should be isolated). Unfortunately, there's no tool to do this automatically.

### Done

Ensure that PyMOL python environment is selected and run installation from sources.

    conda activate pymol
    python setup.py install --no-cxx11 --no-libxml

After successfull compilation just run the PyMOL:

    pymol

## Compile PyMOL Win32-Ready Fork

Install a compiler. As already mentioned above (in 'Microsoft Compiler' section), the official way to compile C++ packages for Python is to [download and install Microsoft Visual C++ Compiler for Python 2.7][MSVC] which is actually a variety of Visual Studio 2008. Unfortunately, it doesn't support C++11 features, so the sources are modified to satisfy the old beast. Just in case you need to update the sources from upstream, `setup_msvc_patch_allocs.py` script is available (see motivation in 'Name Conflicts' section above), but you'll need to fix all C++11 usages by hand.

Get a fork somewhere:

    mkdir <YOUR_PYMOL_FOLDER>
    cd <YOUR_PYMOL_FOLDER>
    git clone <URL_OF_THE_FORK> .

Don't forget to select the proper conda environment:

    conda activate pymol

Use a `build.cmd` script to compile and filter errors and warnings (adjust filters in `setup_msvc_parse_log.py` if you need to). Check the `setup.log` afterwards for more details.

    build

Run the compiled application:

    pymol

You can even try to debug it in Visual Studio 2017 Community with the help of automatically generated solution file (check `build/_cmd.sln`).

Use a `dist.cmd` script to build a source (zip) and a binary (wheel) distribution. Find the packed application in a `dist/` subdirectory.

    dist

Distribute the wheel for easier installation on user PCs:

    pip install <THE_WHEEL_FILE>

You can create a standalone distribution using [WinPython][] (2.7, 64 bit) and [Inno Setup][]. During the build process an `.iss` script is generated right in the `build` subdirectory, e.g. `PyMOL-2.1.0.iss`. Install WinPython in a subdirectory with a matching name (`PyMOL-2.1.0`), go there and install the wheel into this portable python installation (use WinPython Command Prompt and pip install). Then, open and execute the generated Inno Setup script, the `PyMOL-2.1.0.exe` executable will be created. You may use it to install a standalone PyMOL application, multiple versions at the same time on the same PC.

[WinPython]: https://sourceforge.net/projects/winpython/files/WinPython_2.7/
[Inno Setup]: http://www.jrsoftware.org/isdl.php

That's all for now.
