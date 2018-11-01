#!/usr/bin/env python
#
# This script only applies if you are performing a Python Distutils-based
# installation of PyMOL.
#
# It may assume that all of PyMOL's external dependencies are
# pre-installed into the system.

from setuptools import setup
from distutils.core import Extension
from distutils.util import change_root
from distutils.errors import *
from distutils.command.install import install
from distutils.command.build_py import build_py
from glob import glob
import shutil
import sys, os, re
import platform


# handle extra arguments
class options:
    osx_frameworks = False
    jobs = int(os.getenv('JOBS', 0))
    no_libxml = False
    pyqt = 'PyQt5,PyQt4,PySide'
    no_glut = False
    use_msgpackc = 'guess'
    help_distutils = False
    no_cxx11 = False

# OS X <= 10.8
if sys.platform == 'darwin' and tuple(
        map(int, platform.mac_ver()[0].split('.'))) < (10, 9):
    options.no_cxx11 = True

# win32 official compiler is Visual Studio 2008 (long before C++11)
if sys.platform == "win32":
    options.no_cxx11 = True
    options.no_libxml = True

try:
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--pyqt')
    parser.add_argument('--no-glut', action="store_true")
    parser.add_argument('--osx-frameworks', action="store_true",
            help="on MacOS use OpenGL and GLUT frameworks instead of shared "
            "libraries from XQuartz. Note that the GLUT framework has no "
            "mouse wheel support, so this option is generally not desired.")
    parser.add_argument('--jobs', '-j', type=int, help="for parallel builds "
            "(defaults to number of processors)")
    parser.add_argument('--no-libxml', action="store_true",
            help="skip libxml2 dependency, disables COLLADA export")
    parser.add_argument('--use-msgpackc', choices=('c++11', 'c', 'guess', 'no'),
            help="c++11: use msgpack-c header-only library; c: link against "
            "shared library; no: disable fast MMTF load support")
    parser.add_argument('--no-cxx11', action="store_true", help="Disable "
            "C++11 std library features. Will still require C++11 'auto' "
            "keyword support.")
    parser.add_argument('--help-distutils', action="store_true",
            help="show help for distutils options and exit")
    options, sys.argv[1:] = parser.parse_known_args(namespace=options)
except ImportError:
    print("argparse not available")

if options.help_distutils:
    sys.argv.append("--help")

if True:
    import monkeypatch_distutils
    monkeypatch_distutils.set_parallel_jobs(options.jobs)


def forms_uic(build_lib='modules'):
    '''
    Convert Qt UI files in "modules/pmg_qt/forms" to Python files in place
    '''

def get_prefix_path():
    '''
    Return a list of paths which will be searched for "include",
    "include/freetype2", "lib", "lib64" etc.
    '''
    try:
        return os.environ['PREFIX_PATH'].split(os.pathsep)
    except KeyError:
        pass

    if sys.platform.startswith("freebsd"):
        return ["/usr/local"]

    X11 = ['/usr/X11'] * (not options.osx_frameworks)

    if sys.platform == 'darwin':
        for prefix in ['/sw', '/opt/local', '/usr/local']:
            if sys.executable.startswith(prefix):
                return [prefix] + X11

    if is_conda_env():
        if sys.platform.startswith('win'):
            return [os.path.join(sys.prefix, 'Library')]

        return [sys.prefix] + X11

    return ['/usr'] + X11


def is_conda_env():
    return (
        'conda' in sys.prefix or
        'conda' in sys.version or
        'Continuum' in sys.version or
        sys.prefix == os.getenv('CONDA_PREFIX'))


def posix_find_lib(names, lib_dirs):
    # http://stackoverflow.com/questions/1376184/determine-if-c-library-is-installed-on-unix
    from subprocess import Popen, PIPE
    args = ["cc", "-shared", "-o", os.devnull] + ["-L" + d for d in lib_dirs]
    for name in names:
        p = Popen(args + ["-l" + name], stdout=PIPE, stderr=PIPE)
        p.communicate()
        if p.wait() == 0:
            return name
    raise IOError('could not find any of ' + str(names))


