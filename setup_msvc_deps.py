import os
import sys
import re
import time
import uuid

class DependencyEntry:
    def __init__(self, path):
        self.path = path
        self.exists = os.path.exists(path)
        self.time = os.path.getmtime(path) if self.exists else None
        self.invalid = None
        self.deps = set()

    def depends_on(self, other):
        if not isinstance(other, DependencyEntry):
            raise TypeError('Expected a DependencyEntry')
        self.deps.add(other)

    def is_invalid(self):
        if self.invalid == None:
            self.invalid = (
                # file is missing
                not self.exists or
                # there are dependencies and
                len(self.deps) > 0 and (
                    # one of dependencies is invalid
                    any(dep.is_invalid() for dep in self.deps) or
                    # one of dependencies is newer
                    max(dep.time for dep in self.deps) > self.time
                )
            )
        return self.invalid

class ShowIncludes:
    prefix = 'Note: including file: '
    prefix_length = len(prefix)

    @staticmethod
    def match(line):
        return line.startswith(ShowIncludes.prefix)

    @staticmethod
    def extract(line):
        return os.path.abspath(line[ShowIncludes.prefix_length:].strip())

def strip_prefix(path, prefix):
    if isinstance(prefix, str):
        prefix = re.compile(re.escape(prefix), re.I)
    matched = re.match(prefix, path)
    if matched:
        return path[matched.end(0) + 1:]
    return path

