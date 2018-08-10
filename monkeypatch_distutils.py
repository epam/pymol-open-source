'''
Enhances distutils with:
    - better C++ support
    - incremental builds (w/o header dependency handling)
    - parallel builds
'''

import os
import sys
import distutils.sysconfig
import distutils.unixccompiler
from setup_msvc_deps import DependencyTracker

# threaded parallel map (optional)
pmap = map

try:
    import _osx_support
    _osx_support._UNIVERSAL_CONFIG_VARS += ('CXXFLAGS', 'LDCXXSHARED',)
    _osx_support._COMPILER_CONFIG_VARS += ('LDCXXSHARED',)
except ImportError:
    _osx_support = None

distutils.unixccompiler.UnixCCompiler.executables.update({
    'compiler_cxx'      : ["c++"],
    'compiler_so_cxx'   : ["c++"],
    'linker_so_cxx'     : ["c++", "-shared"],
    'linker_exe_cxx'    : ["c++"],
})


def set_parallel_jobs(N):
    '''
    Set the number of parallel build jobs.
    N=1 : single threaded
    N=0 : use number of CPUs
    '''
    global pmap

    if N == 1:
        pmap = map
    else:
        from multiprocessing import pool
        pmap = pool.ThreadPool(N or None).map


def monkeypatch(parent, name):
    '''
    Decorator to replace a function or class method. Makes the
    unpatched function available as <patchedfunction>._super
    '''
    def wrapper(func):
        orig = getattr(parent, name)
        func._super = orig
        func.__name__ = name
        setattr(parent, name, func)
        return func
    return wrapper

@monkeypatch(distutils.sysconfig, 'customize_compiler')
def customize_compiler(compiler):
    customize_compiler._super(compiler)

    if compiler.compiler_type != "unix":
        return

    (cxx, ccshared, ldcxxshared) = \
            distutils.sysconfig.get_config_vars('CXX', 'CCSHARED', 'LDCXXSHARED')

    cxx = os.environ.get('CXX') or cxx
    cxxflags = os.environ.get('CXXFLAGS', '') + ' ' + os.environ.get('CPPFLAGS', '')
    ldcxxshared = os.environ.get('LDCXXSHARED', ldcxxshared or '') + \
            ' ' + os.environ.get('LDFLAGS', '') + \
            ' ' + os.environ.get('CXXFLAGS', '') + \
            ' ' + os.environ.get('CPPFLAGS', '')

    cxx_cmd = cxx + ' ' + cxxflags

    # C++11 by default (c++0x works for now and supports more compiler versions)
    if '-std=' not in cxx_cmd:
        cxx_cmd += ' -std=c++0x'

    compiler.set_executables(
            compiler_cxx=cxx_cmd,
            compiler_so_cxx=cxx_cmd + ' ' + ccshared,
            linker_so_cxx=ldcxxshared,
            linker_exe_cxx=cxx)

@monkeypatch(distutils.unixccompiler.UnixCCompiler, 'compile')
def compile(self, sources, output_dir=None, macros=None,
        include_dirs=None, debug=0, extra_preargs=None, extra_postargs=None,
        depends=None):
    '''
    Enable parallel and incremental build.

    To do a clean build, please remove the "build" directory.
    '''
    if os.getenv('DEBUG', ''):
        debug = 1

    macros, objects, extra_postargs, pp_opts, build = self._setup_compile(
            output_dir, macros, include_dirs, sources, depends, extra_postargs)
    cc_args = self._get_cc_args(pp_opts, debug, extra_preargs)

    compiler_so = self.compiler_so
    compiler_so_cxx = self.compiler_so_cxx

    if sys.platform == 'darwin' and _osx_support is not None:
        compiler_so = _osx_support.compiler_fixup(compiler_so, cc_args + extra_postargs)
        compiler_so_cxx = _osx_support.compiler_fixup(compiler_so_cxx, cc_args + extra_postargs)

    def _single_compile(obj):
        try:
            src, ext = build[obj]
        except KeyError:
            return
        try:
            if not self.force and \
                    os.path.getmtime(obj) > \
                    os.path.getmtime(src):
                return
        except OSError:
            pass

        # _compile
        compiler = compiler_so_cxx \
                if self.detect_language(src) == 'c++' \
                else compiler_so
        try:
            self.spawn(compiler + cc_args + [src, '-o', obj] + extra_postargs)
        except distutils.errors.DistutilsExecError as msg:
            raise distutils.errors.CompileError(msg)

    for _ in pmap(_single_compile, objects):
        pass

    return objects

@monkeypatch(distutils.msvc9compiler.MSVCCompiler, '_setup_compile')
def _setup_compile(self, outdir, macros, incdirs, sources, depends, extra):
    macros, objects, extra, pp_opts, build = _setup_compile._super(self, outdir, macros, incdirs, sources, depends, extra)
    extra += ['/Fd' + outdir + '\\vc90.pdb']

    # examine only C/C++ files
    c_cpp_objects = filter(lambda obj: build[obj][1] in self._c_extensions + self._cpp_extensions, objects)

    # add them to a deps tracker
    tracker = self.__tracker = DependencyTracker(outdir, self.cc, self.compile_options_debug, pp_opts, extra)
    for obj in c_cpp_objects:
        tracker.add(obj, build[obj][0])

    # remove unchanged objects from build
    for obj in c_cpp_objects:
        obj_path = os.path.abspath(obj)
        _, dep_path = tracker.sources[obj_path]
        if not tracker.is_invalid(obj_path):
            # print 'Skipping', obj
            del build[obj]
        elif tracker.is_invalid(dep_path):
            tracker.rebuild_deps(obj_path)
        # else:
        #     print 'Using deps', dep_path
    print 'Dependency tracker update time:', tracker.report_time(tracker.time_rebuild)

    # tracker.dump()

    return (macros, objects, extra, pp_opts, build)

@monkeypatch(distutils.msvc9compiler.MSVCCompiler, 'link')
def link(self,
        target_desc,
        objects,
        output_filename,
        output_dir=None,
        libraries=None,
        library_dirs=None,
        runtime_library_dirs=None,
        export_symbols=None,
        debug=0,
        extra_preargs=None,
        extra_postargs=None,
        build_temp=None,
        target_lang=None):
    link._super(self, target_desc, objects, output_filename, output_dir, libraries, library_dirs,
        runtime_library_dirs, export_symbols, debug, extra_preargs, extra_postargs, build_temp, target_lang)
    if self.__tracker:
        self.__tracker.save_vs2017_project(os.path.splitext(os.path.basename(output_filename))[0])