def guess_msgpackc():
    for prefix in prefix_path:
        f = os.path.join(prefix, 'include', 'msgpack', 'version_master.h')

        try:
            m = re.search(r'MSGPACK_VERSION_MAJOR\s+(\d+)', open(f).read())
        except EnvironmentError:
            continue

        if m is not None:
            major = int(m.group(1))
            if major > 1 and not options.no_cxx11:
                return 'c++11'
            if major > 0:
                return 'c'

    return 'no'


class build_py_pymol(build_py):
    def run(self):
        build_py.run(self)
        forms_uic(self.build_lib)
        if sys.platform.startswith('win'):
            self.make_inno_setup_script()

    def make_inno_setup_script(self):
        import uuid
        app_name = 'PyMOL'
        app_version = short_version = get_pymol_version()
        parsed_version = re.match(r'^(?P<main>\d+(\.\d+)*)\+.*\.(?P<build>\d+)$', app_version, re.I)
        if parsed_version:
            short_version = '%s+VR.%02i' % (parsed_version.group('main'), int(parsed_version.group('build')))
        app_folder = app_name + '-' + app_version
        app_guid = str(uuid.uuid3(uuid.NAMESPACE_URL, app_name + ' ' + app_version)).upper()
        with open(os.path.join('build', app_folder + '.iss'), 'w') as f:
            f.write('''\
#define MyAppName "%(name)s"
#define MyAppVersion "%(version)s"
#define MyAppShortVersion "%(short_version)s"
#define MyAppPublisher "Open-Source"
#define MyAppURL "https://github.com/schrodinger/pymol-open-source"
#define MyAppExeName "pymol.bat"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{%(guid)s}
AppName={#MyAppName}
AppVersion={#MyAppShortVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
SetupIconFile=pymol.ico
UninstallDisplayIcon={app}\\pymol.ico
DefaultDirName={pf}\\{#MyAppName} {#MyAppShortVersion}
DefaultGroupName={#MyAppName} {#MyAppShortVersion}
AllowNoIcons=yes
LicenseFile={#MyAppName}-{#MyAppVersion}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\LICENSE
InfoBeforeFile={#MyAppName}-{#MyAppVersion}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\README-VR
OutputDir=.
OutputBaseFilename={#MyAppName}-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#MyAppName}-{#MyAppVersion}\\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "pymol.ico"; DestDir: "{app}"; Flags: ignoreversion 

[Icons]
Name: "{group}\\{#MyAppName} {#MyAppShortVersion}"; Filename: "{app}\\%(pythonsubdir)s\\Scripts\\{#MyAppExeName}"; WorkingDir: "{app}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\"; IconFilename: "{app}\\pymol.ico"
Name: "{group}\\{cm:UninstallProgram,{#MyAppName} {#MyAppShortVersion}}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\\%(pythonsubdir)s\\Scripts\\{#MyAppExeName}"; WorkingDir: "{app}\\%(pythonsubdir)s\\Lib\\site-packages\\pymol\\pymol_path\\"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
''' % {
            'name': app_name,
            'version': app_version,
            'short_version': short_version,
            'guid': app_guid,
            'pythonsubdir': 'python-2.7.13.amd64', # for WinPython-64bit-2.7.13.1Zero distribution
        })