class DependencyTracker:
    def __init__(self, outdir, cc, cc_opts, pp_opts, extra):
        self.root = os.path.dirname(os.path.abspath(__file__))
        self.root_re = re.compile(re.escape(self.root), re.I)
        self.outdir = os.path.abspath(outdir)
        self.cc = cc
        self.cc_opts = cc_opts
        self.pp_opts = pp_opts
        self.extra = extra
        self.entries = {}
        self.sources = {}
        self.time_rebuild = 0

    def add(self, obj, src):
        obj = os.path.abspath(obj)
        src = os.path.abspath(src)
        dep = obj + '.dep'
        self.sources[obj] = (src, dep)

        # expect it only once
        paths = (obj, src, dep)
        for path in paths:
            if path in self.entries:
                raise AssertionError('Already in dependency tracker: ' + path)
        
        # add files as entries
        entries = map(DependencyEntry, paths)
        for entry in entries:
            self.entries[entry.path] = entry

        # wire basic dependencies
        obj_entry, src_entry, dep_entry = entries
        obj_entry.depends_on(dep_entry)
        obj_entry.depends_on(src_entry)
        dep_entry.depends_on(src_entry)

        # read a list of dependencies
        if dep_entry.exists:
            with open(dep_entry.path) as f:
                for header in f.read().splitlines():
                    header = os.path.abspath(header)
                    if header not in self.entries:
                        self.entries[header] = DependencyEntry(header)
                    dep_entry.depends_on(self.entries[header])

    def is_invalid(self, path):
        path = os.path.abspath(path)
        entry = self.entries[path]
        return entry.is_invalid()

    def rebuild_deps(self, obj):
        time_start = time.time()

        obj = os.path.abspath(obj)
        src, dep = self.sources[obj]
        print 'Rebuilding deps for', strip_prefix(obj, self.outdir)
        sys.stdout.flush()

        # execute preprocessor on the source file
        import subprocess
        proc = subprocess.Popen(
            [self.cc, '/c', '/Fonul', src, '/showIncludes'] + self.pp_opts + self.extra,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = proc.communicate()

        # parse a printed list of includes
        includes = set(
            map(ShowIncludes.extract,
                filter(ShowIncludes.match, stdout.splitlines() + stderr.splitlines())
            )
        )

        # ignore system includes
        filtered_includes = set(
            map(lambda path: strip_prefix(path, self.root_re),
                filter(lambda path: re.match(self.root_re, path), includes)
            )
        )

        # save them to a dependency list file
        with open(dep, 'w') as f:
            f.write('\n'.join(sorted(filtered_includes)))

        # # save raw stderr for debugging
        # with open(dep + '.raw', 'w') as f:
        #     f.write('root: ' + self.root + '\n\n\n')
        #     f.write('\n'.join(sorted(includes)) + '\n\n\n')
        #     f.write('stderr:\n\n' + stderr + '\n\n')
        #     f.write('stdout:\n\n' + stdout)

        self.time_rebuild += time.time() - time_start

    def report_time(self, seconds):
        return '%02i:%02i' % (int(seconds / 60), int(seconds) % 60)

    def dump(self):
        with open(os.path.join(self.outdir, 'DependencyTracker.log'), 'w') as f:
            for key in sorted(self.entries.keys()):
                entry = self.entries[key]
                f.write('%s :: %s :: %s\n' % (entry.path, entry.time, entry.is_invalid()))

    def save_vs2017_project(self, projname):
        projfile = projname + '.vcxproj'
        slnfile = projname + '.sln'

        projname_uuid = str(uuid.uuid3(uuid.NAMESPACE_URL, projname)).upper()
        projfile_uuid = str(uuid.uuid3(uuid.NAMESPACE_URL, projfile)).upper()
        slnfile_uuid = str(uuid.uuid3(uuid.NAMESPACE_URL, slnfile)).upper()

        pydir = os.environ['CONDA_PREFIX']
        projdir = os.path.join(self.root, 'build')
        projpath = os.path.join(projdir, projfile)
        slnpath = os.path.join(projdir, slnfile)

        files = [(os.path.relpath(os.path.abspath(path), projdir), os.path.relpath(os.path.dirname(path), self.root)) for path in self.entries.keys() if re.match(r'.*\.(c|cpp|h)$', path, re.I)]
        cpp_files = filter(lambda pair: re.match(r'.*\.c(pp)?$', pair[0], re.I), files)
        h_files = filter(lambda pair: re.match(r'.*\.h$', pair[0], re.I), files)
        include_folders = [os.path.join(pydir, 'include')] + sorted(set(os.path.dirname(relpath) for relpath, dirname in h_files))

        def cumulative_subpaths(path):
            parts = path.split('\\')
            return ['\\'.join(parts[:i]) for i in xrange(1, len(parts))]

        folders = set(dirname for relpath, dirname in files)
        for folder in list(folders):
            if '\\' in folder:
                folders |= set(cumulative_subpaths(folder))
        folders = sorted(folders)

        with open(slnpath, 'w') as f:
            f.write('''\
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 15
VisualStudioVersion = 15.0.27703.2047
MinimumVisualStudioVersion = 10.0.40219.1
Project("{%(ProjectNameUUID)s}") = "%(ProjectName)s", "%(ProjectFile)s", "{%(ProjectFileUUID)s}"
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|x64 = Debug|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
		{%(ProjectFileUUID)s}.Debug|x64.ActiveCfg = Debug|x64
		{%(ProjectFileUUID)s}.Debug|x64.Build.0 = Debug|x64
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {%(SolutionFileUUID)s}
	EndGlobalSection
EndGlobal''' % {
            'ProjectName': projname,
            'ProjectNameUUID': projname_uuid,
            'ProjectFile': projfile,
            'ProjectFileUUID': projfile_uuid,
            'SolutionFileUUID': slnfile_uuid,
        })

        filters_xml = '\n'.join('''\
    <Filter Include="%s">
      <UniqueIdentifier>{%s}</UniqueIdentifier>
    </Filter>''' % (folder, str(uuid.uuid3(uuid.NAMESPACE_URL, folder))) for folder in folders)

        compile_xml = '\n'.join('<ClCompile Include="%s" />' % relpath for relpath, dirname in cpp_files)

        compile_filter_xml = '\n'.join('''\
    <ClCompile Include="%s">
      <Filter>%s</Filter>
    </ClCompile>''' % (relpath, dirname) for relpath, dirname in cpp_files)

        include_xml = '\n'.join('<ClInclude Include="%s" />' % relpath for relpath, dirname in h_files)

        include_filter_xml = '\n'.join('''\
    <ClInclude Include="%s">
      <Filter>%s</Filter>
    </ClInclude>''' % (relpath, dirname) for relpath, dirname in h_files)

        with open(projpath, 'w') as f:
            f.write('''\
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>15.0</VCProjectVersion>
    <ProjectGuid>{%(ProjectFileUUID)s}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>pymol</RootNamespace>
    <WindowsTargetPlatformVersion>10.0.14393.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v141</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader>
      </PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>Disabled</Optimization>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>%(AdditionalIncludeDirectories)s</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
%(ClCompile)s
  </ItemGroup>
  <ItemGroup>
%(ClInclude)s
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>''' % {
            'ProjectFileUUID': projfile_uuid,
            'ClCompile': compile_xml,
            'ClInclude': include_xml,
            'AdditionalIncludeDirectories': ';'.join(include_folders),
        })

        with open(projpath + '.filters', 'w') as f:
            f.write('''\
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
%s
  </ItemGroup>
  <ItemGroup>
%s
  </ItemGroup>
  <ItemGroup>
%s
  </ItemGroup>
</Project>''' % (filters_xml, compile_filter_xml, include_filter_xml))

        with open(projpath + '.user', 'w') as f:
            f.write('''\
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LocalDebuggerCommand>%(PythonFolder)s\\python.exe</LocalDebuggerCommand>
    <LocalDebuggerCommandArguments>%(PythonFolder)s\\Lib\\site-packages\\pymol\\__init__.py</LocalDebuggerCommandArguments>
    <LocalDebuggerWorkingDirectory>%(PythonFolder)s</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>''' % {
            'PythonFolder': pydir,
        })