class install_pymol(install):
    pymol_path = None
    bundled_pmw = False
    no_launcher = False

    user_options = install.user_options + [
        ('pymol-path=', None, 'PYMOL_PATH'),
        ('bundled-pmw', None, 'install bundled Pmw module'),
        ('no-launcher', None, 'skip installation of the pymol launcher'),
        ]

    def finalize_options(self):
        install.finalize_options(self)
        if self.pymol_path is None:
            self.pymol_path = os.path.join(self.install_libbase, 'pymol', 'pymol_path')
        elif self.root is not None:
            self.pymol_path = change_root(self.root, self.pymol_path)

    def run(self):
        install.run(self)
        self.install_pymol_path()

        if not self.no_launcher:
            self.make_launch_script()

        if self.bundled_pmw:
            import tarfile
            pmwtgz = "modules/pmg_tk/pmw-py%d.tgz" % (sys.version_info[0])
            if not os.path.exists(pmwtgz):
                if sys.version_info[0] > 2:
                    raise UserWarning('bundled pmw.tgz not compatible with Python 3')
                pmwtgz = "modules/pmg_tk/pmw.tgz"
            tar = tarfile.open(pmwtgz)
            tar.extractall(self.install_libbase)
            tar.close()

    def unchroot(self, name):
        ### FIXME: Leaves the (back)slash in the string making non-relative path, "/some/root/and/subfolder" -> "/and/subfolder"
        if self.root is not None and name.startswith(self.root):
            return name[len(self.root):]
        return name

    def copy_tree_nosvn(self, src, dst):
        ignore = lambda src, names: set(['.svn']).intersection(names)
        if os.path.exists(dst):
            shutil.rmtree(dst)
        print('copying %s -> %s' % (src, dst))
        shutil.copytree(src, dst, ignore=ignore)

    def copy(self, src, dst):
        copy = self.copy_tree_nosvn if os.path.isdir(src) else self.copy_file
        copy(src, dst)

    def install_pymol_path(self):
        self.mkpath(self.pymol_path)
        for name in [ 'LICENSE', 'README-VR', 'data', 'test', 'scripts', 'examples', ]:
            self.copy(name, os.path.join(self.pymol_path, name))

    def make_launch_script(self):
        if sys.platform.startswith('win'):
           launch_script = 'pymol.bat'
        else:
           launch_script = 'pymol'

        self.mkpath(self.install_scripts)
        launch_script = os.path.join(self.install_scripts, launch_script)

        ### FIXME: An issue with paths while creating a wheel for installing on a different machine!
        # This method is called during the wheel creation on a developer's PC, not on a target user's PC.
        # Moreover, relative paths are different in case of a wheel creation.
        # Not to mention Anaconda environments.
        python_exe = os.path.abspath(sys.executable)
        pymol_file = self.unchroot(os.path.join(self.install_libbase, 'pymol', '__init__.py'))
        pymol_path = self.unchroot(self.pymol_path)

        with open(launch_script, 'w') as out:
            if sys.platform.startswith('win'):
                # paths relative to launcher, if possible
                try:
                    python_exe = '%~dp0\\' + os.path.relpath(python_exe, self.install_scripts)
                except ValueError:
                    pass
                try:
                    pymol_file = '%~dp0\\' + os.path.relpath(pymol_file, self.install_scripts)
                except ValueError:
                    pymol_file = os.path.abspath(pymol_file)

                ### FIXME: Workaround for the wheel creation issue!
                if True:
                    out.write('@"%~dp0..\\python.exe" -m pymol %*' + os.linesep)
                    return

                # out.write('set PYMOL_PATH=' + pymol_path + os.linesep)
                out.write('"%s" "%s"' % (python_exe, pymol_file))
                out.write(' %*' + os.linesep)
            else:
                out.write('#!/bin/bash' + os.linesep)
                if sys.platform.startswith('darwin'):
                    out.write('[ "$DISPLAY" == "" ] && export DISPLAY=":0.0"' + os.linesep)
                out.write('export PYMOL_PATH="%s"' % pymol_path + os.linesep)
                out.write('"%s" "%s" "$@"' % (python_exe, pymol_file) + os.linesep)

        os.chmod(launch_script, 0o755)

#============================================================================

# should be something like (build_base + "/generated"), but that's only
# known to build and install instances
generated_dir = os.path.join(os.environ.get("PYMOL_BLD", "build"), "generated")

import create_shadertext
create_shadertext.create_all(generated_dir)

# can be changed with environment variable PREFIX_PATH
prefix_path = get_prefix_path()

pymol_src_dirs = [
    "ov/src",
    "layer0",
    "layer1",
    "layer2",
    "layer3",
    "layer4",
    "layer5",
    "modules/cealign/src",
    generated_dir,
]

def_macros = [
    ("_PYMOL_LIBPNG", None),
    ("_PYMOL_INLINE", None),
]

libs = []
pyogl_libs = []
lib_dirs = []
ext_comp_args = [
    '/wd4244', # (1049) 'initializing' : conversion from 'double' to 'const float', possible loss of data
    '/wd4996', # (1029) 'fscanf': This function or variable may be unsafe. Consider using fscanf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
    '/wd4805', # (438) '&=' : unsafe mix of type 'int' and type 'bool' in operation
    '/wd4267', # (280) 'initializing' : conversion from 'size_t' to 'int', possible loss of data
    '/wd4305', # (194) '=' : truncation from 'double' to 'float'
    '/wd4800', # (62) 'int' : forcing value to bool 'true' or 'false' (performance warning)
    '/wd4101', # (20) 'j' : unreferenced local variable
    '/wd4146', # (5) unary minus operator applied to unsigned type, result still unsigned
    '/wd4018', # (5) '<=' : signed/unsigned mismatch
    '/wd4005', # (5) 'WCHAR_MIN' : macro redefinition
    '/J',      # (3) warning C4309: 'argument' : truncation of constant value
    '/EHsc',   # (2) C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
    '/wd4102', # (1) 'ok_except1' : unreferenced label
    '/wd4065', # (1) switch statement contains 'default' but no 'case' labels
    '/wd4390', # (1) ';' : empty controlled statement found; is this the intent?
] if sys.platform == "win32" else [
    # legacy stuff
    '-Wno-write-strings',
    '-Wno-unused-function',
    '-Wno-char-subscripts',
]
ext_link_args = []
ext_objects = []
data_files = []
ext_modules = []

if options.no_cxx11:
    def_macros += [
        ('_PYMOL_NO_CXX11', None),
    ]
    if options.use_msgpackc == 'c++11':
        options.use_msgpackc = 'no'

if True:
    # VMD plugin support
    pymol_src_dirs += [
        'contrib/uiuc/plugins/include',
        'contrib/uiuc/plugins/molfile_plugin/src',
    ]
    def_macros += [
        ("_PYMOL_VMD_PLUGINS", None),
    ]

if not options.no_libxml:
    # COLLADA support
    def_macros += [
        ("_HAVE_LIBXML", None)
    ]
    libs += ["xml2"]

if options.use_msgpackc == 'guess':
    options.use_msgpackc = guess_msgpackc()

if options.use_msgpackc == 'no':
    def_macros += [("_PYMOL_NO_MSGPACKC", None)]
else:
    if options.use_msgpackc == 'c++11':
        def_macros += [("MMTF_MSGPACK_USE_CPP11", None)]
    else:
        libs += ['msgpackc']

    pymol_src_dirs += ["contrib/mmtf-c"]

if options.no_glut:
    def_macros += [
        ("_PYMOL_NO_MAIN", None),
    ]

inc_dirs = list(pymol_src_dirs)

def get_sources(subdirs, suffixes=('.c', '.cpp')):
    return sorted([f for d in subdirs for s in suffixes for f in glob(d + '/*' + s)])

#============================================================================
if sys.platform=='win32': 
    # NOTE: this branch not tested in years and may not work...
    inc_dirs += [
        "win32/include",
        "win32/include/freetype",
    ]
    libs=["opengl32","glu32","freeglut","glew32","libpng16","zlib","freetype","Advapi32","Ws2_32","openvr_api"]
    pyogl_libs = ["opengl32","glu32","glut32"]
    lib_dirs=["win32/lib"]
    def_macros += [
        ("WIN32",None),
        ("_PYMOL_FREETYPE",None),
        ("nullptr","0"),
    ]
    data_files += [("Lib/site-packages/pymol", get_sources(["win32/dll"], (".dll",".pdb")))]
    ext_link_args=['/NODEFAULTLIB:"LIBC"', "/DEBUG"]
    ext_comp_args += ["/Od", "/Zi"]
#============================================================================
elif sys.platform=='cygwin':
    # NOTE: this branch not tested in years and may not work...
    libs=["glut32","opengl32","glu32","png"]
    pyogl_libs = ["glut32","opengl32","glu32"]
    lib_dirs=["/usr/lib/w32api"]
    def_macros += [
                ("CYGWIN",None),
                ("_PYMOL_LIBPNG",None)]
#============================================================================
else: # unix style (linux, mac, ...)

    def_macros += [
            ("_PYMOL_FREETYPE",None),
            ("NO_MMLIBS",None),
            ]

    try:
        import numpy
        inc_dirs += [
            numpy.get_include(),
        ]
        def_macros += [
            ("_PYMOL_NUMPY", None),
        ]
    except ImportError:
        print("numpy not available")

    libs += ["png", "freetype"]

    for prefix in prefix_path:
        for dirs, suffixes in [
                [inc_dirs, [("include",), ("include", "freetype2"), ("include", "libxml2")]],
                [lib_dirs, [("lib64",), ("lib",)]],
                ]:
            dirs.extend(filter(os.path.isdir, [os.path.join(prefix, *s) for s in suffixes]))

    if sys.platform == 'darwin' and options.osx_frameworks:
        ext_link_args += [
            "-framework", "OpenGL",
            "-framework", "GLUT",
        ]
        def_macros += [
            ("_PYMOL_OSX", None),
        ]
    else:
        pyogl_libs += ["GL"]
        if not options.no_glut:
            glut = posix_find_lib(['glut', 'freeglut'], lib_dirs)
            pyogl_libs += [glut]

    libs += ["GLEW"]
    libs += pyogl_libs

    ext_comp_args += ["-ffast-math", "-funroll-loops", "-fcommon"]

    # optimization currently causes a clang segfault on OS X 10.9 when
    # compiling layer2/RepCylBond.cpp
    if sys.platform != 'darwin':
        ext_comp_args += ["-O3"]

def get_pymol_version():
    return re.findall(r'_PyMOL_VERSION "(.*)"', open('layer0/Version.h').read())[0]

def get_packages(base, parent='', r=None):
    from os.path import join, exists
    if r is None:
        r = []
    if parent:
        r.append(parent)
    for name in os.listdir(join(base, parent)):
        if '.' not in name and exists(join(base, parent, name, '__init__.py')):
            get_packages(base, join(parent, name), r)
    return r

def without(suffix, strings):
    return filter(lambda s: not s.endswith(suffix), strings)

package_dir = dict((x, os.path.join(base, x))
        for base in ['modules']
        for x in get_packages(base))

ext_modules += [
    Extension("pymol._cmd",
              get_sources(pymol_src_dirs),
              include_dirs = inc_dirs,
              depends = without('PyMOLBuildInfo.h', get_sources(inc_dirs, ('.h'))),
              libraries = libs,
              library_dirs = lib_dirs,
              define_macros = def_macros,
              extra_link_args = ext_link_args,
              extra_compile_args = ext_comp_args,
              extra_objects = ext_objects,
    ),

    Extension("chempy.champ._champ",
        get_sources(['contrib/champ']),
        include_dirs=["contrib/champ"],
        depends = get_sources(["contrib/champ"], ('.h')),
        extra_compile_args=[
            '/wd4267', # (10) '+=' : conversion from 'size_t' to 'int', possible loss of data
            '/wd4244', # (5) '=' : conversion from 'Py_ssize_t' to 'int', possible loss of data
            '/wd4996', # (2) 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
        ] if sys.platform == "win32" else [],
    ),
]

distribution = setup ( # Distribution meta-data
    cmdclass  = {
        'build_py': build_py_pymol,
        'install': install_pymol,
    },
    name      = "pymol",
    version   = get_pymol_version(),
    author    = "Schrodinger",
    url       = "http://pymol.org",
    contact   = "pymol-users@lists.sourceforge.net",
    description = ("PyMOL is a Python-enhanced molecular graphics tool. "
        "It excels at 3D visualization of proteins, small molecules, density, "
        "surfaces, and trajectories. It also includes molecular editing, "
        "ray tracing, and movies. Open Source PyMOL is free to everyone!"),
    install_requires = [
        'Pmw',
    ] if sys.platform == "win32" else [],

    package_dir = package_dir,
    packages = list(package_dir),
    package_data = {'pmg_qt': ['forms/*.ui']},

    ext_modules = ext_modules,
    data_files  = data_files,
)
